#include "arg.h"
#include "result.h"
#include "scanner.h"
#include "tokenizer.h"
#include "tty.h"
#include <stdio.h>

int main(int argc, char **argv) {
    tty_init();

    result_t result = {.ok = true, .errcode = 0};

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
    tokenizer_t tokenizer = {0};

    result = parse_args(&args);
    if (!result.ok) {
        goto cleanup;
    }

    if (args.scan_exts.count > 0) {
        result = run_scanner(&args, &scanner);

        if (!result.ok) {
            goto cleanup;
        }
    }

    result = run_tokenizer(&args, &tokenizer);
    if (!result.ok) {
        goto cleanup;
    }

    if (args.dry_run) {
        print_dry_run_message();
        goto cleanup;
    }

    goto cleanup;

cleanup:
    fflush(stderr);
    free_args(&args);
    free_scanner(&scanner);
    free_tokenizer(&tokenizer);
    return result.errcode;
}
