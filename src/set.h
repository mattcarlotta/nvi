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
    size_t capacity;
    size_t count;
} set_t;

static inline uint64_t set_hash(const char *key, size_t len) {
    // FNV-1a offset basis
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= (unsigned char)key[i];
        // FNV prime
        h *= 1099511628211ULL;
    }
    return h;
}

static inline void set_grow(set_t *set, size_t need) {
    size_t new_cap = set->capacity == 0 ? SET_INIT_CAP : set->capacity;
    // keep load factor under ~0.7
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
