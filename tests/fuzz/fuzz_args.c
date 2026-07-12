// libFuzzer harness for the argv parser (POSIX only).
//
// Splits the input on NUL bytes into argv entries (argv[0] is a fixed program
// name) and feeds them through parse_args. Covers flag dispatch, flag-param
// pairing, scan extension validation, thread count parsing, format selection,
// and the end-of-options command capture. Fresh arena per input, no
// filesystem access.
//
// Build/run: ./nob fuzz args [extra libFuzzer flags]
// Progress/stall behavior: see fuzz_watchdog.h

#include "arena.h"
#include "arg.h"
#include "fuzz_watchdog.h"

#include <stdint.h>
#include <string.h>

// generous for real usage while keeping mutated inputs meaningful
#define MAX_FUZZ_ARGS 64
#define MAX_FUZZ_INPUT (64 * 1024)

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc;
    (void)argv;

    fuzz_watchdog_init("argv parser");

    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size > MAX_FUZZ_INPUT) {
        return 0;
    }

    fuzz_watchdog_input_begin(data, size);

    arena_t arena = {0};
    args_t args = {0};

    // copy with a trailing NUL so the final segment is always terminated
    char *buf = arena_alloc(&arena, size + 1);
    memcpy(buf, data, size);
    buf[size] = '\0';

    const char *argv[MAX_FUZZ_ARGS];
    int argc = 0;

    argv[argc++] = "nvi";

    // NUL bytes delimit argv entries, mirroring how the OS hands them over
    char *cursor = buf;
    char *end = buf + size;
    while (cursor < end && argc < MAX_FUZZ_ARGS) {
        argv[argc++] = cursor;
        cursor += strlen(cursor) + 1;
    }

    parse_args(&arena, argc, argv, &args);

    arena_free(&arena);

    fuzz_watchdog_input_end();

    return 0;
}
