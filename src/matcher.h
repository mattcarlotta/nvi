#ifndef MATCHER_H
#define MATCHER_H
#include "accessors.h"
#include "dynarr.h"

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
    size_t count, capacity;
} env_matches_t;

void scan_file_content(const char *contents, size_t len, const accessor_t *accessors, size_t accessor_count,
                       env_matches_t *matches);
static inline void free_env_matches(env_matches_t *m) { DYN_ARR_FREE(m); }

#endif
