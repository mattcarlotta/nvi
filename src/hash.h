#ifndef HASH_H
#define HASH_H

#include <stddef.h>
#include <stdint.h>

uint64_t fnv1a(const char *key, size_t len);

#endif // HASH_H
