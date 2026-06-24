#include "arg.h"
#include "result.h"
#include "scanner.h"
#include <stdio.h>

int main(int argc, char **argv) {
    int exit_code = 0;

    args_t args = {.argc = argc,
                   .argv = argv,
                   .command = {.count = 0, .items = NULL},
                   .format = get_default_format(),
                   .dry_run = false,
                   .files = {0},
                   .ignored = {0},
                   .required = {0},
                   .scan_exts = {0}};
    scanner_t scanner = {0};

    const result_t arg_res = parse_args(&args);
    if (!arg_res.ok) {
        exit_code = arg_res.errcode;
        goto cleanup;
    }

    if (args.scan_exts.count > 0) {
        const result_t scan_res = run_scanner(&args, &scanner);

        if (!scan_res.ok) {
            exit_code = scan_res.errcode;
            goto cleanup;
        }
    }

    goto cleanup;

cleanup:
    fflush(stderr);
    free_args(&args);
    free_scanner(&scanner);
    return exit_code;
}
