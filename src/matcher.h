#ifndef MATCHER_H
#define MATCHER_H
#include "dynarr.h"
#include "file.h"
#include "scanner.h"

typedef struct {
    const char *key;
    size_t key_len;
    size_t start;
    size_t end;
} env_t;

typedef struct {
    const char *key;
    size_t key_len;
    size_t line;
    size_t byte;
} env_match_t;

typedef struct {
    env_match_t *items;
    size_t count;
    size_t capacity;
} env_matches_t;

void scan_file_content(file_details_t *file, const file_ext_t *file_ext_match, env_matches_t *env_matches);
static inline void free_env_matches(env_matches_t *env_matches) { DYN_ARR_FREE(env_matches); }

#endif
