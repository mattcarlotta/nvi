#ifndef FORMAT_H
#define FORMAT_H

typedef enum {
    FORMAT_NULL,
    FORMAT_POWERSHELL,
} format_t;

static inline format_t get_default_format(void) {
#ifdef _WIN32
    return FORMAT_POWERSHELL;
#else
    return FORMAT_NULL;
#endif
}

#endif
