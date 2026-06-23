#ifndef ERRORS_H
#define ERRORS_H
#include "result.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static inline result_t operation_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, args);

    va_end(args);

    return (result_t){.ok = false, .errcode = 1};
}

static inline result_t usage_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\nTry 'nvi --help' for more information.\n");

    va_end(args);

    return (result_t){.ok = false, .errcode = 2};
}

#endif
