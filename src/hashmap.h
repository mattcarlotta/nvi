#ifndef HASHMAP_H
#define HASHMAP_H

#include "arena.h"
#include <stddef.h>
#include <stdint.h>

// Linear probing hash map of borrowed string pointers to values.
#define HASHMAP_NOT_FOUND SIZE_MAX

typedef struct {
    const char *key;
    size_t len;
    uint64_t hash;
    size_t value;
} hashmap_entry_t;

typedef struct {
    hashmap_entry_t *items;
    size_t capacity;
    size_t count;
} hashmap_t;

// Returns the stored value, or HASHMAP_NOT_FOUND if the key is absent.
size_t hashmap_get(const hashmap_t *map, const char *key, size_t len);

// Borrows key. Overwrites the value if the key is already present.
void hashmap_append(arena_t *arena, hashmap_t *map, const char *key, size_t len, size_t value);

#endif // HASHMAP_H
