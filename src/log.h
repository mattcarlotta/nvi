#ifndef LOG_H
#define LOG_H

void log_f(const char *fmt, ...);
void log_fi(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_bold_info(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_comment(const char *fmt, ...);

#endif
