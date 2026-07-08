#ifndef LIST_H
#define LIST_H
#include "arena.h"
#include "dynarr.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} list_t;

static inline bool list_contains(const list_t *list, const char *key) {
    for (size_t i = 0; i < list->count; ++i) {
        if (strcmp(list->items[i], key) == 0) {
            return true;
        }
    }

    return false;
}

static inline void list_append(arena_t *arena, list_t *list, const char *value) {
    ARENA_DYN_ARR_APPEND(arena, list, value);
}

static inline bool list_append_unique(arena_t *arena, list_t *list, const char *value) {
    if (list_contains(list, value)) {
        return false;
    }
    ARENA_DYN_ARR_APPEND(arena, list, value);
    return true;
}

#endif // LIST_H
