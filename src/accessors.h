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

#include <stddef.h>
typedef enum {
    ident,   // identifier follows the prefix:                   process.env.FOO
    quoted,  // first string literal ("..."/'...') follows:      getenv("FOO")
    braced,  // interpolated identifier within '{' up to '}':               $ENV{FOO}  ${FOO}
    parened, // indentifier within '(' up to ')':                $env(FOO)
} pattern_t;

typedef struct {
    const char *prefix;
    pattern_t pattern;
} accessor_t;

typedef struct {
    const char *ext;
    const accessor_t *accessors;
    size_t count;
} ext_entry;

const ext_entry *find_ext(const char *ext);

#endif
