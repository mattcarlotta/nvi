#include "arg.h"
#include <stdio.h>

int main(int argc, char **argv) {
    arg_t args = {.argc = argc,
                  .argv = argv,
                  .command = {.count = 0, .items = NULL},
                  .format = get_default_format(),
                  .dry_run = false,
                  .files = {0},
                  .ignored = {0},
                  .required = {0},
                  .scan_exts = {0}};

    const int err = arg_parser(&args);
    if (err != 0) {
        return err;
    }

    fflush(stderr);

    free_args(&args);

    return 0;
}
