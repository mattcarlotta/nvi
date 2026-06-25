#include "scanner.h"
#include "accessors.h"
#include "arg.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "file.h"
#include "list.h"
#include "log.h"
#include "macros.h"
#include "matcher.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEP "\\"
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <dirent.h>
#include <limits.h>
#define PATH_SEP "/"
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

static const file_ext_t *get_file_ext(const file_ext_map_t *file_exts, const char *file_ext) {
    for (size_t i = 0; i < file_exts->count; ++i) {
        if (strcmp(file_exts->items[i].ext, file_ext) == 0) {
            return &file_exts->items[i];
        }
    }

    return NULL;
}

static void file_ext_append(file_ext_map_t *file_exts, const ext_entry *entry) {
    if (get_file_ext(file_exts, entry->ext) != NULL) {
        return;
    }

    file_ext_t match = {
        .ext = entry->ext,
        .accessors = entry->accessors,
        .accessor_count = entry->count,
    };

    DYN_ARR_APPEND(file_exts, match);
}

static const file_ext_t *get_file_accessors(const scanner_t *scanner, const char *name) {
    const char *dot = strrchr(name, DOT);
    if (dot == NULL || dot[1] == '\0') {
        return NULL;
    }

    return get_file_ext(&scanner->scan_exts, dot + 1);
}

static void append_unique_envs(list_t *envs, env_key_match_t *env) {
    for (size_t i = 0; i < envs->count; ++i) {
        const char *stored = envs->items[i];
        if (strlen(stored) == env->key_len && strncmp(stored, env->key, env->key_len) == 0) {
            return;
        }
    }

    char *unique_env_key = strndup(env->key, env->key_len);
    if (unique_env_key == NULL) {
        log_error("[ERROR] Failed to copy key '%s' (system is out of memory?); aborting.\n", env->key);
        abort();
    }

    DYN_ARR_APPEND(envs, unique_env_key);
}

static result_t scan_file(scanner_t *scanner, const char *path, const char *name) {
    result_t result = {.ok = true, .errcode = 0};
    const file_ext_t *file_ext_match = get_file_accessors(scanner, name);
    if (file_ext_match == NULL) {
        return result;
    }

    file_details_t file = open_file(path, scanner->dry_run);
    if (file.contents == NULL || file.len <= 0) {
        return result;
    }

    ++scanner->files_scanned;

    env_key_matches_t env_key_matches = {0};
    scan_file_content(&file, file_ext_match, &env_key_matches);

    if (scanner->dry_run && env_key_matches.count > 0) {
        log_info("[INFO]");
        log_f(" Scanned %s and found %zu key%s...\n", path, env_key_matches.count, TO_PLURAL(env_key_matches.count));

        for (size_t i = 0; i < env_key_matches.count; ++i) {
            const env_key_match_t *m = &env_key_matches.items[i];
            log_f("    \u2022 %.*s (%zu:%zu)\n", (int)m->key_len, m->key, m->line, m->byte);
        }

        log_f("\n");
    }

    for (size_t i = 0; i < env_key_matches.count; ++i) {
        ++scanner->references;
        append_unique_envs(&scanner->envs, &env_key_matches.items[i]);
    }

    free_env_key_matches(&env_key_matches);
    free(file.contents);

    return result;
}

static result_t walk_file_tree(scanner_t *scanner, const char *path);

static result_t handle_entry(scanner_t *scanner, const char *parent, const char *name) {
    if (name[0] == '.' || is_blacklisted(name)) {
        return (result_t){.ok = true};
    }

    char child[PATH_MAX];
    int n = snprintf(child, sizeof(child), "%s" PATH_SEP "%s", parent, name);
    if (n < 0 || (size_t)n >= sizeof(child)) {
        return operation_error("path too long: '%s" PATH_SEP "%s'", parent, name);
    }

    struct stat st;
    if (stat(child, &st) != 0) {
        return operation_error("cannot stat '%s'", child);
    }

    if (S_ISDIR(st.st_mode)) {
        return walk_file_tree(scanner, child);
    }

    if (S_ISREG(st.st_mode)) {
        return scan_file(scanner, child, name);
    }

    return (result_t){.ok = true};
}

static result_t walk_file_tree(scanner_t *scanner, const char *path) {
    result_t result = {.ok = true};

#ifdef _WIN32
    char pattern[PATH_MAX];
    int n = snprintf(pattern, sizeof(pattern), "%s" PATH_SEP "*", path);
    if (n < 0 || (size_t)n >= sizeof(pattern)) {
        return operation_error("path too long: '%s'", path);
    }

    WIN32_FIND_DYN_ARRTA fd;
    HANDLE h = FindFirstFile(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return operation_error("cannot open directory '%s'; aborting.", path);
    }

    ++scanner->dirs_scanned;

    do {
        result = handle_entry(scanner, path, fd.cFileName);
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
        result = handle_entry(scanner, path, entry->d_name);
        if (!result.ok) {
            break;
        }
    }

    closedir(dir);
#endif

    return result;
}

static bool is_ignored_key(const args_t *args, const char *key) {
    for (size_t i = 0; i < args->ignored.count; ++i) {
        if (strcmp(args->ignored.items[i], key) == 0) {
            return true;
        }
    }

    return false;
}

static void merge_required_envs(scanner_t *scanner, args_t *args) {
    if (scanner->envs.count == 0) {
        return;
    }

    for (size_t i = 0; i < scanner->envs.count; ++i) {
        const char *key = scanner->envs.items[i];
        if (is_ignored_key(args, key)) {
            continue;
        }

        char *new_key = strdup(key);
        if (new_key == NULL) {
            log_error("[ERROR] Failed to copy key '%s' (system out of memory?); aborting.\n", key);
            abort();
        }

        DYN_ARR_APPEND(&args->required, new_key);
    }

    if (scanner->dry_run) {
        log_info("[INFO]");
        log_f(" The following ENV keys are marked as required...\n");

        if (args->required.count > 0) {
            for (size_t i = 0; i < args->required.count; ++i) {
                log_f("    \u2022 %s\n", args->required.items[i]);
            }
        } else {
            log_f("    \u2022 (none)\n");
        }

        log_f("\n");
    }
}

result_t run_scanner(args_t *args, scanner_t *scanner) {
    result_t result = {.ok = true, .errcode = 0};
    scanner->dry_run = args->dry_run || args->command.count == 0;

    if (scanner->dry_run) {
        log_info("[INFO]");
        log_f(" Scanning for environment keys in");

        for (size_t i = 0; i < args->scan_exts.count; ++i) {
            if (i != 0 && args->scan_exts.count > 2) {
                log_f(",");
            }

            if (i == args->scan_exts.count - 1 && args->scan_exts.count > 1) {
                log_f(" and");
            }

            log_f(" *.%s", args->scan_exts.items[i]);
        }

        log_f(" files...\n\n");
    }

    for (size_t i = 0; i < args->scan_exts.count; ++i) {
        const char *ext = args->scan_exts.items[i];
        const ext_entry *entry = find_ext(ext);

        if (entry == NULL) {
            return usage_error("Unsupported scan file extension '%s'", ext);
        }

        file_ext_append(&scanner->scan_exts, entry);
    }

    result = walk_file_tree(scanner, ".");
    if (!result.ok) {
        return result;
    }

    if (scanner->dry_run) {
        log_info("[INFO]");
        log_f(" Walked %zu director%s, checked %zu file%s, and found %zu reference%s to %zu unique key%s\n\n",
              scanner->dirs_scanned, TO_PLURAL(scanner->dirs_scanned, "ies", "y"), scanner->files_scanned,
              TO_PLURAL(scanner->files_scanned), scanner->references, TO_PLURAL(scanner->references),
              scanner->envs.count, TO_PLURAL(scanner->envs.count));
    }

    merge_required_envs(scanner, args);

    return result;
}

void free_scanner(scanner_t *scanner) {
    free(scanner->scan_exts.items);
    scanner->scan_exts.items = NULL;
    scanner->scan_exts.count = 0;
    scanner->scan_exts.capacity = 0;

    for (size_t i = 0; i < scanner->envs.count; ++i) {
        free((void *)scanner->envs.items[i]);
    }

    free_list(&scanner->envs);
}
