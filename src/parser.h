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
