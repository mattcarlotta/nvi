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

extern const unsigned char LITERAL_STOPS[];
extern const size_t LITERAL_STOPS_LEN;
extern const unsigned char STOP_NL[];
extern const size_t STOP_NL_LEN;
extern const unsigned char STOP_BRACE_NL[];
extern const size_t STOP_BRACE_NL_LEN;

#endif
