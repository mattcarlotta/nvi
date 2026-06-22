#ifndef LIST_H
#define LIST_H
#include <stddef.h>
#include <stdlib.h>

#define DA_INIT_CAP 8

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} list_t;

static inline void list_append(list_t *list, const char *value) {
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity == 0 ? DA_INIT_CAP : list->capacity * 2;
        const char **p = realloc(list->items, new_cap * sizeof(*list->items));
        if (p == NULL)
            abort();
        list->items = p;
        list->capacity = new_cap;
    }
    list->items[list->count++] = value;
}

static inline void free_list(list_t *list) {
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
#endif
