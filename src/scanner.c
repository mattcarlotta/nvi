#include "scanner.h"
#include "accessors.h"
#include "arena.h"
#include "arg.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "file.h"
#include "hashmap.h"
#include "list.h"
#include "log.h"
#include "macros.h"
#include "matcher.h"
#include "nthread.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// the walker's best knowledge of an entry's type before dispatch;
// ENTRY_UNKNOWN means a stat() is required to classify it
typedef enum {
    ENTRY_UNKNOWN,
    ENTRY_DIR,
    ENTRY_FILE,
} entry_kind_t;

typedef struct {
    char **items; // heap-owned directory paths, popped LIFO
    size_t count;
    size_t capacity;
} dir_queue_t;

typedef struct {
    const args_t *args;
    mutex_t lock;
    cond_t work_ready;
    arena_t arena; // shared queue storage; only ever touched under 'lock'
    dir_queue_t dirs;
    size_t pending;  // dirs queued or currently being processed
    result_t result; // first error wins
    bool failed;
} walk_ctx_t;

typedef struct {
    walk_ctx_t *ctx;
    scanner_t scanner; // share-nothing: private counters and env hashmap
    buf_t report;      // per-worker report buffer; each flush is one fwrite,
                       // so blocks land whole even under concurrency
    arena_t persist;   // worker lifetime: env key set, report buffer, path scratch
    arena_t scratch;   // file lifetime: contents and match list, reset after each file
    thread_t thread;
} scan_worker_t;

// ---------------------------------------------------------------------------
// dry-run reporting
//
// Reports emitted from worker threads (per-file results, empty-file warnings)
// compose into the worker's log_buf_t and flush once, which is the atomicity
// boundary stdio guarantees. Reports emitted from the main thread before or
// after the join keep using the direct log_ functions.
// ---------------------------------------------------------------------------

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

// duplicate a match key out of the scratch arena (it points into file contents) into the
// worker's persist arena before the per-file scratch reset invalidates it
static void copy_unique_env_key(arena_t *persist, scanner_t *scanner, const env_key_match_t *env) {
    if (hashmap_contains(&scanner->envs, env->key, env->key_len)) {
        return;
    }

    const char *new_key = arena_strndup(persist, env->key, env->key_len);
    hashmap_append(persist, &scanner->envs, new_key, env->key_len, 0);
}

static result_t scan_file(const args_t *args, scan_worker_t *worker, const char *path, const char *name) {
    const file_ext_t *file_ext_match = get_file_accessors(&args->scan_exts, name);
    if (file_ext_match == NULL) {
        return RESULT_OK;
    }

    file_details_t file = open_file(&worker->scratch, path);
    if (file.contents == NULL) {
        arena_reset(&worker->scratch);
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

    for (size_t i = 0; i < env_key_matches.count; ++i) {
        ++worker->scanner.references;
        copy_unique_env_key(&worker->persist, &worker->scanner, &env_key_matches.items[i]);
    }

    // contents and matches die together; keys were copied into persist above
    arena_reset(&worker->scratch);

    return RESULT_OK;
}

// queue a directory for any worker to pick up; the shared ctx arena owns the copy and is
// only touched under the lock. Popped paths are abandoned in the arena (no per-item free)
// and reclaimed all at once when the scan completes.
static void queue_dir(walk_ctx_t *ctx, const char *path) {
    mutex_lock(&ctx->lock);
    char *copy = arena_strdup(&ctx->arena, path);
    DYN_ARR_APPEND(&ctx->arena, &ctx->dirs, copy);
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

    // only classify via stat_path() when the directory listing couldn't
    // (e.g. DT_UNKNOWN filesystems, or links: stat_path is lstat on POSIX,
    // so links classify as neither dir nor file and are skipped)
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

// list one directory: scan matching files inline, queue subdirectories.
// 'scratch' is a PATH_MAX buffer owned by the calling worker.
static result_t process_dir(scan_worker_t *worker, const char *path, char *scratch) {
    result_t result = RESULT_OK;

#if defined(_WIN32) && defined(_MSC_VER)
    // build the search pattern in the scratch buffer, then reuse it for entries
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

        // FindFirstFile/FindNextFile already report the entry type, so no
        // stat is needed on this platform
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

        // d_type classifies most entries straight from the directory listing,
        // saving one stat syscall per entry; DT_UNKNOWN (filesystems that
        // don't populate d_type) and DT_LNK fall back to the stat_path path
        // in handle_entry
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

// worker loop: pop a directory, process it, repeat. Exits when a failure is
// flagged or when the queue is empty with no directories still in flight
// (pending == 0), which is the guaranteed-quiescent termination condition.
static thread_ret_t THREAD_CALL scan_worker(void *arg) {
    scan_worker_t *worker = arg;
    walk_ctx_t *ctx = worker->ctx;

    char *scratch = arena_alloc(&worker->persist, PATH_MAX);

    for (;;) {
        mutex_lock(&ctx->lock);
        while (ctx->dirs.count == 0 && ctx->pending > 0 && !ctx->failed) {
            cond_wait(&ctx->work_ready, &ctx->lock);
        }

        if (ctx->failed || ctx->dirs.count == 0) {
            mutex_unlock(&ctx->lock);
            break;
        }

        char *dir = ctx->dirs.items[--ctx->dirs.count];
        mutex_unlock(&ctx->lock);

        result_t result = process_dir(worker, dir, scratch);

        mutex_lock(&ctx->lock);
        if (!result.ok && !ctx->failed) {
            ctx->failed = true;
            ctx->result = result;
        }
        --ctx->pending;
        if (ctx->pending == 0 || ctx->failed) {
            cond_broadcast(&ctx->work_ready);
        }
        mutex_unlock(&ctx->lock);
    }

    return 0;
}

// fold a worker's private results into the main scanner. Unique keys are copied out of the
// worker's persist arena into the main arena so the worker arenas can be freed afterwards.
static void merge_worker_scanner(arena_t *main_arena, scanner_t *dst, scanner_t *src) {
    dst->dirs_scanned += src->dirs_scanned;
    dst->files_scanned += src->files_scanned;
    dst->references += src->references;

    for (size_t i = 0; i < src->envs.capacity; ++i) {
        const hashmap_entry_t *entry = &src->envs.slots[i];
        if (entry->key == NULL || hashmap_contains(&dst->envs, entry->key, entry->len)) {
            continue;
        }

        const char *key = arena_strndup(main_arena, entry->key, entry->len);
        hashmap_append(main_arena, &dst->envs, key, entry->len, 0);
    }
}

static void append_list_keys(arena_t *arena, hashmap_t *env_map, const list_t *list) {
    for (size_t i = 0; i < list->count; ++i) {
        const char *key = list->items[i];
        hashmap_append(arena, env_map, key, strlen(key), 0);
    }
}

void merge_required_envs(args_t *args, const scanner_t *scanner) {
    if (scanner->envs.count == 0) {
        return;
    }

    hashmap_t env_map = {0};

    append_list_keys(args->arena, &env_map, &args->ignored);
    append_list_keys(args->arena, &env_map, &args->required);

    for (size_t i = 0; i < scanner->envs.capacity; ++i) {
        const hashmap_entry_t *entry = &scanner->envs.slots[i];
        if (entry->key == NULL || hashmap_contains(&env_map, entry->key, entry->len)) {
            continue;
        }

        DYN_ARR_APPEND(args->arena, &args->required, entry->key);
    }

    report_required_keys(args);
}

result_t run_scanner(args_t *args, scanner_t *scanner) {
    scanner->scan_exts = &args->scan_exts;

    report_scan_start(args);

    walk_ctx_t ctx = {0};
    ctx.args = args;
    ctx.result = RESULT_OK;
    arena_init(&ctx.arena, 0);
    mutex_init(&ctx.lock);
    cond_init(&ctx.work_ready);

    queue_dir(&ctx, ".");

    // dry-run runs parallel too: buffered reports keep each block whole, at
    // the cost of file order varying run to run (completion order)
    size_t nthreads = args->scan_threads;

    if (nthreads == 1) {
        // run the worker loop on the calling thread: no spawn, same code path
        scan_worker_t worker = {.ctx = &ctx, .scanner = {.scan_exts = &args->scan_exts}};
        arena_init(&worker.persist, 0);
        arena_init(&worker.scratch, 0);
        worker.report.arena = &worker.persist;

        scan_worker(&worker);
        merge_worker_scanner(args->arena, scanner, &worker.scanner);

        arena_free(&worker.scratch);
        arena_free(&worker.persist);
    } else {
        scan_worker_t *workers = arena_alloc_zeroed(args->arena, nthreads * sizeof(*workers));

        // arenas are initialized on the main thread before any spawn so the post-join merge
        // and frees below never race a worker
        size_t spawned = 0;
        for (size_t i = 0; i < nthreads; ++i) {
            workers[i].ctx = &ctx;
            workers[i].scanner.scan_exts = &args->scan_exts;
            arena_init(&workers[i].persist, 0);
            arena_init(&workers[i].scratch, 0);
            workers[i].report.arena = &workers[i].persist;

            if (thread_create(&workers[i].thread, scan_worker, &workers[i]) != 0) {
                break; // proceed with however many threads started
            }
            ++spawned;
        }

        if (spawned == 0) {
            // no threads available: fall back to running inline (no join,
            // since no thread handle exists)
            scan_worker(&workers[0]);
        } else {
            for (size_t i = 0; i < spawned; ++i) {
                thread_join(workers[i].thread);
            }
        }

        for (size_t i = 0; i < nthreads; ++i) {
            merge_worker_scanner(args->arena, scanner, &workers[i].scanner);
            arena_free(&workers[i].scratch);
            arena_free(&workers[i].persist);
        }
    }

    // queued directory paths (including any left unprocessed by a failure) are released in
    // one call regardless of how the scan ended
    arena_free(&ctx.arena);

    cond_destroy(&ctx.work_ready);
    mutex_destroy(&ctx.lock);

    if (!ctx.result.ok) {
        return ctx.result;
    }

    report_scan_summary(args, scanner);

    merge_required_envs(args, scanner);

    return ctx.result;
}
