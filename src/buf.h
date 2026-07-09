#ifndef BUF_H
#define BUF_H

#include "arena.h"
#include <stddef.h>

// Growable byte buffer backed by an arena; set 'arena' before the first append.
typedef struct {
    char *items;
    size_t count;
    size_t capacity;
    arena_t *arena;
} buf_t;

#endif
