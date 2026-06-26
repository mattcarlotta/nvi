#ifndef MATCHER_H
#define MATCHER_H
#include "accessors.h"
#include "dynarr.h"
#include "file.h"

typedef struct {
    const char *key;
    size_t key_len;
    size_t start;
    size_t end;
} env_key_t;

typedef struct {
    const char *key;
    size_t key_len;
    size_t line;
    size_t byte;
} env_key_match_t;

typedef struct {
    env_key_match_t *items;
    size_t count;
    size_t capacity;
} env_key_matches_t;

void scan_file_content(file_details_t *file, const file_ext_t *file_ext_match, env_key_matches_t *env_key_matches);
static inline void free_env_key_matches(env_key_matches_t *env_key_matches) { DYN_ARR_FREE(env_key_matches); }

#endif
