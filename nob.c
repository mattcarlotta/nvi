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
    const char *env = getenv("NVI_COMMIT");
    if (env != NULL && env[0] != '\0') {
        return env;
    }

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "git", "rev-parse", "--short", "HEAD");
    if (!nob_cmd_run(&cmd, .stdout_path = ".nvi_commit")) {
        return "unknown";
    }

    Nob_String_Builder sb = {0};
    bool read_ok = nob_read_entire_file(".nvi_commit", &sb);
    nob_delete_file(".nvi_commit");
    if (!read_ok) {
        return "unknown";
    }

    while (sb.count > 0 && (sb.items[sb.count - 1] == '\n' || sb.items[sb.count - 1] == '\r')) {
        --sb.count;
    }
    nob_sb_append_null(&sb);

    return sb.items;
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

// Appends every src/*.c to the command. When `except` is non-NULL, the file
// with that base name (e.g. "main.c") is skipped, which lets the test build
// link the library code without colliding with the binary's own main().
static bool append_sources_except(Nob_Cmd *cmd, const char *except) {
    Nob_File_Paths paths = {0};
    if (!nob_read_entire_dir("src", &paths)) {
        return false;
    }

    for (size_t i = 0; i < paths.count; i++) {
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

        nob_cmd_append(cmd, nob_temp_sprintf("src/%s", name));
    }

    nob_da_free(paths);
    return true;
}

static bool append_sources(Nob_Cmd *cmd) { return append_sources_except(cmd, NULL); }

static bool delete_entry(Nob_Walk_Entry e) { return nob_delete_file(e.path); }

static void remove_dir_recursively(const char *path) {
    if (nob_get_file_type(path) != NOB_FILE_DIRECTORY) {
        return;
    }
    nob_walk_dir(path, delete_entry, .post_order = true);
}

static bool build_dev(void) {
    Nob_Cmd cmd = {0};
    add_common_flags(&cmd, "debug");

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(&cmd, "/Zi", "/Od", "/Fe:" OUT_BIN);
#else
    nob_cmd_append(&cmd, "-g", "-O0", "-o", OUT_BIN);
#endif

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
    nob_cmd_append(&cmd, "-Oz", "-flto", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-Wl,-dead_strip",
                   "-Wl,-x", "-o", OUT_BIN);
#elif defined(__linux__)
    nob_cmd_append(&cmd, "-Oz", "-flto", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-ffunction-sections",
                   "-fdata-sections", "-Wl,--gc-sections", "-s", "-o", OUT_BIN);
#endif

    if (!append_sources(&cmd)) {
        return false;
    }

    if (!nob_cmd_run(&cmd)) {
        return false;
    }

#ifdef __APPLE__
    nob_cmd_append(&cmd, "strip", "-x", OUT_BIN);
    if (!nob_cmd_run(&cmd)) {
        return false;
    }
#endif

    return true;
}

// Builds a single Unity test executable: the test source + the vendored Unity
// runtime + every src/*.c except main.c. The compiler and base flags come from
// add_common_flags() so the platform handling matches the dev/release builds.
static bool build_one_test(const char *test_src, const char *out_path) {
    Nob_Cmd cmd = {0};
    add_common_flags(&cmd, "test");

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(&cmd, "/Isrc/tests/unity", "/Zi", "/Od", nob_temp_sprintf("/Fe:%s", out_path));
#else
    nob_cmd_append(&cmd, "-Itests/unity", "-g", "-O0");
#if defined(__APPLE__) || defined(__linux__)
    // Sanitize unit tests on the dev platforms; MSVC is left unsanitized.
    nob_cmd_append(&cmd, "-fsanitize=address,undefined", "-fno-omit-frame-pointer");
#endif
    nob_cmd_append(&cmd, "-o", out_path);
#endif

    nob_cmd_append(&cmd, test_src, "tests/unity/unity.c");

    if (!append_sources_except(&cmd, "main.c")) {
        return false;
    }

    return nob_cmd_run(&cmd);
}

// Discovers tests/test_*.c, builds each into build/tests/, runs them, and
// aggregates the result. Each suite returns its failure count as an exit code,
// so a nonzero exit (caught by nob_cmd_run) marks the suite as failed.
static bool run_tests(void) {
    if (!nob_mkdir_if_not_exists("build") || !nob_mkdir_if_not_exists("build/tests")) {
        return false;
    }

    Nob_File_Paths paths = {0};
    if (!nob_read_entire_dir("tests", &paths)) {
        return false;
    }

    size_t total = 0;
    size_t passed = 0;

    for (size_t i = 0; i < paths.count; ++i) {
        const char *name = paths.items[i];
        size_t len = strlen(name);

        if (strncmp(name, "test_", 5) != 0 || len < 3 || strcmp(name + len - 2, ".c") != 0) {
            continue;
        }

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

    nob_da_free(paths);

    nob_log(passed == total ? NOB_INFO : NOB_ERROR, "test suites: %zu/%zu passed", passed, total);
    return passed == total;
}

static bool install_binary(void) {
    const char *dir = getenv("DIR");
    if (dir == NULL || dir[0] == '\0') {
        nob_log(NOB_ERROR, "PREFIX is not set; run: DIR=/path/to/bin ./nob install");
        return false;
    }

    const char *dest = nob_temp_sprintf("%s/%s", dir, OUT_BIN);

    if (!nob_copy_file(OUT_BIN, dest)) {
        nob_log(NOB_ERROR, "failed to copy %s -> %s (permission denied? try a writable PREFIX)", OUT_BIN, dest);
        return false;
    }

    nob_log(NOB_INFO, "installed %s to %s", OUT_BIN, dest);
    return true;
}

static void log_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        nob_log(NOB_WARNING, "could not stat %s for size", path);
        return;
    }
    nob_log(NOB_INFO, "%s is %.1f KB (%lld bytes)", path, (double)st.st_size / 1024.0, (long long)st.st_size);
}

static bool timed(const char *label, const char *out, bool (*build)(void)) {
    uint64_t start = nob_nanos_since_unspecified_epoch();
    bool ok = build();
    uint64_t elapsed = nob_nanos_since_unspecified_epoch() - start;

    nob_log(ok ? NOB_INFO : NOB_ERROR, "%s build %s in %.3f s", label, ok ? "succeeded" : "failed",
            (double)elapsed / NOB_NANOS_PER_SEC);

    if (ok) {
        log_file_size(out);
    }
    return ok;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    // drop program name
    nob_shift(argv, argc);

    const char *subcmd = argc > 0 ? nob_shift(argv, argc) : "dev";

    if (strcmp(subcmd, "install") == 0) {
        if (!timed("release", OUT_BIN, build_release)) {
            return 1;
        }

        if (!timed("install", OUT_BIN, install_binary)) {
            return 1;
        }
    } else if (strcmp(subcmd, "release") == 0) {
        if (!timed("release", OUT_BIN, build_release)) {
            return 1;
        }
    } else if (strcmp(subcmd, "test") == 0) {
        if (!run_tests()) {
            return 1;
        }
    } else if (strcmp(subcmd, "clean") == 0) {
        nob_delete_file(OUT_BIN);
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
