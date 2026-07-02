#include "chars.h"
#include "macros.h"

const unsigned char LITERAL_STOPS[] = {
    NULL_CHAR, LINE_DELIMITER, CARRIAGE_RETURN, ASSIGN_OP, HASH, DOLLAR_SIGN, BACK_SLASH,
};
const size_t LITERAL_STOPS_LEN = ARR_LEN(LITERAL_STOPS);

// Inside double quotes '$' (interpolation) and '\' (continuation) stay active;
// '=' and '#' are literal after a key
const unsigned char DQ_STOPS[] = {
    NULL_CHAR, LINE_DELIMITER, CARRIAGE_RETURN, DOUBLE_QUOTE, DOLLAR_SIGN, BACK_SLASH,
};
const size_t DQ_STOPS_LEN = ARR_LEN(DQ_STOPS);

// Inside single quotes everything is literal until the closing quote.
const unsigned char SQ_STOPS[] = {
    NULL_CHAR,
    LINE_DELIMITER,
    CARRIAGE_RETURN,
    SINGLE_QUOTE,
};
const size_t SQ_STOPS_LEN = ARR_LEN(SQ_STOPS);

const unsigned char STOP_NL[] = {LINE_DELIMITER, CARRIAGE_RETURN};
const size_t STOP_NL_LEN = ARR_LEN(STOP_NL);

const unsigned char STOP_BRACE_NL[] = {NULL_CHAR, CLOSE_BRACE, LINE_DELIMITER, CARRIAGE_RETURN};
const size_t STOP_BRACE_NL_LEN = ARR_LEN(STOP_BRACE_NL);
