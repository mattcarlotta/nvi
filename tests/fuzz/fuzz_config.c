// libFuzzer harness for the '.nvi' config file tokenizer (POSIX only).
//
// Feeds arbitrary bytes through tokenize_config_file as if they were the
// contents of a '@<path>' config file. Covers token splitting, comment
// skipping, line counting, the embedded '--' and nested '@' rejections, and
// the CONFIG_MAX_TOKENS cap. No filesystem access, fresh arena per input,
// so every iteration is hermetic.
//
// Build/run: ./nob fuzz config [extra libFuzzer flags]
// Progress/stall behavior: see fuzz_watchdog.h

#include "arena.h"
#include "config.h"
#include "file.h"
#include "fuzz_watchdog.h"
#include "result.h"

#include <stdint.h>
#include <string.h>

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc;
    (void)argv;

    fuzz_watchdog_init("config tokenizer");

    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size > MAX_FILE_SIZE) {
        return 0;
    }

    fuzz_watchdog_input_begin(data, size);

    arena_t arena = {0};
    config_tokens_t tokens = {0};

    // mirror open_file: contents are NUL-terminated, len excludes the NUL
    char *contents = arena_alloc(&arena, size + 1);
    memcpy(contents, data, size);
    contents[size] = '\0';

    const file_details_t file = {.contents = contents, .path = "fuzz.nvi", .len = size};

    tokenize_config_file(&arena, &file, &tokens);

    arena_free(&arena);

    fuzz_watchdog_input_end();

    return 0;
}
