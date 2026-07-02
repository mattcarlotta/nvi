#ifndef SHIMS_H
#define SHIMS_H

// Windows (MSC) compiler requires shimming "strerror" and "strndup" or else it'll fail to compile.

#if defined(_WIN32) && defined(_MSC_VER)
#include <io.h>
#include <stdlib.h>
#include <string.h>
#define isatty _isatty
#define access _access
#define STDERR_FILENO 2

static inline const char *nvi_strerror(int errnum) {
    static char buf[256];
    strerror_s(buf, sizeof(buf), errnum);
    return buf;
}
#define strerror nvi_strerror

static char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *copy = malloc(len + 1);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}
#else
#include <unistd.h>
#endif

#endif
