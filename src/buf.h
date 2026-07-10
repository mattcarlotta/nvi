#ifndef BUF_H
#define BUF_H

#include "arena.h"
#include <stddef.h>

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
    arena_t *arena;
} buf_t;

#endif // BUF_H
