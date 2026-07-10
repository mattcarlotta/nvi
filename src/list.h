#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
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

#endif // LIST_H
