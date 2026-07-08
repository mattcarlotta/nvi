#include "scanner.h"
#include "accessors.h"
#include "arena.h"
#include "arg.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "file.h"
#include "list.h"
#include "log.h"
#include "macros.h"
#include "map.h"
#include "matcher.h"
#include "nthread.h"
#include "result.h"
#include "set.h"
#include "utils.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef enum {
    ENTRY_UNKNOWN,
    ENTRY_DIR,
    ENTRY_FILE,
} entry_kind_t;

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} dir_queue_t;

typedef struct {
    const args_t *args;
    mutex_t lock;
    cond_t work_ready;
    arena_t arena; // backs the dirs slot array only; guarded by lock; freed after join
    dir_queue_t dirs;
    size_t pending;
    result_t result;
} walk_ctx_t;

typedef struct {
    walk_ctx_t *ctx;
    scanner_t scanner;
    arena_t arena;   // per-worker findings arena; backs scanner.envs, freed wholesale after merge
    arena_t scratch; // per-worker file scratch; holds file.contents + matches, reset per file
    buf_t report;
    thread_t thread;
} scan_worker_t;

static void report_scan_start(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_info(SINK_STDERR, "[INFO]");
    log_f(SINK_STDERR, " Scanning for environment keys in");

    for (size_t i = 0; i < args->scan_exts.count; ++i) {
        if (i != 0 && args->scan_exts.count > 2) {
            log_f(SINK_STDERR, ",");
        }

        if (i == args->scan_exts.count - 1 && args->scan_exts.count > 1) {
            log_f(SINK_STDERR, " and");
        }

        log_fi(SINK_STDERR, " *.%s", args->scan_exts.items[i].ext);
    }

    log_f(SINK_STDERR, " files...\n\n");
}

static void report_empty_file_warning(const args_t *args, buf_t *buf, const char *path) {
    if (!args->dry_run) {
        return;
    }

    log_warning(SINK_BUF(buf), "[WARNING] The file '%s' appears to be empty; skipping.\n\n", path);
    log_buf_flush(buf);
}

static void report_file_scan_results(const args_t *args, buf_t *buf, const char *path,
                                     const env_key_matches_t *matches) {
    if (!args->dry_run || matches->count == 0) {
        return;
    }

    log_info(SINK_BUF(buf), "[INFO]");
    log_f(SINK_BUF(buf), " Scanned ");
    log_fi(SINK_BUF(buf), "%s", path);
    log_f(SINK_BUF(buf), " and found %zu key%s...\n", matches->count, TO_PLURAL(matches->count));

    for (size_t i = 0; i < matches->count; ++i) {
        const env_key_match_t *m = &matches->items[i];
        log_f(SINK_BUF(buf), "    \u2022 ");
        log_bold_info(SINK_BUF(buf), "%.*s", (int)m->key_len, m->key);
        log_comment(SINK_BUF(buf), " [%zu:%zu]\n", m->line, m->byte);
    }

    log_f(SINK_BUF(buf), "\n");
    log_buf_flush(buf);
}

static void report_scan_summary(const args_t *args, const scanner_t *scanner) {
    if (!args->dry_run) {
        return;
    }

    log_info(SINK_STDERR, "[INFO]");
    log_f(SINK_STDERR, " Walked %zu director%s, scanned %zu file%s, and found %zu reference%s to %zu unique key%s\n\n",
          scanner->dirs_scanned, TO_PLURAL(scanner->dirs_scanned, "ies", "y"), scanner->files_scanned,
          TO_PLURAL(scanner->files_scanned), scanner->references, TO_PLURAL(scanner->references), scanner->envs.count,
          TO_PLURAL(scanner->envs.count));
}

static void report_required_keys(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_info(SINK_STDERR, "[INFO]");
    log_f(SINK_STDERR, " The following ENV keys are now required...\n");

    if (args->required.count > 0) {
        for (size_t i = 0; i < args->required.count; ++i) {
            log_f(SINK_STDERR, "    \u2022 ");
            log_bold_info(SINK_STDERR, "%s", args->required.items[i]);
            log_f(SINK_STDERR, "\n");
        }
    } else {
        log_f(SINK_STDERR, "    \u2022 ");
        log_comment(SINK_STDERR, "(none)\n");
    }

    log_f(SINK_STDERR, "\n");
}

// ---------------------------------------------------------------------------
// scanning
// ---------------------------------------------------------------------------

static const file_ext_t *get_file_accessors(const file_ext_map_t *map, const char *name) {
    const char *dot = strrchr(name, DOT);
    if (dot == NULL || dot[1] == '\0') {
        return NULL;
    }

    return get_file_extension(map, dot + 1);
}

static result_t scan_file(const args_t *args, scan_worker_t *worker, const char *path, const char *name) {
    const file_ext_t *file_ext_match = get_file_accessors(&args->scan_exts, name);
    if (file_ext_match == NULL) {
        return RESULT_OK;
    }

    // file.contents and matches are per-file scratch: read into the scratch arena and rewind
    // it at the end so steady-state allocator traffic converges to zero across the tree
    file_details_t file = open_file(&worker->scratch, path);
    if (file.contents == NULL) {
        return RESULT_OK;
    }

    if (file.len == 0) {
        report_empty_file_warning(args, &worker->report, path);
        arena_reset(&worker->scratch);
        return RESULT_OK;
    }

    ++worker->scanner.files_scanned;

    env_key_matches_t env_key_matches = {0};
    scan_file_content(&worker->scratch, &file, file_ext_match, &env_key_matches);

    report_file_scan_results(args, &worker->report, path, &env_key_matches);

    // set_add copies each unique key into the findings arena; matches point into the scratch
    // arena, so the copy must happen before the reset below
    for (size_t i = 0; i < env_key_matches.count; ++i) {
        const env_key_match_t *m = &env_key_matches.items[i];
        ++worker->scanner.references;
        set_add(&worker->scanner.envs, m->key, m->key_len);
    }

    arena_reset(&worker->scratch);

    return RESULT_OK;
}

static void queue_dir(walk_ctx_t *ctx, const char *path) {
    char *copy = strdup(path);
    if (copy == NULL) {
        log_error(SINK_STDERR, "[ERROR] Failed to copy a directory path (system is out of memory?); aborting.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    mutex_lock(&ctx->lock);
    // the slot array lives in the ctx arena (serialized by this lock); each path string
    // stays on malloc and is freed on pop so peak memory tracks in-flight dirs, not all dirs
    ARENA_DYN_ARR_APPEND(&ctx->arena, &ctx->dirs, copy);
    ++ctx->pending;
    cond_signal(&ctx->work_ready);
    mutex_unlock(&ctx->lock);
}

static result_t handle_entry(scan_worker_t *worker, const char *parent, const char *name, char *path,
                             entry_kind_t kind) {
    if (name[0] == '.' || is_blacklisted(name)) {
        return RESULT_OK;
    }

    int n = snprintf(path, PATH_MAX, "%s" PATH_SEP "%s", parent, name);
    if (n < 0 || (size_t)n >= PATH_MAX) {
        return operation_error("The file path is too long: '%s" PATH_SEP "%s'\n", parent, name);
    }

    if (kind == ENTRY_UNKNOWN) {
        struct stat st;
        if (stat_path(path, &st) != 0) {
            return operation_error("Cannot locate '%s': %s\n", path, strerror(errno));
        }

        if (S_ISDIR(st.st_mode)) {
            kind = ENTRY_DIR;
        } else if (S_ISREG(st.st_mode)) {
            kind = ENTRY_FILE;
        } else {
            return RESULT_OK;
        }
    }

    if (kind == ENTRY_DIR) {
        queue_dir(worker->ctx, path);
        return RESULT_OK;
    }

    return scan_file(worker->ctx->args, worker, path, name);
}

// list a directory: scan matching files inline, queue subdirectories.
static result_t process_dir(scan_worker_t *worker, const char *path, char *scratch) {
    result_t result = RESULT_OK;

#if defined(_WIN32) && defined(_MSC_VER)
    int n = snprintf(scratch, PATH_MAX, "%s" PATH_SEP "*", path);
    if (n < 0 || (size_t)n >= PATH_MAX) {
        return operation_error("path too long: '%s'", path);
    }

    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(scratch, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return operation_error("cannot open directory '%s'; aborting.", path);
    }

    ++worker->scanner.dirs_scanned;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            continue;
        }

        entry_kind_t kind = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? ENTRY_DIR : ENTRY_FILE;

        result = handle_entry(worker, path, fd.cFileName, scratch, kind);
        if (!result.ok) {
            break;
        }
    } while (FindNextFile(h, &fd));
    FindClose(h);
#else
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return operation_error("cannot open directory '%s'; aborting.", path);
    }

    ++worker->scanner.dirs_scanned;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        entry_kind_t kind = ENTRY_UNKNOWN;

#if defined(DT_DIR) && defined(DT_REG)
        if (entry->d_type == DT_DIR) {
            kind = ENTRY_DIR;
        } else if (entry->d_type == DT_REG) {
            kind = ENTRY_FILE;
        }
#endif

        result = handle_entry(worker, path, entry->d_name, scratch, kind);
        if (!result.ok) {
            break;
        }
    }

    closedir(dir);
#endif

    return result;
}

// pop a directory, process it, repeat. Exits when a failure is
// flagged or when the queue is empty with no directories still in flight
static thread_ret_t THREAD_CALL scan_worker(void *arg) {
    scan_worker_t *worker = arg;
    walk_ctx_t *ctx = worker->ctx;

    char *scratch = malloc(PATH_MAX);
    if (scratch == NULL) {
        log_error(SINK_STDERR, "[ERROR] Failed to allocate a path buffer (system is out of memory?); aborting.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    for (;;) {
        mutex_lock(&ctx->lock);
        while (ctx->dirs.count == 0 && ctx->pending > 0 && ctx->result.ok) {
            cond_wait(&ctx->work_ready, &ctx->lock);
        }

        if (!ctx->result.ok || ctx->dirs.count == 0) {
            mutex_unlock(&ctx->lock);
            break;
        }

        char *dir = ctx->dirs.items[--ctx->dirs.count];
        mutex_unlock(&ctx->lock);

        result_t result = process_dir(worker, dir, scratch);
        free(dir);

        mutex_lock(&ctx->lock);
        if (!result.ok && ctx->result.ok) {
            ctx->result = result;
        }

        --ctx->pending;

        if (ctx->pending == 0 || !ctx->result.ok) {
            cond_broadcast(&ctx->work_ready);
        }

        mutex_unlock(&ctx->lock);
    }

    log_buf_free(&worker->report);
    free(scratch);
    return 0;
}

// Merges a worker's findings into the destination (main-arena) set. set_add copies each key
// into the destination arena, so the worker's own arena can be freed wholesale afterward.
static void merge_worker_scanner(scanner_t *dst, const scanner_t *src) {
    dst->dirs_scanned += src->dirs_scanned;
    dst->files_scanned += src->files_scanned;
    dst->references += src->references;

    for (size_t i = 0; i < src->envs.capacity; ++i) {
        const set_entry_t *entry = &src->envs.items[i];
        if (entry->key == NULL) {
            continue;
        }

        set_add(&dst->envs, entry->key, entry->len);
    }
}

void merge_required_envs(arena_t *arena, args_t *args, const scanner_t *scanner) {
    if (scanner->envs.count == 0) {
        return;
    }

    // temporary borrowed-key lookup of ignored + already-required keys. It's scratch that
    // dies before return, so it gets its own arena rather than stranding in the main one.
    // Scanner keys are already unique (deduped in scanner->envs), so this lookup only needs
    // the fixed ignored + CLI-required sets, not the keys appended below.
    arena_t lookup_arena = arena_init(0);
    map_t lookup = map_init(&lookup_arena, args->ignored.count + args->required.count);

    for (size_t i = 0; i < args->ignored.count; ++i) {
        const char *key = args->ignored.items[i];
        map_add(&lookup, key, strlen(key), 0);
    }
    for (size_t i = 0; i < args->required.count; ++i) {
        const char *key = args->required.items[i];
        map_add(&lookup, key, strlen(key), 0);
    }

    for (size_t i = 0; i < scanner->envs.capacity; ++i) {
        const set_entry_t *entry = &scanner->envs.items[i];
        if (entry->key == NULL || map_contains(&lookup, entry->key, entry->len)) {
            continue;
        }

        // required borrows the set's main-arena key pointer; both live until arena_free
        ARENA_DYN_ARR_APPEND(arena, &args->required, entry->key);
    }

    arena_free(&lookup_arena);

    report_required_keys(args);
}

result_t run_scanner(arena_t *arena, args_t *args, scanner_t *scanner) {
    scanner->scan_exts = &args->scan_exts;
    // the union set of all workers' findings lives in the main arena, so its keys survive
    // until exec and can be borrowed by args->required
    scanner->envs = set_init(arena, 0);

    report_scan_start(args);

    walk_ctx_t ctx = {.args = args, .result = RESULT_OK};
    ctx.arena = arena_init(0);
    mutex_init(&ctx.lock);
    cond_init(&ctx.work_ready);

    queue_dir(&ctx, ".");

    uint8_t nthreads = args->scan_threads;
    scan_worker_t *workers = calloc(nthreads, sizeof(*workers));
    if (workers == NULL) {
        log_error(SINK_STDERR, "[ERROR] Failed to allocate scan workers (system is out of memory?); aborting.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    for (uint8_t i = 0; i < nthreads; ++i) {
        workers[i].ctx = &ctx;
        workers[i].scanner.scan_exts = &args->scan_exts;
        // each worker owns private arenas so its bumps never contend across threads
        workers[i].arena = arena_init(0);
        workers[i].scratch = arena_init(0);
        workers[i].scanner.envs = set_init(&workers[i].arena, 0);
    }

    uint8_t spawned = 0;
    while (spawned < nthreads) {
        if (thread_create(&workers[spawned].thread, scan_worker, &workers[spawned]) != 0) {
            break;
        }
        ++spawned;
    }

    if (spawned == 0) {
        scan_worker(&workers[0]);
    } else {
        for (uint8_t i = 0; i < spawned; ++i) {
            thread_join(workers[i].thread);
        }
    }

    // single-threaded from here: merge each worker's findings into the main-arena set, then
    // release that worker's arena wholesale (keys were copied into the main arena by the merge)
    for (uint8_t i = 0; i < nthreads; ++i) {
        merge_worker_scanner(scanner, &workers[i].scanner);
        arena_free(&workers[i].arena);
        arena_free(&workers[i].scratch);
    }

    free(workers);

    // free the remaining malloc'd path strings; the slot array is arena-owned
    for (size_t i = 0; i < ctx.dirs.count; ++i) {
        free(ctx.dirs.items[i]);
    }
    arena_free(&ctx.arena);

    cond_destroy(&ctx.work_ready);
    mutex_destroy(&ctx.lock);

    if (!ctx.result.ok) {
        return ctx.result;
    }

    report_scan_summary(args, scanner);

    merge_required_envs(arena, args, scanner);

    return ctx.result;
}
