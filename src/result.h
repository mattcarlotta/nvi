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

static const result_t RESULT_OK = {.ok = true, .code = 0};
static const result_t EXIT_EARLY = {.ok = false, .code = 0};
static const result_t OPERATION_FAILURE = {.ok = false, .code = 1};
static const result_t USAGE_FAILURE = {.ok = false, .code = 2};

#endif // RESULT_H
