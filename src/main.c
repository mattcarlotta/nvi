#include "arg.h"
#include "emitter.h"
#include "info.h"
#include "parser.h"
#include "result.h"
#include "scanner.h"
#include "timer.h"
#include "tokenizer.h"
#include "tty.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
    tty_init();

    const double start = monotonic_seconds();

    // this would set stderr to be fully buffered to prevent unnecessary line by line prints
    // this may increase performance when dry-running a large code base, but the disadvantage
    // is that partial prints flush to stderr and it feels choppy, disabling for now
    // https://man7.org/linux/man-pages/man3/setvbuf.3p.html
    // setvbuf(stderr, NULL, _IOFBF, 1 << 16);

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

    if (args.files.count == 0) {
        goto done;
    }

    result = run_tokenizer(&args, &tokenizer);
    if (!result.ok) {
        goto done;
    }

    result = run_parser(&args, &tokenizer.tokens, &env_map);
    if (!result.ok) {
        goto done;
    }

    if (args.command.count == 0) {
        goto done;
    }

    run_emitter(&args, &env_map);

    goto done;

done:
    if (args.dry_run) {
        log_time(start);
    }
    fflush(stderr);
    fflush(stdout);
    free_args(&args);
    free_scanner(&scanner);
    free_tokenizer(&tokenizer);
    free_envs(&env_map);
    return result.code;
}
