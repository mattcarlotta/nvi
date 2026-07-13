#ifndef PARSER_H
#define PARSER_H

#include "arena.h"
#include "arg.h"
#include "hashmap.h"
#include "tokenizer.h"

// The parser is strictly responsible for converting tokenized values from .env files
// into KV pairs within a single env hash map.
//
// Supported token types:
// commented -> intentionally skipped by the parser
// literal -> a literal value (combines multi-line values as well)
// interpolated -> extracting a value from a process ENV or ENV key from a previous .env file

// Interpolation can multiply value length exponentially (a few KB of nested
// ${KEY} references can expand into gigabytes), so assembled values are capped.
#define MAX_ENV_VALUE_SIZE ((size_t)1024 * 1024)

// Interpolation also amplifies across keys: each ~9-byte ${KEY} line can expand
// to a value up to MAX_ENV_VALUE_SIZE, so a 10MB file (MAX_FILE_SIZE) could
// otherwise attempt ~1TB of total output and die on OOM instead of exiting
// cleanly. 8MB comfortably exceeds any real environment (Linux caps a single
// exec'd env string at 128KB and the whole argv+env block near 2MB) while
// bounding the blowup.
#define MAX_PARSED_OUTPUT ((size_t)8 * 1024 * 1024)

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} list_t;

typedef struct {
    const char *key;
    char *value;
} env_t;

typedef struct {
    env_t *items;
    size_t count;
    size_t capacity;
    hashmap_t index;
} env_map_t;

typedef struct {
    env_map_t env_map;
    list_t missing_envs;
} parser_t;

env_t *get_env_from_map(env_map_t *env_map, const char *entry);
result_t run_parser(arena_t *arena, const args_t *args, const token_list_t *tokens, parser_t *parser);

#endif // PARSER_H
