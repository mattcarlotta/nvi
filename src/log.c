#include "log.h"
#include "tty.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void buf_vappendf(log_buf_t *buf, const char *fmt, va_list args) {
    va_list measure;
    va_copy(measure, args);
    int n = vsnprintf(NULL, 0, fmt, measure);
    va_end(measure);

    if (n < 0) {
        return;
    }

    if (buf->count + (size_t)n + 1 > buf->capacity) {
        size_t cap = buf->capacity ? buf->capacity * 2 : 256;
        while (cap < buf->count + (size_t)n + 1) {
            cap *= 2;
        }
        char *grown = realloc(buf->items, cap);
        if (grown == NULL) {
            log_error("[ERROR] Failed to grow a report buffer (system out of memory?); aborting.\n");
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
        buf->items = grown;
        buf->capacity = cap;
    }

    vsnprintf(buf->items + buf->count, (size_t)n + 1, fmt, args);
    buf->count += (size_t)n;
}

static void buf_appendf(log_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_vappendf(buf, fmt, args);
    va_end(args);
}

static void buf_colored(log_buf_t *buf, const char *color, const char *fmt, va_list args) {
    if (use_color) {
        buf_appendf(buf, "%s", color);
    }
    buf_vappendf(buf, fmt, args);
    if (use_color) {
        buf_appendf(buf, "%s", RESET);
    }
}

void log_buf_info(log_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_colored(buf, CYAN, fmt, args);
    va_end(args);
}
void log_buf_f(log_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_vappendf(buf, fmt, args);
    va_end(args);
}

void log_buf_fi(log_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_colored(buf, LIGHT_MAGENTA ITALIC, fmt, args);
    va_end(args);
}

void log_buf_bold_info(log_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_colored(buf, BOLD_GREEN, fmt, args);
    va_end(args);
}

void log_buf_warning(log_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_colored(buf, YELLOW, fmt, args);
    va_end(args);
}

void log_buf_comment(log_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_colored(buf, DARK_GRAY, fmt, args);
    va_end(args);
}

void log_buf_error(log_buf_t *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    buf_colored(buf, RED, fmt, args);
    va_end(args);
}

void log_buf_flush(log_buf_t *buf) {
    if (buf->count > 0) {
        fwrite(buf->items, 1, buf->count, stderr);
        buf->count = 0;
    }
}

void log_buf_free(log_buf_t *buf) {
    free(buf->items);
    *buf = (log_buf_t){0};
}

void log_f(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void log_fi(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (use_color) {
        fprintf(stderr, LIGHT_MAGENTA);
        fprintf(stderr, ITALIC);
    }
    vfprintf(stderr, fmt, args);
    if (use_color) {
        fprintf(stderr, RESET);
    }

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
