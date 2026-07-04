#ifndef MACROS_H
#define MACROS_H

#define EXPAND(x) x

#define FLAG_2(a, v) {(a), (v)}
#define FLAG_3(a, b, v)                                                                                                \
    {(a), (v)}, { (b), (v) }
#define FLAG_4(a, b, c, v)                                                                                             \
    {(a), (v)}, {(b), (v)}, { (c), (v) }

#define FLAG_PICK(_1, _2, _3, _4, NAME, ...) NAME
#define FLAG(...) EXPAND(FLAG_PICK(__VA_ARGS__, FLAG_4, FLAG_3, FLAG_2, FLAG_BAD_ARITY)(__VA_ARGS__))

#define ARR_LEN(a) (sizeof(a) / sizeof((a)[0]))

#define ENTRY(ext, arr) {(ext), (arr), ARR_LEN(arr)}
#define ACCESSOR(str, pat) {.prefix = (str), .prefix_len = sizeof("" str) - 1, .pattern = (pat)}

#define TO_PLURAL_1(a) ((a) != 1 ? "s" : "")
#define TO_PLURAL_3(a, p, s) ((a) != 1 ? (p) : (s))

#define TO_PLURAL_PICK(_1, _2, _3, NAME, ...) NAME
#define TO_PLURAL(...)                                                                                                 \
    EXPAND(TO_PLURAL_PICK(__VA_ARGS__, TO_PLURAL_3, TO_PLURAL_BAD_ARITY, TO_PLURAL_1, _PAD)(__VA_ARGS__))

#endif
