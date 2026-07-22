#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "arena.h"
#include "arg.h"
#include "file.h"
#include "result.h"
#include <stdbool.h>
#include <stddef.h>

// The tokenizer is responsible for converting .env files into value tokens the parser can
// understand. It also does a little bit of syntax checking to ensure ENV keys aren't missing
// values and ENV values don't have invalid key interpolations. For something as simple as an
// .env file, an AST really isn't needed.

typedef enum { LITERAL_VALUE, COMMENTED_LINE, INTERPOLATED_KEY } value_kind_t;

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
    bool reveal;
    token_list_t tokens;
} tokenizer_t;

static inline const char *get_value_kind_name(value_kind_t kind) {
    switch (kind) {
        case LITERAL_VALUE:
            return "literal value";
        case COMMENTED_LINE:
            return "commented line";
        case INTERPOLATED_KEY:
            return "interpolated key";
    }

    return "unknown value kind";
}

result_t run_tokenizer(arena_t *main_arena, const args_t *args, tokenizer_t *tokenizer);
result_t generate_tokens(arena_t *main_arena, arena_t *scratch, const args_t *args, const file_details_t *file,
                         tokenizer_t *tokenizer);

#endif // TOKENIZER_H
