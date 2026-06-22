#ifndef ERRORS_H
#define ERRORS_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int usage_error(const char *fmt, const char *arg) {
    fprintf(stderr, "error: ");
    fprintf(stderr, fmt, arg);
    fprintf(stderr, "\nTry 'nvi --help' for more information.\n");
    return 2;
}

#endif
