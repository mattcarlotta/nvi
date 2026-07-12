#include "set.h"
#include "dynarr.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool set_contains(const set_t *set, const char *key) {
    for (size_t i = 0; i < set->count; ++i) {
        if (strcmp(set->items[i], key) == 0) {
            return true;
        }
    }

    return false;
}

void set_add(arena_t *arena, set_t *set, const char *key) {
    if (!set_contains(set, key)) {
        DYN_ARR_APPEND(arena, set, key);
    }
}
