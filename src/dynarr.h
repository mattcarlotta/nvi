#ifndef ARENA_DYNARR_H
#define ARENA_DYNARR_H
#include "arena.h"

#define DYN_ARR_INIT_CAP 8

// Arena-backed append. The buffer is grown via arena_extend and never freed individually;
// it lives until arena_reset/arena_free. `da` must be zero-initialized before first use.
#define ARENA_DYN_ARR_APPEND(arena, da, item)                                                                          \
    do {                                                                                                               \
        if ((da)->count == (da)->capacity) {                                                                           \
            size_t _new_cap = (da)->capacity == 0 ? DYN_ARR_INIT_CAP : (da)->capacity * 2;                             \
            (da)->items = arena_extend((arena), (da)->items, (da)->capacity * sizeof(*(da)->items),                    \
                                       _new_cap * sizeof(*(da)->items));                                               \
            (da)->capacity = _new_cap;                                                                                 \
        }                                                                                                              \
        (da)->items[(da)->count++] = (item);                                                                           \
    } while (0)

#define ARENA_DYN_ARR_APPEND_MANY(arena, da, src, n)                                                                   \
    do {                                                                                                               \
        const void *_src = (src);                                                                                      \
        size_t _n = (n);                                                                                               \
        if (_n != 0) {                                                                                                 \
            if ((da)->count + _n > (da)->capacity) {                                                                   \
                size_t _new_cap = (da)->capacity == 0 ? DYN_ARR_INIT_CAP : (da)->capacity;                             \
                while (_new_cap < (da)->count + _n) {                                                                  \
                    _new_cap *= 2;                                                                                     \
                }                                                                                                      \
                (da)->items = arena_extend((arena), (da)->items, (da)->capacity * sizeof(*(da)->items),                \
                                           _new_cap * sizeof(*(da)->items));                                           \
                (da)->capacity = _new_cap;                                                                             \
            }                                                                                                          \
            memcpy((da)->items + (da)->count, _src, _n * sizeof(*(da)->items));                                        \
            (da)->count += _n;                                                                                         \
        }                                                                                                              \
    } while (0)

// Optional: pre-size when a bound is known, so appends never grow (zero abandoned buffers).
#define ARENA_DYN_ARR_RESERVE(arena, da, want)                                                                         \
    do {                                                                                                               \
        size_t _want = (want);                                                                                         \
        if (_want > (da)->capacity) {                                                                                  \
            (da)->items = arena_extend((arena), (da)->items, (da)->capacity * sizeof(*(da)->items),                    \
                                       _want * sizeof(*(da)->items));                                                  \
            (da)->capacity = _want;                                                                                    \
        }                                                                                                              \
    } while (0)

#endif // ARENA_DYNARR_H
