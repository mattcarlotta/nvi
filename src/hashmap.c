#include "hashmap.h"
#include "hash.h"
#include <string.h>

#define HASHMAP_INIT_CAP 16

static void hashmap_grow(arena_t *arena, hashmap_t *map, size_t need) {
    size_t new_cap = map->capacity == 0 ? HASHMAP_INIT_CAP : map->capacity;

    // With linear probing, the expected number of probes for a successful lookup is approximately:
    // (1/2) * (1 + 1/(1 - a))
    //
    // And for an unsuccessful lookup (or insertion, which has to find an empty slot):
    // (1/2) * (1 + 1/(1 - a)^2)
    //
    // Where a is the load factor. These are the Knuth formulas from The Art of Computer Programming, Vol. 3. The
    // unsuccessful case is the one that matters most because insertions and cache-miss probes both follow that curve.
    //
    // Plugging in numbers:
    // Load factor  Successful probes   Unsuccessful probes
    // 0.5          1.5                 2.5
    // 0.7          2.17                6.06
    // 0.8          3.0                 13.0
    // 0.9          5.5                 50.5
    // 0.95         10.5                200.5
    while (new_cap * 7 < (map->count + need) * 10) {
        new_cap *= 2;
    }

    if (new_cap == map->capacity) {
        return;
    }

    hashmap_entry_t *new_items = arena_alloc_zeroed(arena, new_cap * sizeof(*new_items));

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

    map->items = new_items;
    map->capacity = new_cap;
}

size_t hashmap_get(const hashmap_t *map, const char *key, size_t len) {
    if (map->capacity == 0) {
        return HASHMAP_NOT_FOUND;
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

    return HASHMAP_NOT_FOUND;
}

void hashmap_append(arena_t *arena, hashmap_t *map, const char *key, size_t len, size_t value) {
    hashmap_grow(arena, map, 1);

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
