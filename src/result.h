#ifndef RESULT_H
#define RESULT_H
#include <stdbool.h>

typedef struct {
    bool ok;
    int errcode;
} result_t;

#endif
