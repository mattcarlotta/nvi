#ifndef DYN_ARR_H
#define DYN_ARR_H
#include <stdlib.h>

#define DYN_ARR_INIT_CAP 8

#define DYN_ARR_APPEND(da, item)                                                                                       \
    do {                                                                                                               \
        if ((da)->count == (da)->capacity) {                                                                           \
            size_t _new_cap = (da)->capacity == 0 ? DYN_ARR_INIT_CAP : (da)->capacity * 2;                             \
            void *_p = realloc((da)->items, _new_cap * sizeof(*(da)->items));                                          \
            if (_p == NULL) {                                                                                          \
                abort();                                                                                               \
            }                                                                                                          \
            (da)->items = _p;                                                                                          \
            (da)->capacity = _new_cap;                                                                                 \
        }                                                                                                              \
        (da)->items[(da)->count++] = (item);                                                                           \
    } while (0)

#define DYN_ARR_FREE(da)                                                                                               \
    do {                                                                                                               \
        free((da)->items);                                                                                             \
        (da)->items = NULL;                                                                                            \
        (da)->count = 0;                                                                                               \
        (da)->capacity = 0;                                                                                            \
    } while (0)

#endif
