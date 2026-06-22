#define NOB_IMPLEMENTATION
#include "nob.h"

#define SRC_FILES "src/main.c", "src/arg.c"
#define OUT "nvi"

#ifdef _WIN32
#define OUT_BIN OUT ".exe"
#else
#define OUT_BIN OUT
#endif

static bool build_dev(void) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "clang", "-Isrc", "-Wall", "-Wextra", "-Wpedantic", "-std=c17", "-g", "-O0", "-o", OUT,
                   SRC_FILES);
    return nob_cmd_run(&cmd);
}

static bool delete_entry(Nob_Walk_Entry e) { return nob_delete_file(e.path); }

static void remove_dir_recursively(const char *path) {
    if (nob_get_file_type(path) != NOB_FILE_DIRECTORY)
        return;
    nob_walk_dir(path, delete_entry, .post_order = true);
}

static bool build_release(void) {
    Nob_Cmd cmd = {0};

#if defined(_WIN32) && defined(_MSC_VER)
    nob_cmd_append(&cmd, "cl", "/nologo", "/Isrc", "/W4", "/std:c17", "/O1", "/GL", "/Fe:" OUT_BIN, SRC_FILES, "/link",
                   "/LTCG");
#elif defined(__APPLE__)
    nob_cmd_append(&cmd, "clang", "-Isrc", "-Wall", "-Wextra", "-Wpedantic", "-std=c17", "-Oz", "-flto",
                   "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-Wl,-dead_strip", "-Wl,-x", "-o", OUT_BIN,
                   SRC_FILES);
#elif defined(__linux__)
    nob_cmd_append(&cmd, "clang", "-Isrc", "-Wall", "-Wextra", "-Wpedantic", "-std=c17", "-Oz", "-flto",
                   "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-ffunction-sections", "-fdata-sections",
                   "-Wl,--gc-sections", "-s", "-o", OUT_BIN, SRC_FILES);
#else
#error "build_release: unsupported platform (expected Windows/MSVC, macOS, or Linux)"
#endif

    if (!nob_cmd_run(&cmd))
        return false;

#ifdef __APPLE__
    nob_cmd_append(&cmd, "strip", "-x", OUT_BIN);
    if (!nob_cmd_run(&cmd))
        return false;
#endif

    return true;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    // drop program name
    nob_shift(argv, argc);

    const char *subcmd = argc > 0 ? nob_shift(argv, argc) : "dev";

    if (strcmp(subcmd, "release") == 0) {
        if (!build_release())
            return 1;
    } else if (strcmp(subcmd, "clean") == 0) {
        nob_delete_file(OUT_BIN);
#ifdef __APPLE__
        remove_dir_recursively(OUT_BIN ".dSYM");
#endif
    } else {
        if (!build_dev())
            return 1;
    }

    return 0;
}
