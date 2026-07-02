#ifndef DYN_ARR_H
#define DYN_ARR_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DYN_ARR_INIT_CAP 8

#define DYN_ARR_APPEND(da, item)                                                                                       \
    do {                                                                                                               \
        if ((da)->count == (da)->capacity) {                                                                           \
            size_t _new_cap = (da)->capacity == 0 ? DYN_ARR_INIT_CAP : (da)->capacity * 2;                             \
            void *_p = realloc((da)->items, _new_cap * sizeof(*(da)->items));                                          \
            if (_p == NULL) {                                                                                          \
                fprintf(stderr, "[ERROR] Failed to reallocate memory (system of out memory?); aborting.\n");           \
                fflush(stderr);                                                                                        \
                abort();                                                                                               \
            }                                                                                                          \
            (da)->items = _p;                                                                                          \
            (da)->capacity = _new_cap;                                                                                 \
        }                                                                                                              \
        (da)->items[(da)->count++] = (item);                                                                           \
    } while (0)

#define DYN_ARR_APPEND_MANY(da, src, n)                                                                                \
    do {                                                                                                               \
        const void *_src = (src);                                                                                      \
        size_t _n = (n);                                                                                               \
        if (_n != 0) {                                                                                                 \
            if ((da)->count + _n > (da)->capacity) {                                                                   \
                size_t _new_cap = (da)->capacity == 0 ? DYN_ARR_INIT_CAP : (da)->capacity;                             \
                while (_new_cap < (da)->count + _n) {                                                                  \
                    _new_cap *= 2;                                                                                     \
                }                                                                                                      \
                void *_p = realloc((da)->items, _new_cap * sizeof(*(da)->items));                                      \
                if (_p == NULL) {                                                                                      \
                    fprintf(stderr, "[ERROR] Failed to reallocate memory (system out of memory?); aborting.\n");       \
                    fflush(stderr);                                                                                    \
                    abort();                                                                                           \
                }                                                                                                      \
                (da)->items = _p;                                                                                      \
                (da)->capacity = _new_cap;                                                                             \
            }                                                                                                          \
            memcpy((da)->items + (da)->count, _src, _n * sizeof(*(da)->items));                                        \
            (da)->count += _n;                                                                                         \
        }                                                                                                              \
    } while (0)

#define DYN_ARR_FREE(da)                                                                                               \
    do {                                                                                                               \
        free((da)->items);                                                                                             \
        (da)->items = NULL;                                                                                            \
        (da)->count = 0;                                                                                               \
        (da)->capacity = 0;                                                                                            \
    } while (0)

#endif
