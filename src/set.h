#ifndef SET_H
#define SET_H
#include "arena.h"
#include "hash.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Arena-backed hash set of owned string keys. On insert the key is copied into the arena
// (arena_strndup, NUL-terminated), so callers may hand in pointers to transient storage
// (per-file scratch, per-worker findings) and the set makes them durable for the arena's
// lifetime. This is what lets scanner findings survive their worker arena being freed at
// join, and lets `required` borrow the set's keys.
//
// Linear probing, power-of-two capacity, FNV-1a, 0.7 max load factor. Slots come from the
// arena; a rehash abandons the old table in the arena (reclaimed at arena_free). No free
// function: teardown is arena_free alone.

#define SET_INIT_CAP 16

typedef struct {
    const char *key;
    size_t len;
    uint64_t hash;
} set_entry_t;

typedef struct {
    set_entry_t *items;
    size_t capacity; // power of two
    size_t count;
    arena_t *arena;
} set_t;

// Grows to hold count+need entries under a 0.7 load factor. The 0.7 target keeps the
// unsuccessful-probe count (the one insertions and misses pay, Knuth TAOCP Vol. 3) near 2.5
// rather than the double-digit territory past 0.8.
static inline void set_grow(set_t *set, size_t need) {
    size_t new_cap = set->capacity == 0 ? SET_INIT_CAP : set->capacity;
    while (new_cap * 7 < (set->count + need) * 10) {
        new_cap *= 2;
    }

    if (new_cap == set->capacity) {
        return;
    }

    // arena_alloc_zeroed never returns NULL (aborts on OOM) and zeroes the table so the
    // key == NULL empty-slot sentinel holds
    set_entry_t *new_items = arena_alloc_zeroed(set->arena, new_cap * sizeof(*new_items));

    size_t mask = new_cap - 1;
    for (size_t i = 0; i < set->capacity; ++i) {
        if (set->items[i].key == NULL) {
            continue;
        }

        size_t j = set->items[i].hash & mask;
        while (new_items[j].key != NULL) {
            j = (j + 1) & mask;
        }

        new_items[j] = set->items[i];
    }

    // old table abandoned in the arena; reclaimed wholesale at arena_free
    set->items = new_items;
    set->capacity = new_cap;
}

// `expected` of 0 allocates nothing until the first add. Passing a known upper bound sizes
// the table once so inserts never rehash (zero stranded tables).
static inline set_t set_init(arena_t *arena, size_t expected) {
    set_t set = {.items = NULL, .capacity = 0, .count = 0, .arena = arena};

    if (expected > 0) {
        set_grow(&set, expected);
    }

    return set;
}

static inline bool set_contains(const set_t *set, const char *key, size_t len) {
    if (set->capacity == 0) {
        return false;
    }

    uint64_t hash = fnv1a(key, len);
    size_t mask = set->capacity - 1;
    size_t i = hash & mask;

    while (set->items[i].key != NULL) {
        if (set->items[i].hash == hash && set->items[i].len == len && memcmp(set->items[i].key, key, len) == 0) {
            return true;
        }
        i = (i + 1) & mask;
    }

    return false;
}

// Copies `key` into the arena and inserts it if absent. Returns true if newly inserted,
// false if already present. The presence check happens before the copy, so duplicates never
// waste an arena_strndup.
static inline bool set_add(set_t *set, const char *key, size_t len) {
    set_grow(set, 1);

    uint64_t hash = fnv1a(key, len);
    size_t mask = set->capacity - 1;
    size_t i = hash & mask;

    while (set->items[i].key != NULL) {
        if (set->items[i].hash == hash && set->items[i].len == len && memcmp(set->items[i].key, key, len) == 0) {
            return false;
        }
        i = (i + 1) & mask;
    }

    set->items[i].key = arena_strndup(set->arena, key, len);
    set->items[i].len = len;
    set->items[i].hash = hash;
    ++set->count;
    return true;
}

#endif // SET_H
