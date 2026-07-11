// libFuzzer harness for the tokenizer -> parser pipeline (POSIX only).
//
// Feeds arbitrary bytes through generate_tokens and run_parser as if they were
// the contents of a single .env file. No filesystem access, fresh arenas per
// input, so every iteration is hermetic.
//
// Build/run: ./nob fuzz parser [extra libFuzzer flags]
// Progress/stall behavior: see fuzz_watchdog.h

#include "arena.h"
#include "arg.h"
#include "file.h"
#include "fuzz_watchdog.h"
#include "parser.h"
#include "result.h"
#include "tokenizer.h"

#include <stdint.h>
#include <string.h>

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc;
    (void)argv;

    fuzz_watchdog_init("tokenizer->parser");

    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size > MAX_FILE_SIZE) {
        return 0;
    }

    fuzz_watchdog_input_begin(data, size);

    arena_t arena = {0};
    arena_t scratch = {0};
    args_t args = {0};
    tokenizer_t tokenizer = {0};
    parser_t parser = {0};

    // mirror open_file: contents are NUL-terminated, len excludes the NUL
    char *contents = arena_alloc(&arena, size + 1);
    memcpy(contents, data, size);
    contents[size] = '\0';

    const file_details_t file = {.contents = contents, .path = "fuzz.env", .len = size};

    result_t result = generate_tokens(&arena, &scratch, &args, &file, &tokenizer);
    if (result.ok) {
        run_parser(&arena, &args, &tokenizer.tokens, &parser);
    }

    arena_free(&scratch);
    arena_free(&arena);

    fuzz_watchdog_input_end();

    return 0;
}
