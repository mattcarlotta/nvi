#ifndef HASHMAP_H
#define HASHMAP_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Linear probing hash map of borrowed string pointers to size_t values.

#define HASHMAP_INIT_CAP 16
#define HASHMAP_NOT_FOUND SIZE_MAX

typedef struct {
    const char *key;
    size_t len;
    uint64_t hash;
    size_t value;
} hashmap_entry_t;

typedef struct {
    hashmap_entry_t *slots;
    size_t capacity; // total number of slots (always a power of two)
    size_t count;
} hashmap_t;

static inline void hashmap_grow(hashmap_t *map, size_t need) {
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

    hashmap_entry_t *new_slots = calloc(new_cap, sizeof(*new_slots));
    if (new_slots == NULL) {
        fprintf(stderr, "[ERROR] Failed to allocate hash map (system out of memory?); aborting.\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
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

#define FNV_OFFSET 1469598103934665603ULL
#define FNV_PRIME 1099511628211ULL

// This is FNV-1a (Fowler-Noll-Vo), a non-cryptographic hash function.
// The two magic constants are specified by the algorithm:
// Magic Offset basis: 14695981039346656037 (decimal) -- this is the seed
// Magic Prime: 1099511628211 -- chosen for good avalanche properties over 64-bit integers
static inline uint64_t fnv1a(const char *key, size_t len) {
    uint64_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; ++i) {
        h ^= (unsigned char)key[i];
        h *= FNV_PRIME;
    }
    return h;
}

static inline size_t hashmap_get(const hashmap_t *map, const char *key, size_t len) {
    if (map->capacity == 0) {
        return HASHMAP_NOT_FOUND;
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

    return HASHMAP_NOT_FOUND;
}

static inline bool hashmap_contains(const hashmap_t *map, const char *key, size_t len) {
    return hashmap_get(map, key, len) != HASHMAP_NOT_FOUND;
}

static inline void hashmap_append(hashmap_t *map, const char *key, size_t len, size_t value) {
    hashmap_grow(map, 1);

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

static inline void free_hashmap(hashmap_t *map) {
    free(map->slots);
    map->slots = NULL;
    map->capacity = 0;
    map->count = 0;
}

#endif
