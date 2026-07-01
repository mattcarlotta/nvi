#define NOB_IMPLEMENTATION
#include "nob.h"
#include <sys/stat.h>

#define OUT "nvi"

#ifdef _WIN32
#define OUT_BIN OUT ".exe"
#define BIN_EXT ".exe"
#else
#define OUT_BIN OUT
#define BIN_EXT ""
#endif

static const char *git_commit(void) {
    static const char *cached = NULL;
    if (cached != NULL) {
        return cached;
    }

    const char *env = getenv("NVI_COMMIT");
    if (env != NULL && env[0] != '\0') {
        cached = env;
        return cached;
    }

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "git", "rev-parse", "--short", "HEAD");
    if (!nob_cmd_run(&cmd, .stdout_path = ".nvi_commit")) {
        cached = "unknown";
        return cached;
    }

    Nob_String_Builder sb = {0};
    bool read_ok = nob_read_entire_file(".nvi_commit", &sb);
    nob_delete_file(".nvi_commit");
    if (!read_ok) {
        cached = "unknown";
        return cached;
    }

    while (sb.count > 0 && (sb.items[sb.count - 1] == '\n' || sb.items[sb.count - 1] == '\r')) {
        --sb.count;
    }
    nob_sb_append_null(&sb);

    cached = sb.items;
    return cached;
}

static void add_common_flags(Nob_Cmd *cmd, const char *build_label) {
    const char *commit_def = nob_temp_sprintf("-DNVI_COMMIT=\"%s\"", git_commit());
    const char *build_def = nob_temp_sprintf("-DNVI_BUILD=\"%s\"", build_label);

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(cmd, "cl", "/nologo", "/Isrc", commit_def, build_def, "/W4", "/std:c17");
#elif defined(__APPLE__) || defined(__linux__)
    nob_cmd_append(cmd, "clang", "-Isrc", commit_def, build_def, "-Wformat-security", "-Wall", "-Wextra", "-Wpedantic",
                   "-std=gnu17");
#else
#error "unsupported platform (expected Windows/MSVC, macOS, or Linux)"
#endif
}

static void compose_dev_cmd(Nob_Cmd *cmd) {
    add_common_flags(cmd, "debug");

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(cmd, "/Zi", "/Od", "/Fe:" OUT_BIN);
#else
    nob_cmd_append(cmd, "-g", "-O0", "-o", OUT_BIN);
#endif
}

static void compose_test_cmd(Nob_Cmd *cmd, const char *out_path) {
    add_common_flags(cmd, "test");

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(cmd, "/Itests/unity", "/Zi", "/Od", nob_temp_sprintf("/Fe:%s", out_path));
#else
    nob_cmd_append(cmd, "-Itests/unity", "-g", "-O0");
#if defined(__APPLE__) || defined(__linux__)
    // Sanitize unit tests on the dev platforms; MSVC is left unsanitized.
    nob_cmd_append(cmd, "-fsanitize=address,undefined", "-fno-omit-frame-pointer");
#endif
    nob_cmd_append(cmd, "-o", out_path);
#endif
}

static bool collect_sources_except(const char *except, Nob_File_Paths *out) {
    Nob_File_Paths paths = {0};
    if (!nob_read_entire_dir("src", &paths)) {
        return false;
    }

    for (size_t i = 0; i < paths.count; ++i) {
        const char *name = paths.items[i];

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        size_t len = strlen(name);
        if (len < 2 || strcmp(name + len - 2, ".c") != 0) {
            continue;
        }

        if (except != NULL && strcmp(name, except) == 0) {
            continue;
        }

        nob_da_append(out, nob_temp_sprintf("src/%s", name));
    }

    nob_da_free(paths);
    return true;
}

static bool append_sources_except(Nob_Cmd *cmd, const char *except) {
    Nob_File_Paths srcs = {0};
    if (!collect_sources_except(except, &srcs)) {
        return false;
    }

    for (size_t i = 0; i < srcs.count; ++i) {
        nob_cmd_append(cmd, srcs.items[i]);
    }

    nob_da_free(srcs);
    return true;
}

static bool append_sources(Nob_Cmd *cmd) { return append_sources_except(cmd, NULL); }

static bool collect_test_names(Nob_File_Paths *out) {
    Nob_File_Paths paths = {0};
    if (!nob_read_entire_dir("tests", &paths)) {
        return false;
    }

    for (size_t i = 0; i < paths.count; ++i) {
        const char *name = paths.items[i];

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        size_t len = strlen(name);
        if (len < 2 || strcmp(name + len - 2, ".c") != 0) {
            continue;
        }

        nob_da_append(out, nob_temp_strdup(name));
    }

    nob_da_free(paths);
    return true;
}

static bool delete_entry(Nob_Walk_Entry e) { return nob_delete_file(e.path); }

static void remove_dir_recursively(const char *path) {
    if (nob_get_file_type(path) != NOB_FILE_DIRECTORY) {
        return;
    }
    nob_walk_dir(path, delete_entry, .post_order = true);
}

static bool build_dev(void) {
    Nob_Cmd cmd = {0};
    compose_dev_cmd(&cmd);

    if (!append_sources(&cmd)) {
        return false;
    }

    return nob_cmd_run(&cmd);
}

static bool build_release(void) {
    Nob_Cmd cmd = {0};
    add_common_flags(&cmd, "release");

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(&cmd, "/O1", "/GL", "/Fe:" OUT_BIN);
#elif defined(__APPLE__)
    nob_cmd_append(&cmd, "-Oz", "-flto", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
                   "-Wl,-dead_strip_dylibs", "-Wl,-dead_strip", "-Wl,-x", "-o", OUT_BIN);
#elif defined(__linux__)
    const char *mps = getenv("NVI_MAX_PAGE_SIZE");
    // Page sizing breakdown:
    // 0x1000 = 4096 = 4KB
    // 0x4000 = 16384 = 16KB
    // 0x10000 = 65536 = 64KB (default)
    if (mps == NULL || mps[0] == '\0') {
        mps = "0x10000";
    }
    nob_cmd_append(&cmd, "-Oz", "-flto", "-fno-ident", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
                   "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections", "-Wl,-z,noseparate-code",
                   "-Wl,--build-id=none", nob_temp_sprintf("-Wl,-z,max-page-size=%s", mps), "-s", "-o", OUT_BIN);
#endif

    if (!append_sources(&cmd)) {
        return false;
    }

    if (!nob_cmd_run(&cmd)) {
        return false;
    }

#ifdef __APPLE__
    nob_cmd_append(&cmd, "strip", "-rSTx", OUT_BIN);
    if (!nob_cmd_run(&cmd)) {
        return false;
    }
#endif

    return true;
}

static bool build_one_test(const char *test_src, const char *out_path) {
    Nob_Cmd cmd = {0};
    compose_test_cmd(&cmd, out_path);

    nob_cmd_append(&cmd, test_src, "tests/unity/unity.c");

    if (!append_sources_except(&cmd, "main.c")) {
        return false;
    }

    return nob_cmd_run(&cmd);
}

static bool run_tests(void) {
    if (!nob_mkdir_if_not_exists("build") || !nob_mkdir_if_not_exists("build/tests")) {
        return false;
    }

    Nob_File_Paths tests = {0};
    if (!collect_test_names(&tests)) {
        return false;
    }

    size_t total = 0;
    size_t passed = 0;

    for (size_t i = 0; i < tests.count; ++i) {
        const char *name = tests.items[i];
        size_t len = strlen(name);

        const char *stem = nob_temp_sprintf("%.*s", (int)(len - 2), name);
        const char *src = nob_temp_sprintf("tests/%s", name);
        const char *out = nob_temp_sprintf("build/tests/%s" BIN_EXT, stem);

        ++total;

        if (!build_one_test(src, out)) {
            nob_log(NOB_ERROR, "failed to build %s", stem);
            continue;
        }

        Nob_Cmd run = {0};
        nob_cmd_append(&run, out);
        if (nob_cmd_run(&run)) {
            ++passed;
        } else {
            nob_log(NOB_ERROR, "%s reported failures", stem);
        }
    }

    nob_da_free(tests);

    nob_log(passed == total ? NOB_INFO : NOB_ERROR, "test suites: %zu/%zu passed", passed, total);
    return passed == total;
}

static int cmp_cstr(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

static void json_str(Nob_String_Builder *sb, const char *s) {
    nob_da_append(sb, '"');
    for (const char *p = s; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        switch (c) {
            case '"':
                nob_sb_append_cstr(sb, "\\\"");
                break;
            case '\\':
                nob_sb_append_cstr(sb, "\\\\");
                break;
            case '\b':
                nob_sb_append_cstr(sb, "\\b");
                break;
            case '\f':
                nob_sb_append_cstr(sb, "\\f");
                break;
            case '\n':
                nob_sb_append_cstr(sb, "\\n");
                break;
            case '\r':
                nob_sb_append_cstr(sb, "\\r");
                break;
            case '\t':
                nob_sb_append_cstr(sb, "\\t");
                break;
            default:
                if (c < 0x20) {
                    nob_sb_appendf(sb, "\\u%04x", c);
                } else {
                    nob_da_append(sb, (char)c);
                }
        }
    }
    nob_da_append(sb, '"');
}

static void emit_entry(Nob_String_Builder *json, bool *first, const char *dir, const Nob_Cmd *prefix,
                       const char *output, const char *src) {
    if (!*first) {
        nob_sb_append_cstr(json, ",\n");
    }
    *first = false;

    nob_sb_append_cstr(json, "  {\n    \"directory\": ");
    json_str(json, dir);
    nob_sb_append_cstr(json, ",\n    \"file\": ");
    json_str(json, src);
    nob_sb_append_cstr(json, ",\n    \"output\": ");
    json_str(json, output);
    nob_sb_append_cstr(json, ",\n    \"arguments\": [");
    for (size_t i = 0; i < prefix->count; ++i) {
        if (i) {
            nob_sb_append_cstr(json, ", ");
        }
        json_str(json, prefix->items[i]);
    }
    if (prefix->count) {
        nob_sb_append_cstr(json, ", ");
    }
    json_str(json, src);
    nob_sb_append_cstr(json, "]\n  }");
}

static bool cmd_generate(void) {
    const char *dir = nob_get_current_dir_temp();

    Nob_String_Builder json = {0};
    nob_sb_append_cstr(&json, "[\n");
    bool first = true;
    size_t count = 0;

    {
        Nob_Cmd prefix = {0};
        compose_dev_cmd(&prefix);

        Nob_File_Paths srcs = {0};
        if (!collect_sources_except(NULL, &srcs)) {
            nob_cmd_free(prefix);
            nob_sb_free(json);
            return false;
        }
        qsort(srcs.items, srcs.count, sizeof(*srcs.items), cmp_cstr);

        for (size_t i = 0; i < srcs.count; ++i) {
            emit_entry(&json, &first, dir, &prefix, OUT_BIN, srcs.items[i]);
            ++count;
        }

        nob_da_free(srcs);
        nob_cmd_free(prefix);
    }

    Nob_File_Paths lib = {0};
    if (!collect_sources_except("main.c", &lib)) {
        nob_sb_free(json);
        return false;
    }
    qsort(lib.items, lib.count, sizeof(*lib.items), cmp_cstr);

    Nob_File_Paths tests = {0};
    if (!collect_test_names(&tests)) {
        nob_da_free(lib);
        nob_sb_free(json);
        return false;
    }
    qsort(tests.items, tests.count, sizeof(*tests.items), cmp_cstr);

    for (size_t t = 0; t < tests.count; ++t) {
        const char *name = tests.items[t];
        size_t len = strlen(name);
        const char *stem = nob_temp_sprintf("%.*s", (int)(len - 2), name);
        const char *src = nob_temp_sprintf("tests/%s", name);
        const char *out = nob_temp_sprintf("build/tests/%s" BIN_EXT, stem);

        Nob_Cmd prefix = {0};
        compose_test_cmd(&prefix, out);

        emit_entry(&json, &first, dir, &prefix, out, src);
        ++count;
        emit_entry(&json, &first, dir, &prefix, out, "tests/unity/unity.c");
        ++count;
        for (size_t i = 0; i < lib.count; ++i) {
            emit_entry(&json, &first, dir, &prefix, out, lib.items[i]);
            ++count;
        }

        nob_cmd_free(prefix);
    }

    nob_da_free(tests);
    nob_da_free(lib);

    nob_sb_append_cstr(&json, "\n]\n");

    bool ok = nob_write_entire_file("compile_commands.json", json.items, json.count);
    if (ok) {
        nob_log(NOB_INFO, "wrote compile_commands.json (%zu entries)", count);
    }

    nob_sb_free(json);
    return ok;
}

static bool timed(const char *label, const char *out, bool (*build)(void)) {
    uint64_t start = nob_nanos_since_unspecified_epoch();
    bool ok = build();
    uint64_t elapsed = nob_nanos_since_unspecified_epoch() - start;

    nob_log(ok ? NOB_INFO : NOB_ERROR, "%s build %s in %.3f s", label, ok ? "succeeded" : "failed",
            (double)elapsed / NOB_NANOS_PER_SEC);

    if (ok) {
        struct stat st;
        if (stat(out, &st) != 0) {
            nob_log(NOB_WARNING, "could not stat %s for size", out);
            return false;
        }
        nob_log(NOB_INFO, "%s is %.1f KB (%lld bytes)", out, (double)st.st_size / 1024.0, (long long)st.st_size);
    }

    return ok;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    // drop program name
    nob_shift(argv, argc);

    const char *subcmd = argc > 0 ? nob_shift(argv, argc) : "dev";

    if (strcmp(subcmd, "install") == 0) {
        const char *dir = argc > 0 ? nob_shift(argv, argc) : NULL;
        if (dir == NULL || dir[0] == '\0') {
            nob_log(NOB_ERROR, "installation directory was not set; run: ./nob install <path>");
            return 1;
        }

        if (!timed("release", OUT_BIN, build_release)) {
            return 1;
        }

        const char *dest = nob_temp_sprintf("%s/%s", dir, OUT_BIN);

        if (!nob_copy_file(OUT_BIN, dest)) {
            nob_log(NOB_ERROR, "failed to copy %s -> %s (permission denied? try a directory owned by you)", OUT_BIN,
                    dest);
            return 1;
        }

        nob_log(NOB_INFO, "installed %s to %s", OUT_BIN, dest);
    } else if (strcmp(subcmd, "release") == 0) {
        if (!timed("release", OUT_BIN, build_release)) {
            return 1;
        }
    } else if (strcmp(subcmd, "test") == 0) {
        if (!cmd_generate()) {
            return 1;
        }

        if (!run_tests()) {
            return 1;
        }
    } else if (strcmp(subcmd, "generate") == 0) {
        if (!cmd_generate()) {
            return 1;
        }
    } else if (strcmp(subcmd, "clean") == 0) {
        nob_delete_file(OUT_BIN);
        nob_delete_file("compile_commands.json");
        remove_dir_recursively("build/tests");
        remove_dir_recursively("build");
#ifdef __APPLE__
        remove_dir_recursively(OUT_BIN ".dSYM");
#endif
    } else {
        if (!timed("dev", OUT_BIN, build_dev)) {
            return 1;
        }
    }

    return 0;
}
