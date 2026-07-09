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

    // one arena owns everything with process lifetime: args lists, scan extensions, merged
    // scan keys, tokens, and the parsed env map. Worker- and file-scoped arenas live inside
    // the scanner and tokenizer. The single free below replaces all per-structure cleanup
    // and is itself optional before an exec, since the process image is replaced anyway.
    arena_t arena;
    arena_init(&arena, 0);

    result_t result = RESULT_OK;
    args_t args = {0};
    scanner_t scanner = {0};
    tokenizer_t tokenizer = {0};
    env_map_t env_map = {0};

    result = parse_args(&arena, argc, argv, &args);
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

done:
    if (result.ok && args.dry_run) {
        log_dry_run_time(start);
    }
    fflush(stderr);
    fflush(stdout);
    arena_free(&arena);
    return result.code;
}
