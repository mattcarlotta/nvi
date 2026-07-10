#include "log.h"
#include "arena.h"
#include "buf.h"
#include "tty.h"
#include <stdarg.h>
#include <stdio.h>

static void buf_vappendf(buf_t *buf, const char *fmt, va_list args) {
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
        buf->items = arena_extend(buf->arena, buf->items, buf->capacity, cap);
        buf->capacity = cap;
    }

    vsnprintf(buf->items + buf->count, (size_t)n + 1, fmt, args);
    buf->count += (size_t)n;
}

static void sink_vappendf(sink_t s, const char *fmt, va_list args) {
    if (s.buf) {
        buf_vappendf(s.buf, fmt, args);
        return;
    }

    vfprintf(s.file, fmt, args);
}

static void sink_appendf(sink_t s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sink_vappendf(s, fmt, args);
    va_end(args);
}

static void sink_colored(sink_t s, const char *color, const char *fmt, va_list args) {
    if (use_color) {
        sink_appendf(s, "%s", color);
    }
    sink_vappendf(s, fmt, args);
    if (use_color) {
        sink_appendf(s, "%s", RESET);
    }
}

void log_f(sink_t s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sink_vappendf(s, fmt, args);
    va_end(args);
}

void log_fi(sink_t s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sink_colored(s, LIGHT_MAGENTA ITALIC, fmt, args);
    va_end(args);
}

void log_info(sink_t s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sink_colored(s, CYAN, fmt, args);
    va_end(args);
}

void log_bold_info(sink_t s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sink_colored(s, BOLD_GREEN, fmt, args);
    va_end(args);
}

void log_warning(sink_t s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sink_colored(s, YELLOW, fmt, args);
    va_end(args);
}

void log_comment(sink_t s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sink_colored(s, DARK_GRAY, fmt, args);
    va_end(args);
}

void log_error(sink_t s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sink_colored(s, RED, fmt, args);
    va_end(args);
}

void log_buf_flush(buf_t *buf) {
    if (buf->count > 0) {
        fwrite(buf->items, 1, buf->count, stderr);
        buf->count = 0;
    }
}
