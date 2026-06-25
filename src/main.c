#include "arg.h"
#include "result.h"
#include "scanner.h"
#include "tokenizer.h"
#include "tty.h"
#include <stdio.h>

int main(int argc, char **argv) {
    tty_init();

    result_t result = {.ok = true, .errcode = 0};
    args_t args = {0};
    scanner_t scanner = {0};
    tokenizer_t tokenizer = {0};

    result = parse_args(argc, argv, &args);
    if (!result.ok) {
        goto done;
    }

    if (args.scan_exts.count > 0) {
        result = run_scanner(&args, &scanner);

        if (!result.ok) {
            goto done;
        }
    }

    result = run_tokenizer(&args, &tokenizer);
    if (!result.ok) {
        goto done;
    }

    if (args.dry_run) {
        print_dry_run_message();
        goto done;
    }

    goto done;

done:
    fflush(stderr);
    free_args(&args);
    free_scanner(&scanner);
    free_tokenizer(&tokenizer);
    return result.errcode;
}
