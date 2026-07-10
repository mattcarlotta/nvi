#include "hashset.h"
#include "hash.h"
#include <assert.h>
#include <string.h>

#define HASHSET_INIT_CAP 16

static void hashset_grow(arena_t *arena, hashset_t *set, size_t need) {
    size_t new_cap = set->capacity == 0 ? HASHSET_INIT_CAP : set->capacity;

    // Load factor capped at 0.7. See hashmap.c for the Knuth probe-count analysis;
    // the math is identical here.
    while (new_cap * 7 < (set->count + need) * 10) {
        new_cap *= 2;
    }

    if (new_cap == set->capacity) {
        return;
    }

    hashset_entry_t *new_items = arena_alloc_zeroed(arena, new_cap * sizeof(*new_items));

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

    set->items = new_items;
    set->capacity = new_cap;
}

// Probes for key. Returns the index of the matching entry if present,
// otherwise the index of the empty slot where it would be inserted.
// Requires a non-empty, power-of-two capacity (the '& mask' probe step
// silently returns wrong indices otherwise).
static size_t hashset_probe(const hashset_t *set, const char *key, size_t len, uint64_t hash) {
    assert(set->capacity > 0 && (set->capacity & (set->capacity - 1)) == 0);

    size_t mask = set->capacity - 1;
    size_t i = hash & mask;

    while (set->items[i].key != NULL) {
        if (set->items[i].hash == hash && set->items[i].len == len && memcmp(set->items[i].key, key, len) == 0) {
            break;
        }
        i = (i + 1) & mask;
    }

    return i;
}

bool hashset_contains(const hashset_t *set, const char *key, size_t len) {
    if (set->capacity == 0) {
        return false;
    }

    size_t i = hashset_probe(set, key, len, fnv1a(key, len));
    return set->items[i].key != NULL;
}

bool hashset_append(arena_t *arena, hashset_t *set, const char *key, size_t len) {
    hashset_grow(arena, set, 1);

    uint64_t hash = fnv1a(key, len);
    size_t i = hashset_probe(set, key, len, hash);

    if (set->items[i].key != NULL) {
        return false;
    }

    set->items[i].key = key;
    set->items[i].len = len;
    set->items[i].hash = hash;
    ++set->count;
    return true;
}
