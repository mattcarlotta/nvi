#ifndef MACROS_H
#define MACROS_H

#define ARR_LEN(a) (sizeof(a) / sizeof((a)[0]))

#define TO_PLURAL_1(a) ((a) != 1 ? "s" : "")
#define TO_PLURAL_3(a, p, s) ((a) != 1 ? (p) : (s))

#define TO_PLURAL_PICK(_1, _2, _3, NAME, ...) NAME
#define TO_PLURAL(...) TO_PLURAL_PICK(__VA_ARGS__, TO_PLURAL_3, TO_PLURAL_2, TO_PLURAL_1, _PAD)(__VA_ARGS__)

#endif
