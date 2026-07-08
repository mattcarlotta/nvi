#include "arena.h"
#include "arg.h"
#include "emitter.h"
#include "parser.h"
#include "result.h"
#include "scanner.h"
#include "timer.h"
#include "tokenizer.h"
#include "tty.h"
#include <stdlib.h>

int main(int argc, const char **argv) {
    tty_init();

    const double start = monotonic_seconds();

    arena_t arena = arena_init(0);
    args_t args = {0};
    scanner_t scanner = {0};
    tokenizer_t tokenizer = {0};
    env_map_t env_map = {0};
    result_t result = RESULT_OK;

    result = parse_args(argc, argv, &arena, &args);
    if (!result.ok) {
        goto done;
    }

    if (args.scan_exts.count > 0) {
        result = run_scanner(&arena, &args, &scanner);

        if (!result.ok) {
            goto done;
        }
    }

    if (args.files.count == 0) {
        goto done;
    }

    result = run_tokenizer(&arena, &args, &tokenizer);
    if (!result.ok) {
        goto done;
    }

    result = run_parser(&arena, &args, &tokenizer.tokens, &env_map);
    if (!result.ok) {
        goto done;
    }

    if (args.command.count == 0) {
        goto done;
    }

    run_emitter(&args, &env_map);

done:
    if (result.ok && args.dry_run) {
        log_dry_run_time(start);
    }
    fflush(stderr);
    fflush(stdout);
    arena_free(&arena);
    return result.code;
}
