#define NOB_IMPLEMENTATION
#include "nob.h"

#define SRC_FILES "src/main.c", "src/arg.c"
#define OUT "nvi"

static bool build_dev(void) {
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "clang", "-Isrc", "-Wall", "-Wextra", "-Wpedantic", "-std=c17", "-g", "-O0", "-o", OUT,
                   SRC_FILES);
    return nob_cmd_run(&cmd);
}

static bool build_release(void) {
    Nob_Cmd cmd = {0};

    nob_cmd_append(&cmd, "rm", "-rf", OUT, OUT ".dSYM");
    if (!nob_cmd_run(&cmd))
        return false;

    cmd.count = 0;
    nob_cmd_append(&cmd, "clang", "-Isrc", "-Wall", "-Wextra", "-Wpedantic", "-std=c17", "-Oz", "-flto",
                   "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-Wl,-dead_strip", "-Wl,-x", "-o", OUT,
                   SRC_FILES);
    if (!nob_cmd_run(&cmd))
        return false;

    cmd.count = 0;
    nob_cmd_append(&cmd, "strip", "-x", OUT);
    return nob_cmd_run(&cmd);
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
        nob_delete_file(OUT);
        nob_delete_file(OUT ".dSYM");
    } else {
        if (!build_dev())
            return 1;
    }

    return 0;
}
