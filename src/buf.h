#ifndef BUF_H
#define BUF_H

#include <stddef.h>

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} buf_t;

#endif
