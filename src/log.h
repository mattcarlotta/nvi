#ifndef LOG_H
#define LOG_H
#include "buf.h"
#include <stdio.h>

typedef struct {
    buf_t *buf;
    FILE *file;
} sink_t;

#define SINK_STDERR ((sink_t){.file = stderr})
#define SINK_BUF(b) ((sink_t){.buf = (b)})

void log_f(sink_t s, const char *fmt, ...);
void log_fi(sink_t s, const char *fmt, ...);
void log_info(sink_t s, const char *fmt, ...);
void log_bold_info(sink_t s, const char *fmt, ...);
void log_warning(sink_t s, const char *fmt, ...);
void log_comment(sink_t s, const char *fmt, ...);
void log_error(sink_t s, const char *fmt, ...);

void log_buf_flush(buf_t *buf);
void log_buf_free(buf_t *buf);

#endif // LOG_H
