#include "scanner.h"
#include "accessors.h"
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

// I/O-bound walk; more workers than this hit diminishing returns
#define MAX_SCAN_THREADS 8

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
    dir_queue_t dirs;
    size_t pending;  // dirs queued or currently being processed
    result_t result; // first error wins
    bool failed;
} walk_ctx_t;

typedef struct {
    walk_ctx_t *ctx;
    scanner_t scanner; // share-nothing: private counters and env hashmap
    thread_t thread;
} scan_worker_t;

static void report_scan_start(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_info("[INFO]");
    log_f(" Scanning for environment keys in");

    for (size_t i = 0; i < args->scan_exts.count; ++i) {
        if (i != 0 && args->scan_exts.count > 2) {
            log_f(",");
        }

        if (i == args->scan_exts.count - 1 && args->scan_exts.count > 1) {
            log_f(" and");
        }

        log_fi(" *.%s", args->scan_exts.items[i].ext);
    }

    log_f(" files...\n\n");
}

static void report_empty_file_warning(const args_t *args, const char *path) {
    if (!args->dry_run) {
        return;
    }

    log_warning("[WARNING] The file '%s' appears to be empty; skipping.\n\n", path);
}

static void report_file_scan_results(const args_t *args, const char *path, const env_key_matches_t *matches) {
    if (!args->dry_run || matches->count == 0) {
        return;
    }

    log_info("[INFO]");
    log_f(" Scanned ");
    log_fi("%s", path);
    log_f(" and found %zu key%s...\n", matches->count, TO_PLURAL(matches->count));

    for (size_t i = 0; i < matches->count; ++i) {
        const env_key_match_t *m = &matches->items[i];
        log_f("    \u2022 ");
        log_bold_info("%.*s", (int)m->key_len, m->key);
        log_comment(" [%zu:%zu]\n", m->line, m->byte);
    }

    log_f("\n");
}

static void report_scan_summary(const args_t *args, const scanner_t *scanner) {
    if (!args->dry_run) {
        return;
    }

    log_info("[INFO]");
    log_f(" Walked %zu director%s, scanned %zu file%s, and found %zu reference%s to %zu unique key%s\n\n",
          scanner->dirs_scanned, TO_PLURAL(scanner->dirs_scanned, "ies", "y"), scanner->files_scanned,
          TO_PLURAL(scanner->files_scanned), scanner->references, TO_PLURAL(scanner->references), scanner->envs.count,
          TO_PLURAL(scanner->envs.count));
}

static void report_required_keys(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_info("[INFO]");
    log_f(" The following ENV keys are now required...\n");

    if (args->required.count > 0) {
        for (size_t i = 0; i < args->required.count; ++i) {
            log_f("    \u2022 ");
            log_bold_info("%s", args->required.items[i]);
            log_f("\n");
        }
    } else {
        log_f("    \u2022 ");
        log_comment("(none)\n");
    }

    log_f("\n");
}

static const file_ext_t *get_file_accessors(const file_ext_map_t *map, const char *name) {
    const char *dot = strrchr(name, DOT);
    if (dot == NULL || dot[1] == '\0') {
        return NULL;
    }

    return get_file_extension(map, dot + 1);
}

static void copy_unique_env_key(scanner_t *scanner, const env_key_match_t *env) {
    if (hashmap_contains(&scanner->envs, env->key, env->key_len)) {
        return;
    }

    const char *new_key = strndup(env->key, env->key_len);
    if (new_key == NULL) {
        log_error("[ERROR] Failed to copy key '%s' (system is out of memory?); aborting.\n", env->key);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    hashmap_append(&scanner->envs, new_key, env->key_len, 0);
}

static result_t scan_file(const args_t *args, scanner_t *scanner, const char *path, const char *name) {
    const file_ext_t *file_ext_match = get_file_accessors(&args->scan_exts, name);
    if (file_ext_match == NULL) {
        return RESULT_OK;
    }

    file_details_t file = open_file(path);
    if (file.contents == NULL) {
        return RESULT_OK;
    }

    if (file.len == 0) {
        report_empty_file_warning(args, path);
        free(file.contents);
        return RESULT_OK;
    }

    ++scanner->files_scanned;

    env_key_matches_t env_key_matches = {0};
    scan_file_content(&file, file_ext_match, &env_key_matches);

    report_file_scan_results(args, path, &env_key_matches);

    for (size_t i = 0; i < env_key_matches.count; ++i) {
        ++scanner->references;
        copy_unique_env_key(scanner, &env_key_matches.items[i]);
    }

    free_env_key_matches(&env_key_matches);
    free(file.contents);

    return RESULT_OK;
}

// queue a directory for any worker to pick up; the queue owns the copy
static void queue_dir(walk_ctx_t *ctx, const char *path) {
    char *copy = strdup(path);
    if (copy == NULL) {
        log_error("[ERROR] Failed to copy a directory path (system is out of memory?); aborting.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    mutex_lock(&ctx->lock);
    DYN_ARR_APPEND(&ctx->dirs, copy);
    ++ctx->pending;
    cond_signal(&ctx->work_ready);
    mutex_unlock(&ctx->lock);
}

static result_t handle_entry(walk_ctx_t *ctx, scanner_t *scanner, const char *parent, const char *name, char *path,
                             entry_kind_t kind) {
    if (name[0] == '.' || is_blacklisted(name)) {
        return RESULT_OK;
    }

    int n = snprintf(path, PATH_MAX, "%s" PATH_SEP "%s", parent, name);
    if (n < 0 || (size_t)n >= PATH_MAX) {
        return operation_error("The file path is too long: '%s" PATH_SEP "%s'\n", parent, name);
    }

    // only classify via stat() when the directory listing couldn't
    // (e.g. DT_UNKNOWN filesystems or symlinks, which stat() follows)
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
        queue_dir(ctx, path);
        return RESULT_OK;
    }

    return scan_file(ctx->args, scanner, path, name);
}

// list one directory: scan matching files inline, queue subdirectories.
// 'scratch' is a PATH_MAX buffer owned by the calling worker.
static result_t process_dir(walk_ctx_t *ctx, scanner_t *scanner, const char *path, char *scratch) {
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

    ++scanner->dirs_scanned;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            continue;
        }

        // FindFirstFile/FindNextFile already report the entry type, so no
        // stat() is needed on this platform
        entry_kind_t kind = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? ENTRY_DIR : ENTRY_FILE;

        result = handle_entry(ctx, scanner, path, fd.cFileName, scratch, kind);
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

    ++scanner->dirs_scanned;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        entry_kind_t kind = ENTRY_UNKNOWN;

        // d_type classifies most entries straight from the directory listing,
        // saving one stat() syscall per entry; DT_UNKNOWN (filesystems that
        // don't populate d_type) and DT_LNK (stat() follows the link) fall
        // back to the stat() path in handle_entry
#if defined(DT_DIR) && defined(DT_REG)
        if (entry->d_type == DT_DIR) {
            kind = ENTRY_DIR;
        } else if (entry->d_type == DT_REG) {
            kind = ENTRY_FILE;
        }
#endif

        result = handle_entry(ctx, scanner, path, entry->d_name, scratch, kind);
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
static thread_ret_t NVI_THREAD_CALL scan_worker(void *arg) {
    scan_worker_t *worker = arg;
    walk_ctx_t *ctx = worker->ctx;

    char *scratch = malloc(PATH_MAX);
    if (scratch == NULL) {
        log_error("[ERROR] Failed to allocate a path buffer (system is out of memory?); aborting.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

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

        result_t result = process_dir(ctx, &worker->scanner, dir, scratch);
        free(dir);

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

    free(scratch);
    return 0;
}

// fold a worker's private results into the main scanner, transferring key
// ownership so no string is copied twice or freed twice
static void merge_worker_scanner(scanner_t *dst, scanner_t *src) {
    dst->dirs_scanned += src->dirs_scanned;
    dst->files_scanned += src->files_scanned;
    dst->references += src->references;

    for (size_t i = 0; i < src->envs.capacity; ++i) {
        const hashmap_entry_t *entry = &src->envs.slots[i];
        if (entry->key == NULL) {
            continue;
        }

        if (hashmap_contains(&dst->envs, entry->key, entry->len)) {
            free((void *)entry->key);
        } else {
            hashmap_append(&dst->envs, entry->key, entry->len, 0);
        }
    }

    free_hashmap(&src->envs);
}

static void append_list_keys(hashmap_t *env_map, const list_t *list) {
    for (size_t i = 0; i < list->count; ++i) {
        const char *key = list->items[i];
        hashmap_append(env_map, key, strlen(key), 0);
    }
}

void merge_required_envs(args_t *args, const scanner_t *scanner) {
    if (scanner->envs.count == 0) {
        return;
    }

    hashmap_t env_map = {0};

    append_list_keys(&env_map, &args->ignored);
    append_list_keys(&env_map, &args->required);

    for (size_t i = 0; i < scanner->envs.capacity; ++i) {
        const hashmap_entry_t *entry = &scanner->envs.slots[i];
        if (entry->key == NULL || hashmap_contains(&env_map, entry->key, entry->len)) {
            continue;
        }

        DYN_ARR_APPEND(&args->required, entry->key);
    }

    free_hashmap(&env_map);

    report_required_keys(args);
}

result_t run_scanner(args_t *args, scanner_t *scanner) {
    scanner->scan_exts = &args->scan_exts;

    report_scan_start(args);

    walk_ctx_t ctx = {0};
    ctx.args = args;
    ctx.result = RESULT_OK;
    mutex_init(&ctx.lock);
    cond_init(&ctx.work_ready);

    queue_dir(&ctx, ".");

    // dry-run stays serial so its interleaved per-file reports remain
    // deterministic; everything else scales to the core count
    size_t nthreads = cpu_count();
    if (nthreads > MAX_SCAN_THREADS) {
        nthreads = MAX_SCAN_THREADS;
    }

    if (nthreads == 1) {
        // run the worker loop on the calling thread: no spawn, same code path
        scan_worker_t worker = {.ctx = &ctx, .scanner = {.scan_exts = &args->scan_exts}};
        scan_worker(&worker);
        merge_worker_scanner(scanner, &worker.scanner);
    } else {
        scan_worker_t *workers = calloc(nthreads, sizeof(*workers));
        if (workers == NULL) {
            log_error("[ERROR] Failed to allocate scan workers (system is out of memory?); aborting.\n");
            fflush(stderr);
            exit(EXIT_FAILURE);
        }

        size_t spawned = 0;
        for (size_t i = 0; i < nthreads; ++i) {
            workers[i].ctx = &ctx;
            workers[i].scanner.scan_exts = &args->scan_exts;

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
            merge_worker_scanner(scanner, &workers[i].scanner);
        }

        free(workers);
    }

    // a failure can leave queued directories unprocessed; release them
    for (size_t i = 0; i < ctx.dirs.count; ++i) {
        free(ctx.dirs.items[i]);
    }
    free(ctx.dirs.items);

    cond_destroy(&ctx.work_ready);
    mutex_destroy(&ctx.lock);

    if (!ctx.result.ok) {
        return ctx.result;
    }

    report_scan_summary(args, scanner);

    merge_required_envs(args, scanner);

    return ctx.result;
}

void free_scanner(scanner_t *scanner) {
    for (size_t i = 0; i < scanner->envs.capacity; ++i) {
        free((void *)scanner->envs.slots[i].key);
    }
    free_hashmap(&scanner->envs);
}
