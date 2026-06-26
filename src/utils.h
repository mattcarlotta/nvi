#ifndef UTILS_H
#define UTILS_H
#include "file.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

bool ends_with(const char *name, const char *suffix);
bool is_blacklisted(const char *name);
size_t index_of_scalar(const char *s, size_t len, size_t pos, int ch);
size_t index_of(const file_details_t *file, size_t from, char ch);
bool is_same_line(const file_details_t *file, size_t from, size_t to);
bool is_ident_start(char c);
bool is_ident_char(char c);
bool is_valid_key(const char *key, size_t len);
void fput_repeat(FILE *f, char c, size_t n);

#endif
