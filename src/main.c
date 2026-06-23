#include "arg.h"
#include "result.h"
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

    const result_t arg_res = arg_parser(&args);

    if (!arg_res.ok) {
        fflush(stderr);
        free_args(&args);
        return arg_res.errcode;
    }

    free_args(&args);

    return 0;
}
