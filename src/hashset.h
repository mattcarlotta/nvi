#ifndef HASHSET_H
#define HASHSET_H

#include "arena.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Linear probing hash set of borrowed string pointers.
typedef struct {
    const char *key;
    size_t len;
    uint64_t hash;
} hashset_entry_t;

typedef struct {
    hashset_entry_t *items;
    size_t capacity;
    size_t count;
} hashset_t;

bool hashset_contains(const hashset_t *set, const char *key, size_t len);

// Borrows key. Returns true if the key was inserted, false if it was already present.
bool hashset_append(arena_t *arena, hashset_t *set, const char *key, size_t len);

#endif // HASHSET_H
