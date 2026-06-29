// test_capture.h
#ifndef TEST_CAPTURE_H
#define TEST_CAPTURE_H

#ifdef _WIN32
#include <io.h>
#define DUP _dup
#define DUP2 _dup2
#define FILENO _fileno
#define CLOSE _close
#else
#include <unistd.h>
#define DUP dup
#define DUP2 dup2
#define FILENO fileno
#define CLOSE close
#endif

#include "unity.h"
#include <stdio.h>

static inline size_t capture_fd(FILE *stream, char *out, size_t cap, void (*fn)(void *), void *ctx) {
    fflush(stream);
    int saved = DUP(FILENO(stream));
    FILE *tmp = tmpfile();
    TEST_ASSERT_NOT_NULL(tmp);

    DUP2(FILENO(tmp), FILENO(stream));

    fn(ctx);

    fflush(stream);
    DUP2(saved, FILENO(stream));
    CLOSE(saved);

    rewind(tmp);
    size_t n = fread(out, 1, cap, tmp);
    fclose(tmp);
    return n;
}

#endif
