#ifndef CONFIG_H
#define CONFIG_H

// Expands a clang/MSVC/rustc-style '@<path>' config file into argv
// before flag parsing.
//
// The config should contain same-line whitespace-delimited flag tokens that
// are spliced into argv at the position of the '@<path>' argument. Anything
// specified after the path argument either overrides or appends to it.

#include "arena.h"
#include "file.h"
#include "result.h"

#define CONFIG_MAX_TOKENS 1024

typedef struct {
    int argc;
    const char **argv;
    const char *path;
} config_t;

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} config_tokens_t;

result_t tokenize_config_file(arena_t *arena, const file_details_t *file, config_tokens_t *tokens);
result_t load_config_file(arena_t *arena, int argc, const char **argv, config_t *config);

#endif // CONFIG_H
