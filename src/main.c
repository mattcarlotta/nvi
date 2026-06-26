#include "arg.h"
#include "parser.h"
#include "result.h"
#include "scanner.h"
#include "tokenizer.h"
#include "tty.h"
#include <stdio.h>

int main(int argc, char **argv) {
    tty_init();

    result_t result = {.ok = true, .code = 0};
    args_t args = {0};
    scanner_t scanner = {0};
    tokenizer_t tokenizer = {0};
    env_map_t env_map = {0};

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

    if (args.files.count == 0 && args.dry_run) {
        log_dry_run_message();
        goto done;
    }

    result = run_parser(&args, &tokenizer.tokens, &env_map);
    if (!result.ok) {
        goto done;
    }

    if (args.dry_run) {
        log_dry_run_message();
        goto done;
    }

    goto done;

done:
    fflush(stderr);
    free_args(&args);
    free_scanner(&scanner);
    free_tokenizer(&tokenizer);
    free_envs(&env_map);
    return result.code;
}
