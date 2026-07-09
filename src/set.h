#ifndef SET_H
#define SET_H

#include "arena.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} set_t;

bool set_contains(const set_t *set, const char *key);
void set_add(arena_t *arena, set_t *set, const char *key);

#endif // SET_H
