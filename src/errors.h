#ifndef ERRORS_H
#define ERRORS_H
#include "result.h"
#include "tty.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static inline result_t operation_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (use_color) {
        fprintf(stderr, RED);
    }
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, args);
    if (use_color) {
        fprintf(stderr, RESET);
    }

    va_end(args);

    return (result_t){.ok = false, .code = 1};
}

static inline result_t usage_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (use_color) {
        fprintf(stderr, RED);
    }
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, args);
    if (use_color) {
        fprintf(stderr, RESET);
    }
    fprintf(stderr, "\nTry 'nvi --help' for more information.\n");

    va_end(args);

    return (result_t){.ok = false, .code = 2};
}

#endif
