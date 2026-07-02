#ifndef RESULT_H
#define RESULT_H
#include <stdbool.h>

// Probably not needed, but to keep all functions testable, this result type is an
// easy way to bubble up an error into main (avoiding a direct exit). It also allows
// results to be "not ok" with an exit 0 (such as printing help or version to stdout)
// and freeing up any malloc'd variables before the process exits.

typedef struct {
    bool ok;
    int code;
} result_t;

#endif
