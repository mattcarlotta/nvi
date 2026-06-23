#ifndef CHARS_H
#define CHARS_H

#include <stddef.h>

enum {
    CHAR_A = 'A',
    CHAR_Z = 'Z',
    CHAR_ZERO = '0',
    CHAR_NINE = '9',
    NULL_CHAR = 0,
    LINE_DELIMITER = '\n',
    ASSIGN_OP = '=',
    HASH = '#',
    DASH = '-',
    UNDERLINE = '_',
    FORWARD_SLASH = '/',
    DOLLAR_SIGN = '$',
    OPEN_BRACE = '{',
    CLOSE_BRACE = '}',
    CLOSE_PAREN = ')',
    BACK_SLASH = '\\',
    DOUBLE_QUOTE = '"',
    SINGLE_QUOTE = '\'',
    TAB = '\t',
    SPACE = ' ',
    DOT = '.',
};

extern const char SPECIAL_CHARS[];
extern const size_t SPECIAL_CHARS_LEN;

#endif
