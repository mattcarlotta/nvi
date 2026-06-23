#ifndef DA_H
#define DA_H
#include <stdlib.h>

#define DA_INIT_CAP 8

#define DA_APPEND(da, item)                                                                                            \
    do {                                                                                                               \
        if ((da)->count == (da)->capacity) {                                                                           \
            size_t _new_cap = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity * 2;                                  \
            void *_p = realloc((da)->items, _new_cap * sizeof(*(da)->items));                                          \
            if (_p == NULL) {                                                                                          \
                abort();                                                                                               \
            }                                                                                                          \
            (da)->items = _p;                                                                                          \
            (da)->capacity = _new_cap;                                                                                 \
        }                                                                                                              \
        (da)->items[(da)->count++] = (item);                                                                           \
    } while (0)

#endif
