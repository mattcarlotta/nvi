// libFuzzer harness for the scanner's per-file matcher (POSIX only).
//
// Feeds arbitrary bytes through scan_file_content as if they were the contents
// of a source file. The first input byte selects which language's accessor set
// to scan with, covering all five pattern kinds (ident, quoted, braced,
// parened, expansion) plus the comment-skimming state machines; the remaining
// bytes are the file content. No filesystem access, fresh arena per input.
//
// Build/run: ./nob fuzz matcher [extra libFuzzer flags]
// Progress/stall behavior: see fuzz_watchdog.h

#include "accessors.h"
#include "arena.h"
#include "file.h"
#include "fuzz_watchdog.h"
#include "matcher.h"

#include <stdint.h>
#include <string.h>

// one extension per distinct accessor set/pattern kind; adding more duplicates
// coverage since same-language extensions share an accessor table
static const char *const exts[] = {
    "js",   // ident + quoted (process.env.FOO, process.env["FOO"], import.meta.env)
    "py",   // quoted (os.getenv, os.environ.get)
    "go",   // quoted (os.Getenv)
    "rs",   // quoted (env::var)
    "rb",   // braced-ish (ENV["FOO"]) + quoted
    "java", // quoted (System.getenv)
    "pl",   // braced ($ENV{FOO})
    "php",  // quoted + parened
    "c",    // quoted (getenv)
    "yaml", // expansion (${FOO}, ${FOO:-default}, ${FOO:?err})
};

#define EXT_COUNT (sizeof(exts) / sizeof(exts[0]))

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc;
    (void)argv;

    fuzz_watchdog_init("scanner matcher");

    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1 || size - 1 > MAX_FILE_SIZE) {
        return 0;
    }

    const file_ext_t *file_ext_match = get_scan_extension(exts[data[0] % EXT_COUNT]);
    if (file_ext_match == NULL) {
        return 0;
    }

    fuzz_watchdog_input_begin(data, size);

    arena_t scratch = {0};
    env_key_matches_t env_key_matches = {0};

    // mirror open_file: contents are NUL-terminated, len excludes the NUL
    size_t len = size - 1;
    char *contents = arena_alloc(&scratch, len + 1);
    memcpy(contents, data + 1, len);
    contents[len] = '\0';

    const file_details_t file = {.contents = contents, .path = "fuzz.src", .len = len};

    scan_file_content(&scratch, &file, file_ext_match, &env_key_matches);

    arena_free(&scratch);

    fuzz_watchdog_input_end();

    return 0;
}
