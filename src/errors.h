#ifndef ERRORS_H
#define ERRORS_H
#include "result.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static result_t usage_error(const char *fmt, const char *arg) {
    fprintf(stderr, "error: ");
    fprintf(stderr, fmt, arg);
    fprintf(stderr, "\nTry 'nvi --help' for more information.\n");

    result_t r = {.ok = false, .errcode = 2};
    return r;
}

#endif
