#ifndef PARSER_H
#define PARSER_H
#include "arg.h"
#include "tokenizer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *key;
    char *value;
} env_t;

typedef struct {
    env_t *items;
    size_t count;
    size_t capacity;
} env_map_t;

result_t run_parser(args_t *args, token_list_t *tokens, env_map_t *env_map);
void free_envs(env_map_t *env_map);

#endif
