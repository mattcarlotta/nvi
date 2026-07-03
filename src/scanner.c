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
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include "shims.h"
#include <windows.h>
#define PATH_SEP "\\"
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <dirent.h>
#include <limits.h>
#define PATH_SEP "/"
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

static inline const file_ext_t *get_file_accessors(const file_ext_map_t *map, const char *name) {
    const char *dot = strrchr(name, DOT);
    if (dot == NULL || dot[1] == '\0') {
        return NULL;
    }

    return get_file_ext(map, dot + 1);
}

static inline void append_unique_envs(scanner_t *scanner, const env_key_match_t *env) {
    if (hashmap_contains(&scanner->envs, env->key, env->key_len)) {
        return;
    }

    const char *new_key = strndup(env->key, env->key_len);
    if (new_key == NULL) {
        log_error("[ERROR] Failed to copy key '%s' (system is out of memory?); aborting.\n", env->key);
        fflush(stderr);
        abort();
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
        if (args->dry_run) {
            log_warning("[WARNING] The file '%s' appears to be empty; skipping.\n\n", path);
        }
        free(file.contents);
        return RESULT_OK;
    }

    ++scanner->files_scanned;

    env_key_matches_t env_key_matches = {0};
    scan_file_content(&file, file_ext_match, &env_key_matches);

    if (args->dry_run && env_key_matches.count > 0) {
        log_info("[INFO]");
        log_f(" Scanned ");
        log_fi("%s", path);
        log_f(" and found %zu key%s...\n", env_key_matches.count, TO_PLURAL(env_key_matches.count));

        for (size_t i = 0; i < env_key_matches.count; ++i) {
            const env_key_match_t *m = &env_key_matches.items[i];
            log_f("    \u2022 ");
            log_bold_info("%.*s", (int)m->key_len, m->key);
            log_comment(" [%zu:%zu]\n", m->line, m->byte);
        }

        log_f("\n");
    }

    for (size_t i = 0; i < env_key_matches.count; ++i) {
        ++scanner->references;
        append_unique_envs(scanner, &env_key_matches.items[i]);
    }

    free_env_key_matches(&env_key_matches);
    free(file.contents);

    return RESULT_OK;
}

static result_t walk_file_tree(const args_t *args, scanner_t *scanner, const char *path);

// 'file' is a caller-owned scratch buffer of at least PATH_MAX bytes; keeping
// it on the heap (one allocation per directory level) keeps recursion frames
// small so deep trees can't blow the stack
static result_t handle_entry(const args_t *args, scanner_t *scanner, const char *parent, const char *name, char *file) {
    if (name[0] == '.' || is_blacklisted(name)) {
        return RESULT_OK;
    }

    int n = snprintf(file, PATH_MAX, "%s" PATH_SEP "%s", parent, name);
    if (n < 0 || (size_t)n >= PATH_MAX) {
        return operation_error("The file path is too long: '%s" PATH_SEP "%s'\n", parent, name);
    }

    struct stat st;
#if defined(_WIN32) && defined(_MSC_VER)
    if (stat(file, &st) != 0) {
        return operation_error("Unable to stat '%s'\n", file);
    }
#else
    if (lstat(file, &st) != 0) {
        return operation_error("Unable to stat '%s'\n", file);
    }
#endif

    if (S_ISDIR(st.st_mode)) {
        return walk_file_tree(args, scanner, file);
    }

    if (S_ISREG(st.st_mode)) {
        return scan_file(args, scanner, file, name);
    }

    return RESULT_OK;
}

static result_t walk_file_tree(const args_t *args, scanner_t *scanner, const char *path) {
    result_t result = RESULT_OK;

    char *file = malloc(PATH_MAX);
    if (file == NULL) {
        fprintf(stderr, "[ERROR] Failed to allocate a path buffer (system out of memory?); aborting.\n");
        fflush(stderr);
        abort();
    }

#if defined(_WIN32) && defined(_MSC_VER)
    // build the search pattern in the scratch buffer, then reuse it for fileren
    int n = snprintf(file, PATH_MAX, "%s" PATH_SEP "*", path);
    if (n < 0 || (size_t)n >= PATH_MAX) {
        free(file);
        return operation_error("path too long: '%s'", path);
    }

    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(file, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        free(file);
        return operation_error("cannot open directory '%s'; aborting.", path);
    }

    ++scanner->dirs_scanned;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            continue;
        }

        result = handle_entry(args, scanner, path, fd.cFileName, file);
        if (!result.ok) {
            break;
        }
    } while (FindNextFile(h, &fd));
    FindClose(h);
#else
    DIR *dir = opendir(path);
    if (dir == NULL) {
        free(file);
        return operation_error("cannot open directory '%s'; aborting.", path);
    }

    ++scanner->dirs_scanned;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        result = handle_entry(args, scanner, path, entry->d_name, file);
        if (!result.ok) {
            break;
        }
    }

    closedir(dir);
#endif

    free(file);
    return result;
}

static void add_unique_env_keys(hashmap_t *env_map, const list_t *list) {
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

    add_unique_env_keys(&env_map, &args->ignored);
    add_unique_env_keys(&env_map, &args->required);

    for (size_t i = 0; i < scanner->envs.capacity; ++i) {
        const hashmap_entry_t *entry = &scanner->envs.slots[i];
        if (entry->key == NULL || hashmap_contains(&env_map, entry->key, entry->len)) {
            continue;
        }

        DYN_ARR_APPEND(&args->required, entry->key);
    }

    free_hashmap(&env_map);

    if (args->dry_run) {
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
}

result_t run_scanner(args_t *args, scanner_t *scanner) {
    result_t result = RESULT_OK;
    scanner->scan_exts = &args->scan_exts;

    if (args->dry_run) {
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

    result = walk_file_tree(args, scanner, ".");
    if (!result.ok) {
        return result;
    }

    if (args->dry_run) {
        log_info("[INFO]");
        log_f(" Walked %zu director%s, scanned %zu file%s, and found %zu reference%s to %zu unique key%s\n\n",
              scanner->dirs_scanned, TO_PLURAL(scanner->dirs_scanned, "ies", "y"), scanner->files_scanned,
              TO_PLURAL(scanner->files_scanned), scanner->references, TO_PLURAL(scanner->references),
              scanner->envs.count, TO_PLURAL(scanner->envs.count));
    }

    merge_required_envs(args, scanner);

    return result;
}

void free_scanner(scanner_t *scanner) {
    for (size_t i = 0; i < scanner->envs.capacity; ++i) {
        free((void *)scanner->envs.slots[i].key);
    }
    free_hashmap(&scanner->envs);
}
