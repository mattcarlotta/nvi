#ifndef STRSET_H
#define STRSET_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Linear probing hash set of borrowed string pointers, used
// to dedupe env keys

#define SET_INIT_CAP 16

typedef struct {
    char *key;
    size_t len;
    uint64_t hash;
} set_entry_t;

typedef struct {
    set_entry_t *slots;
    // total number of slots (always a power of two)
    size_t capacity;
    size_t count;
} set_t;

// This is FNV-1a (Fowler-Noll-Vo), a non-cryptographic hash function.
// The two magic constants are specified by the algorithm:
// Magic Offset basis: 14695981039346656037 (decimal) -- this is the seed
// Magic Prime: 1099511628211 -- chosen for good avalanche properties over 64-bit integers
static inline uint64_t set_hash(const char *key, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= (unsigned char)key[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static inline void set_grow(set_t *set, size_t need) {
    size_t new_cap = set->capacity == 0 ? SET_INIT_CAP : set->capacity;

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
    while (new_cap * 7 < (set->count + need) * 10) {
        new_cap *= 2;
    }

    if (new_cap == set->capacity) {
        return;
    }

    set_entry_t *new_slots = calloc(new_cap, sizeof(*new_slots));
    if (new_slots == NULL) {
        fprintf(stderr, "[ERROR] Failed to allocate hash set (system out of memory?); aborting.\n");
        fflush(stderr);
        abort();
    }

    size_t mask = new_cap - 1;
    for (size_t i = 0; i < set->capacity; ++i) {
        if (set->slots[i].key == NULL) {
            continue;
        }

        size_t j = set->slots[i].hash & mask;
        while (new_slots[j].key != NULL) {
            j = (j + 1) & mask;
        }

        new_slots[j] = set->slots[i];
    }

    free(set->slots);
    set->slots = new_slots;
    set->capacity = new_cap;
}

static inline bool set_contains(const set_t *set, const char *key, size_t len) {
    if (set->capacity == 0) {
        return false;
    }

    uint64_t hash = set_hash(key, len);
    size_t mask = set->capacity - 1;
    size_t i = hash & mask;

    while (set->slots[i].key != NULL) {
        if (set->slots[i].hash == hash && set->slots[i].len == len && memcmp(set->slots[i].key, key, len) == 0) {
            return true;
        }
        i = (i + 1) & mask;
    }

    return false;
}

// Adds a borrowed pointer. The caller must ensure the key is not already present
// (e.g. via set_contains) and that it stays alive for the set's lifetime.
static inline void set_add(set_t *set, char *key, size_t len) {
    set_grow(set, 1);

    uint64_t hash = set_hash(key, len);
    size_t mask = set->capacity - 1;
    size_t i = hash & mask;

    while (set->slots[i].key != NULL) {
        i = (i + 1) & mask;
    }

    set->slots[i].key = key;
    set->slots[i].len = len;
    set->slots[i].hash = hash;
    ++set->count;
}

static inline void set_free(set_t *set) {
    free(set->slots);
    set->slots = NULL;
    set->capacity = 0;
    set->count = 0;
}

#endif
