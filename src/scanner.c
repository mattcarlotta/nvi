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

static const file_ext_t *get_file_accessors(const scanner_t *scanner, const char *name) {
    const char *dot = strrchr(name, DOT);
    if (dot == NULL || dot[1] == '\0') {
        return NULL;
    }

    return get_file_ext(scanner->scan_exts, dot + 1);
}

static void append_unique_envs(scanner_t *scanner, const env_key_match_t *env) {
    if (set_contains(&scanner->seen, env->key, env->key_len)) {
        return;
    }

    char *unique_env_key = strndup(env->key, env->key_len);
    if (unique_env_key == NULL) {
        log_error("[ERROR] Failed to copy key '%s' (system is out of memory?); aborting.\n", env->key);
        fflush(stderr);
        abort();
    }

    DYN_ARR_APPEND(&scanner->envs, unique_env_key);
    set_add(&scanner->seen, unique_env_key, env->key_len);
}

static result_t scan_file(const args_t *args, scanner_t *scanner, const char *path, const char *name) {
    result_t result = {.ok = true, .code = 0};
    const file_ext_t *file_ext_match = get_file_accessors(scanner, name);
    if (file_ext_match == NULL) {
        return result;
    }

    file_details_t file = open_file(path, args->dry_run);
    if (file.contents == NULL || file.len == 0) {
        return result;
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

    return result;
}

static result_t walk_file_tree(const args_t *args, scanner_t *scanner, const char *path);

static result_t handle_entry(const args_t *args, scanner_t *scanner, const char *parent, const char *name) {
    result_t result = {.ok = true};

    if (name[0] == '.' || is_blacklisted(name)) {
        return result;
    }

    char child[PATH_MAX];
    int n = snprintf(child, sizeof(child), "%s" PATH_SEP "%s", parent, name);
    if (n < 0 || (size_t)n >= sizeof(child)) {
        return operation_error("The file path is too long: '%s" PATH_SEP "%s'\n", parent, name);
    }

    struct stat st;
    if (stat(child, &st) != 0) {
        return operation_error("Unable to stat '%s'\n", child);
    }

    if (S_ISDIR(st.st_mode)) {
        return walk_file_tree(args, scanner, child);
    }

    if (S_ISREG(st.st_mode)) {
        return scan_file(args, scanner, child, name);
    }

    return result;
}

static result_t walk_file_tree(const args_t *args, scanner_t *scanner, const char *path) {
    result_t result = {.ok = true};

#ifdef _WIN32
    char pattern[PATH_MAX];
    int n = snprintf(pattern, sizeof(pattern), "%s" PATH_SEP "*", path);
    if (n < 0 || (size_t)n >= sizeof(pattern)) {
        return operation_error("path too long: '%s'", path);
    }

    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return operation_error("cannot open directory '%s'; aborting.", path);
    }

    ++scanner->dirs_scanned;

    do {
        result = handle_entry(args, scanner, path, fd.cFileName);
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
        result = handle_entry(args, scanner, path, entry->d_name);
        if (!result.ok) {
            break;
        }
    }

    closedir(dir);
#endif

    return result;
}

static void merge_required_envs(args_t *args, const scanner_t *scanner) {
    if (scanner->envs.count == 0) {
        return;
    }

    // Seed a set with the keys already present (user-supplied required/ignored),
    // then add each scanned key once. This keeps the merge O(scanned keys)
    // instead of doing a linear scan of the growing required list per key.
    set_t present = {0};

    for (size_t i = 0; i < args->ignored.count; ++i) {
        const char *key = args->ignored.items[i];
        size_t len = strlen(key);
        if (!set_contains(&present, key, len)) {
            set_add(&present, (char *)key, len);
        }
    }

    for (size_t i = 0; i < args->required.count; ++i) {
        const char *key = args->required.items[i];
        size_t len = strlen(key);
        if (!set_contains(&present, key, len)) {
            set_add(&present, (char *)key, len);
        }
    }

    for (size_t i = 0; i < scanner->envs.count; ++i) {
        const char *key = scanner->envs.items[i];
        size_t len = strlen(key);
        if (set_contains(&present, key, len)) {
            continue;
        }

        DYN_ARR_APPEND(&args->required, key);
        set_add(&present, (char *)key, len);
    }

    set_free(&present);

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
    result_t result = {.ok = true, .code = 0};
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
    for (size_t i = 0; i < scanner->envs.count; ++i) {
        free((void *)scanner->envs.items[i]);
    }

    set_free(&scanner->seen);
    free_list(&scanner->envs);
}
