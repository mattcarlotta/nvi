#ifndef LIST_H
#define LIST_H
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} list_t;

static inline void list_free(list_t *list) {
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

#endif
