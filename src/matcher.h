#ifndef MATCHER_H
#define MATCHER_H

#include "accessors.h"
#include "arena.h"
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

// Matches reference 'file->contents' and the match list itself lives in 'scratch', so both
// share the same lifetime and die together when the caller resets the scratch arena.
void scan_file_content(arena_t *scratch, const file_details_t *file, const file_ext_t *file_ext_match,
                       env_key_matches_t *env_key_matches);

#endif // MATCHER_H
