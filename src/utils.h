#ifndef UTILS_H
#define UTILS_H
#include <stdbool.h>
#include <stddef.h>

size_t index_of(const char *contents, size_t len, size_t from, char ch);
bool is_same_line(const char *contents, size_t len, size_t from, size_t to);
bool is_ident_char(char c);
bool ends_with(const char *name, const char *suffix);
bool is_valid_key(const char *key, size_t len);

#endif
