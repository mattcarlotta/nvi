#ifndef LIST_H
#define LIST_H
#include "dynarr.h"
#include <stddef.h>
#include <string.h>

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} list_t;

static inline void free_list(list_t *list) { DYN_ARR_FREE(list); }

#endif
