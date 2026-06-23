#ifndef MATCHER_H
#define MATCHER_H
#include "accessors.h"

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

void scan_content(const char *contents, size_t len, const accessor_t *accessors, size_t accessor_count,
                  env_matches_t *matches);
void print_matches(const char *path, const env_matches_t *matches);
void env_matches_free(env_matches_t *m);

#endif
