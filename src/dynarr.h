#ifndef DYN_ARR_H
#define DYN_ARR_H
#include "arena.h"
#include <string.h>

// Arena-backed dynamic arrays. Growth goes through arena_extend: in-place when the array is
// the arena's most recent allocation, otherwise a fresh allocation plus copy that orphans
// the old capacity into the arena. Orphaned bytes are reclaimed when the owning arena is
// reset or freed, never individually.

#define DYN_ARR_INIT_CAP 8

#define DYN_ARR_APPEND(arena, da, item)                                                                                \
    do {                                                                                                               \
        if ((da)->count == (da)->capacity) {                                                                           \
            size_t _new_cap = (da)->capacity == 0 ? DYN_ARR_INIT_CAP : (da)->capacity * 2;                             \
            void *_p = arena_extend((arena), (da)->items, (da)->capacity * sizeof(*(da)->items),                       \
                                    _new_cap * sizeof(*(da)->items));                                                  \
            (da)->items = _p;                                                                                          \
            (da)->capacity = _new_cap;                                                                                 \
        }                                                                                                              \
        (da)->items[(da)->count++] = (item);                                                                           \
    } while (0)

#define DYN_ARR_APPEND_MANY(arena, da, src, n)                                                                         \
    do {                                                                                                               \
        const void *_src = (src);                                                                                      \
        size_t _n = (n);                                                                                               \
        if (_n != 0) {                                                                                                 \
            if ((da)->count + _n > (da)->capacity) {                                                                   \
                size_t _new_cap = (da)->capacity == 0 ? DYN_ARR_INIT_CAP : (da)->capacity;                             \
                while (_new_cap < (da)->count + _n) {                                                                  \
                    _new_cap *= 2;                                                                                     \
                }                                                                                                      \
                void *_p = arena_extend((arena), (da)->items, (da)->capacity * sizeof(*(da)->items),                   \
                                        _new_cap * sizeof(*(da)->items));                                              \
                (da)->items = _p;                                                                                      \
                (da)->capacity = _new_cap;                                                                             \
            }                                                                                                          \
            memcpy((da)->items + (da)->count, _src, _n * sizeof(*(da)->items));                                        \
            (da)->count += _n;                                                                                         \
        }                                                                                                              \
    } while (0)

#endif
