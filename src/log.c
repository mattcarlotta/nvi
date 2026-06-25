#include "tty.h"
#include <stdarg.h>
#include <stdio.h>

void log_f(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (use_color) {
        fprintf(stderr, CYAN);
    }
    vfprintf(stderr, fmt, args);
    if (use_color) {
        fprintf(stderr, RESET);
    }

    va_end(args);
}

void log_bold_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (use_color) {
        fprintf(stderr, BOLD_GREEN);
    }
    vfprintf(stderr, fmt, args);
    if (use_color) {
        fprintf(stderr, RESET);
    }

    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (use_color) {
        fprintf(stderr, RED);
    }
    vfprintf(stderr, fmt, args);
    if (use_color) {
        fprintf(stderr, RESET);
    }

    va_end(args);
}

void log_warning(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (use_color) {
        fprintf(stderr, YELLOW);
    }
    vfprintf(stderr, fmt, args);
    if (use_color) {
        fprintf(stderr, RESET);
    }

    va_end(args);
}

void log_comment(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (use_color) {
        fprintf(stderr, DARK_GRAY);
    }
    vfprintf(stderr, fmt, args);
    if (use_color) {
        fprintf(stderr, RESET);
    }

    va_end(args);
}
