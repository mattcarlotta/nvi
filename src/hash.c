#include "hash.h"

#define FNV_OFFSET 1469598103934665603ULL
#define FNV_PRIME 1099511628211ULL

// This is FNV-1a (Fowler-Noll-Vo), a non-cryptographic hash function.
// The two magic constants are specified by the algorithm:
// Magic Offset basis: 1469598103934665603 (decimal) -- this is the seed
// Magic Prime: 1099511628211 -- chosen for good avalanche properties over 64-bit integers
uint64_t fnv1a(const char *key, size_t len) {
    uint64_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; ++i) {
        h ^= (unsigned char)key[i];
        h *= FNV_PRIME;
    }
    return h;
}
