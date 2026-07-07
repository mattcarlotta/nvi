#ifndef FORMAT_H
#define FORMAT_H
#include <string.h>

typedef enum { FORMAT_POWERSHELL, FORMAT_NULL, FORMAT_UNKNOWN } format_t;

static inline format_t get_default_format(void) {
#if defined(_WIN32) && defined(_MSC_VER)
    return FORMAT_POWERSHELL;
#else
    return FORMAT_NULL;
#endif
}

static inline const char *get_format_name(const format_t f) {
    switch (f) {
        case FORMAT_POWERSHELL: {
            return "powershell";
        }
        case FORMAT_NULL: {
            return "nul";
        }
        default:
            return "unknown";
    }
}

static inline format_t get_format(const char *arg) {
    if (strcmp(arg, "powershell") == 0) {
        return FORMAT_POWERSHELL;
    }

    if (strcmp(arg, "nul") == 0) {
        return FORMAT_NULL;
    }

    return FORMAT_UNKNOWN;
}

#endif // FORMAT_H
