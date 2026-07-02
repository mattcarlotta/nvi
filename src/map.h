#ifndef STRMAP_H
#define STRMAP_H
#include "hash.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Linear probing hash map of borrowed string pointers to size_t values,
// used to index env keys to their position in an insertion-ordered array.
// Same layout and growth policy as set.h (see the load-factor notes there).

#define MAP_INIT_CAP 16
#define MAP_NOT_FOUND SIZE_MAX

typedef struct {
    const char *key;
    size_t len;
    uint64_t hash;
    size_t value;
} map_entry_t;

typedef struct {
    map_entry_t *slots;
    size_t capacity; // total number of slots (always a power of two)
    size_t count;
} map_t;

static inline void map_grow(map_t *map, size_t need) {
    size_t new_cap = map->capacity == 0 ? MAP_INIT_CAP : map->capacity;

    while (new_cap * 7 < (map->count + need) * 10) {
        new_cap *= 2;
    }

    if (new_cap == map->capacity) {
        return;
    }

    map_entry_t *new_slots = calloc(new_cap, sizeof(*new_slots));
    if (new_slots == NULL) {
        fprintf(stderr, "[ERROR] Failed to allocate hash map (system out of memory?); aborting.\n");
        fflush(stderr);
        abort();
    }

    size_t mask = new_cap - 1;
    for (size_t i = 0; i < map->capacity; ++i) {
        if (map->slots[i].key == NULL) {
            continue;
        }

        size_t j = map->slots[i].hash & mask;
        while (new_slots[j].key != NULL) {
            j = (j + 1) & mask;
        }

        new_slots[j] = map->slots[i];
    }

    free(map->slots);
    map->slots = new_slots;
    map->capacity = new_cap;
}

// Returns the stored value for key, or MAP_NOT_FOUND.
static inline size_t map_get(const map_t *map, const char *key, size_t len) {
    if (map->capacity == 0) {
        return MAP_NOT_FOUND;
    }

    uint64_t hash = fnv1a(key, len);
    size_t mask = map->capacity - 1;
    size_t i = hash & mask;

    while (map->slots[i].key != NULL) {
        if (map->slots[i].hash == hash && map->slots[i].len == len && memcmp(map->slots[i].key, key, len) == 0) {
            return map->slots[i].value;
        }
        i = (i + 1) & mask;
    }

    return MAP_NOT_FOUND;
}

// Inserts a borrowed pointer, or updates the value if the key is present.
// The caller must ensure the key stays alive for the map's lifetime.
static inline void map_put(map_t *map, const char *key, size_t len, size_t value) {
    map_grow(map, 1);

    uint64_t hash = fnv1a(key, len);
    size_t mask = map->capacity - 1;
    size_t i = hash & mask;

    while (map->slots[i].key != NULL) {
        if (map->slots[i].hash == hash && map->slots[i].len == len && memcmp(map->slots[i].key, key, len) == 0) {
            map->slots[i].value = value;
            return;
        }
        i = (i + 1) & mask;
    }

    map->slots[i].key = key;
    map->slots[i].len = len;
    map->slots[i].hash = hash;
    map->slots[i].value = value;
    ++map->count;
}

static inline void free_map(map_t *map) {
    free(map->slots);
    map->slots = NULL;
    map->capacity = 0;
    map->count = 0;
}

#endif
