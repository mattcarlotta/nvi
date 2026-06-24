#include "scanner.h"
#include "accessors.h"
#include "arg.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "list.h"
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

#define MAX_FILE_SIZE ((size_t)(10 * 1024 * 1024))

static const ext_pair *ext_map_get(const ext_map *map, const char *ext) {
    for (size_t i = 0; i < map->count; ++i) {
        if (strcmp(map->items[i].ext, ext) == 0) {
            return &map->items[i];
        }
    }
    return NULL;
}

static void ext_map_put(ext_map *map, const ext_entry *entry) {
    if (ext_map_get(map, entry->ext) != NULL) {
        return;
    }

    ext_pair pair = {
        .ext = entry->ext,
        .accessors = entry->accessors,
        .accessor_count = entry->count,
    };
    DYN_ARR_APPEND(map, pair);
}

static const ext_pair *get_file_accessors(const scanner_t *scanner, const char *name) {
    const char *dot = strrchr(name, DOT);
    if (dot == NULL || dot[1] == '\0') {
        return NULL;
    }

    return ext_map_get(&scanner->scan_exts, dot + 1);
}

static bool envs_are_unique(list_t *envs, const char *key, size_t key_len) {
    for (size_t i = 0; i < envs->count; ++i) {
        const char *stored = envs->items[i];
        if (strlen(stored) == key_len && strncmp(stored, key, key_len) == 0) {
            return true;
        }
    }

    char *copy = strndup(key, key_len);
    if (copy == NULL) {
        return false;
    }

    DYN_ARR_APPEND(envs, copy);

    return true;
}

static result_t scan_file(scanner_t *scanner, const char *path, const char *name) {
    const ext_pair *match = get_file_accessors(scanner, name);
    if (match == NULL) {
        return (result_t){.ok = true};
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "warning: cannot open '%s': %s\n", path, strerror(errno));
        return (result_t){.ok = true};
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return (result_t){.ok = true};
    }

    if (size > MAX_FILE_SIZE) {
        if (scanner->dry_run) {
            fprintf(stderr, "warning: '%s' exceeds %zu bytes, skipping.\n", path, MAX_FILE_SIZE);
        }

        fclose(f);

        return (result_t){.ok = true};
    }

    char *contents = malloc(size + 1);
    if (contents == NULL) {
        fclose(f);
        abort();
    }

    size_t read = fread(contents, 1, size, f);
    contents[read] = '\0';
    fclose(f);

    ++scanner->files_scanned;

    env_matches_t matches = {0};
    scan_file_content(contents, read, match->accessors, match->accessor_count, &matches);

    if (scanner->dry_run && matches.count > 0) {
        fprintf(stderr, "[INFO] Scanned %s and found %zu key%s...\n", path, matches.count,
                ISPLURAL(matches.count) ? "s" : "");

        for (size_t i = 0; i < matches.count; ++i) {
            const env_match_t *m = &matches.items[i];
            fprintf(stderr, "    \u2022 %.*s (line %zu, byte %zu)\n", (int)m->key_len, m->key, m->line, m->byte);
        }

        fprintf(stderr, "\n");
    }

    for (size_t i = 0; i < matches.count; ++i) {
        ++scanner->references;
        if (!envs_are_unique(&scanner->envs, matches.items[i].key, matches.items[i].key_len)) {
            free_env_matches(&matches);
            free(contents);
            abort();
        }
    }

    free_env_matches(&matches);
    free(contents);

    return (result_t){.ok = true};
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
    result_t res = {.ok = true};

#ifdef _WIN32
    char pattern[PATH_MAX];
    int n = snprintf(pattern, sizeof(pattern), "%s" PATH_SEP "*", path);
    if (n < 0 || (size_t)n >= sizeof(pattern)) {
        return operation_error("path too long: '%s'", path);
    }

    WIN32_FIND_DYN_ARRTA fd;
    HANDLE h = FindFirstFile(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return operation_error("cannot open directory '%s'", path);
    }

    ++scanner->dirs_scanned;

    do {
        res = handle_entry(scanner, path, fd.cFileName);
        if (!res.ok) {
            break;
        }
    } while (FindNextFile(h, &fd));
    FindClose(h);
#else
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return operation_error("cannot open directory '%s'", path);
    }

    ++scanner->dirs_scanned;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        res = handle_entry(scanner, path, entry->d_name);
        if (!res.ok) {
            break;
        }
    }

    closedir(dir);
#endif

    return res;
}

static bool is_ignored_key(const args_t *args, const char *key) {
    for (size_t i = 0; i < args->ignored.count; ++i) {
        if (strcmp(args->ignored.items[i], key) == 0) {
            return true;
        }
    }

    return false;
}

static result_t merge_required_envs(scanner_t *scanner, args_t *args) {
    if (scanner->envs.count == 0) {
        return (result_t){.ok = true};
    }

    bool printed_header = false;

    for (size_t i = 0; i < scanner->envs.count; ++i) {
        const char *key = scanner->envs.items[i];
        if (is_ignored_key(args, key)) {
            continue;
        }

        char *new_key = strdup(key);
        if (new_key == NULL) {
            fprintf(stderr, "[ERROR] Failed to copy key '%s' from file; aborting.\n", key);
            abort();
        }

        DYN_ARR_APPEND(&args->required, new_key);

        if (scanner->dry_run) {
            if (!printed_header) {
                fprintf(stderr, "[INFO] The following ENV keys would be marked as required...\n");
                printed_header = true;
            }
            fprintf(stderr, "  \u2022 %s\n", new_key);
        }
    }

    if (scanner->dry_run && printed_header) {
        fprintf(stderr, "\n");
    }

    return (result_t){.ok = true};
}

result_t run_scanner(args_t *args, scanner_t *scanner) {
    scanner->dry_run = args->dry_run || args->command.count == 0;

    if (scanner->dry_run) {
        fprintf(stderr, "[INFO] Scanning for environment variables in ");
        for (size_t i = 0; i < args->scan_exts.count; ++i) {
            if (i != 0) {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "*.%s", args->scan_exts.items[i]);
        }
        fprintf(stderr, " files...\n\n");
    }

    for (size_t i = 0; i < args->scan_exts.count; ++i) {
        const char *ext = args->scan_exts.items[i];
        const ext_entry *entry = find_ext(ext);

        if (entry == NULL) {
            return usage_error("unsupported scan file extension '%s'", ext);
        }

        ext_map_put(&scanner->scan_exts, entry);
    }

    result_t walk_result = walk_file_tree(scanner, ".");
    if (!walk_result.ok) {
        return walk_result;
    }

    if (scanner->dry_run) {
        fprintf(stderr,
                "[INFO] Walked %zu director%s, checked %zu file%s, and found %zu reference%s to %zu unique key%s\n\n",
                scanner->dirs_scanned, ISPLURAL(scanner->dirs_scanned) ? "ies" : "y", scanner->files_scanned,
                ISPLURAL(scanner->files_scanned) ? "s" : "", scanner->references,
                ISPLURAL(scanner->references) ? "s" : "", scanner->envs.count, scanner->envs.count != 1 ? "s" : "");
    }

    return merge_required_envs(scanner, args);
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
