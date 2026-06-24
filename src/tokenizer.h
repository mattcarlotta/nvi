#ifndef TOKENIZER_H
#define TOKENIZER_H
#include "arg.h"
#include "file.h"
#include "result.h"
#include <stddef.h>

typedef enum { LITERAL, COMMENTED, INTERPOLATED } value_kind_t;

typedef struct {
    char *value;
    size_t value_len;
    value_kind_t kind;
    size_t line;
    size_t byte;
} value_token_t;

typedef struct {
    value_token_t *items;
    size_t count;
    size_t capacity;
} value_token_list_t;

typedef struct {
    char *key;
    const char *file;
    value_token_list_t values;
} token_t;

typedef struct {
    token_t *items;
    size_t count;
    size_t capacity;
} token_list_t;

typedef struct {
    size_t i;
    size_t byte;
    size_t line;
    const char *file;
    size_t file_len;
    const char *file_name;
    token_list_t tokens;
} tokenizer_t;

result_t run_tokenizer(args_t *args, tokenizer_t *tokenizer);
result_t generate_tokens(tokenizer_t *tokenizer, file_details_t *file);
void free_tokenizer(tokenizer_t *tokenizer);

#endif
