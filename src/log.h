#ifndef LOG_H
#define LOG_H
#include <stddef.h>

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} log_buf_t;

void log_buf_f(log_buf_t *buf, const char *fmt, ...);
void log_buf_fi(log_buf_t *buf, const char *fmt, ...);
void log_buf_info(log_buf_t *buf, const char *fmt, ...);
void log_buf_bold_info(log_buf_t *buf, const char *fmt, ...);
void log_buf_comment(log_buf_t *buf, const char *fmt, ...);
void log_buf_warning(log_buf_t *buf, const char *fmt, ...);
void log_buf_flush(log_buf_t *buf);
void log_buf_free(log_buf_t *buf);

void log_f(const char *fmt, ...);
void log_fi(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_bold_info(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_comment(const char *fmt, ...);

#endif
