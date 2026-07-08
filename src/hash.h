#ifndef HASH_H
#define HASH_H
#include <stddef.h>
#include <stdint.h>

// FNV-1a (Fowler-Noll-Vo), 64-bit, non-cryptographic. Lives here so a single translation
// unit can include both set.h and map.h without two conflicting static inline fnv1a
// definitions. The two constants are fixed by the algorithm:
//   64-bit offset basis: 14695981039346656037
//   64-bit prime:        1099511628211
#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

static inline uint64_t fnv1a(const char *key, size_t len) {
    uint64_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; ++i) {
        h ^= (unsigned char)key[i];
        h *= FNV_PRIME;
    }
    return h;
}

#endif // HASH_H
