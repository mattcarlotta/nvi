#ifndef ACCESSORS_H
#define ACCESSORS_H

// Env-variable access patterns, grouped by language and keyed by file
// extension. This replaces convention-based suffix matching (`_ENV`) with
// intent-based matching: a token is an env var because it is read through a
// known environment accessor, not because of how it is spelled.
//
// NOTE: WHAT THIS CANNOT CATCH:
//   - destructuring:   const { DATABASE_URL } = process.env
//   - aliasing:        const e = process.env; e.FOO
//   - dynamic keys:    const key = "DATABASE_URL"; process.env[key]        os.getenv(key)

#include "arena.h"
#include <stddef.h>

typedef enum {
    ident,     // identifier follows the prefix:                    process.env.FOO
    quoted,    // first string literal ("..."/'...') follows:       getenv("FOO")
    braced,    // interpolated identifier within '{' up to '}':     $ENV{FOO}  ${FOO}
    parened,   // indentifier within '(' up to ')':                 $env(FOO)
    expansion, // POSIX-style parameter expansion:                  ${FOO}  ${FOO:-default}  ${FOO:?err}
} pattern_t;

typedef struct {
    const char *prefix;
    size_t prefix_len;
    pattern_t pattern;
} accessor_t;

typedef struct {
    const char *ext;
    const accessor_t *accessors;
    size_t count;
} ext_entry;

typedef struct {
    const char *ext;
    const accessor_t *accessors;
    size_t accessor_count;
} file_ext_t;

typedef struct {
    file_ext_t *items;
    size_t count;
    size_t capacity;
} file_ext_map_t;

const ext_entry *get_scan_extension(const char *ext);
const file_ext_t *get_file_extension(const file_ext_map_t *map, const char *ext);
void append_file_extension(arena_t *arena, file_ext_map_t *map, const ext_entry *entry);

#endif // ACCESSORS_H
