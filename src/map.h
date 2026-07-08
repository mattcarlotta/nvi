#ifndef MAP_H
#define MAP_H
#include "arena.h"
#include "hash.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Arena-backed hash map of borrowed string keys to size_t values. Keys are stored by
// pointer, never copied: every key handed in must already live at least as long as the map
// (parser tokens in the main arena, or keys owned by a set_t in the same arena). Values are
// held inline in the slot. Slots come from the arena; a rehash abandons the old table in the
// arena. No free function: teardown is arena_free alone.
//
// Linear probing, power-of-two capacity, FNV-1a, 0.7 max load factor.

#define MAP_INIT_CAP 16
#define MAP_NOT_FOUND SIZE_MAX

typedef struct {
    const char *key; // borrowed; must outlive the map
    size_t len;
    uint64_t hash;
    size_t value;
} map_entry_t;

typedef struct {
    map_entry_t *items;
    size_t capacity; // power of two
    size_t count;
    arena_t *arena;
} map_t;

static inline void map_grow(map_t *map, size_t need) {
    size_t new_cap = map->capacity == 0 ? MAP_INIT_CAP : map->capacity;
    while (new_cap * 7 < (map->count + need) * 10) {
        new_cap *= 2;
    }

    if (new_cap == map->capacity) {
        return;
    }

    map_entry_t *new_items = arena_alloc_zeroed(map->arena, new_cap * sizeof(*new_items));

    size_t mask = new_cap - 1;
    for (size_t i = 0; i < map->capacity; ++i) {
        if (map->items[i].key == NULL) {
            continue;
        }

        size_t j = map->items[i].hash & mask;
        while (new_items[j].key != NULL) {
            j = (j + 1) & mask;
        }

        new_items[j] = map->items[i];
    }

    // old table abandoned in the arena; reclaimed wholesale at arena_free
    map->items = new_items;
    map->capacity = new_cap;
}

static inline map_t map_init(arena_t *arena, size_t expected) {
    map_t map = {.items = NULL, .capacity = 0, .count = 0, .arena = arena};

    if (expected > 0) {
        map_grow(&map, expected);
    }

    return map;
}

static inline size_t map_get(const map_t *map, const char *key, size_t len) {
    if (map->capacity == 0) {
        return MAP_NOT_FOUND;
    }

    uint64_t hash = fnv1a(key, len);
    size_t mask = map->capacity - 1;
    size_t i = hash & mask;

    while (map->items[i].key != NULL) {
        if (map->items[i].hash == hash && map->items[i].len == len && memcmp(map->items[i].key, key, len) == 0) {
            return map->items[i].value;
        }
        i = (i + 1) & mask;
    }

    return MAP_NOT_FOUND;
}

static inline bool map_contains(const map_t *map, const char *key, size_t len) {
    return map_get(map, key, len) != MAP_NOT_FOUND;
}

// Inserts the borrowed key, or updates the value if the key is already present.
static inline void map_add(map_t *map, const char *key, size_t len, size_t value) {
    map_grow(map, 1);

    uint64_t hash = fnv1a(key, len);
    size_t mask = map->capacity - 1;
    size_t i = hash & mask;

    while (map->items[i].key != NULL) {
        if (map->items[i].hash == hash && map->items[i].len == len && memcmp(map->items[i].key, key, len) == 0) {
            map->items[i].value = value;
            return;
        }
        i = (i + 1) & mask;
    }

    map->items[i].key = key;
    map->items[i].len = len;
    map->items[i].hash = hash;
    map->items[i].value = value;
    ++map->count;
}

#endif // MAP_H
