#include "arena.h"
#include "arg.h"
#include "emitter.h"
#include "parser.h"
#include "result.h"
#include "scanner.h"
#include "timer.h"
#include "tokenizer.h"
#include "tty.h"

int main(int argc, const char **argv) {
    tty_init();

    const double start = monotonic_seconds();

    arena_t arena;
    arena_init(&arena, 0);

    result_t result = RESULT_OK;
    args_t args = {0};
    scanner_t scanner = {0};
    tokenizer_t tokenizer = {0};
    parser_t parser = {0};

    result = parse_args(&arena, argc, argv, &args);
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

    result = run_parser(&arena, &args, &tokenizer.tokens, &parser);
    if (!result.ok) {
        goto done;
    }

    if (args.command.count == 0) {
        goto done;
    }

    run_emitter(&args, &parser.env_map);

done:
    if (result.ok && args.dry_run) {
        log_dry_run_time(start);
    }
    fflush(stderr);
    fflush(stdout);
    arena_free(&arena);
    return result.code;
}
