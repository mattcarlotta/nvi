#ifndef FORMAT_H
#define FORMAT_H
#include <string.h>

typedef enum { FORMAT_NULL, FORMAT_POWERSHELL, FORMAT_UNKNOWN } format_t;

typedef struct {
    const char *name;
    format_t value;
} formats_t;

static const formats_t formats[] = {{.name = "nul", .value = FORMAT_NULL},
                                    {.name = "powershell", .value = FORMAT_POWERSHELL},
                                    {.name = "unknown format", .value = FORMAT_UNKNOWN}};

static inline format_t get_default_format(void) {
#ifdef _WIN32
    return FORMAT_POWERSHELL;
#else
    return FORMAT_NULL;
#endif
}

static inline const char *format_name(format_t f) {
    if (f < 0 || f >= 3)
        return "unknown";

    return formats[f].name;
}

static inline format_t get_format(const char *arg) {
    size_t n = sizeof(formats) / sizeof(formats[0]);

    for (size_t i = 0; i < n; ++i) {
        if (strcmp(arg, formats[i].name) == 0) {
            return formats[i].value;
        }
    }

    return FORMAT_UNKNOWN;
}

#endif
