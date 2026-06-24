#include "chars.h"
#include "macros.h"

const unsigned char LITERAL_STOPS[] = {
    NULL_CHAR, LINE_DELIMITER, ASSIGN_OP, HASH, DOLLAR_SIGN, BACK_SLASH,
};
const size_t LITERAL_STOPS_LEN = ARR_LEN(LITERAL_STOPS);

const unsigned char STOP_NL[] = {LINE_DELIMITER};
const size_t STOP_NL_LEN = ARR_LEN(STOP_NL);

const unsigned char STOP_BRACE_NL[] = {CLOSE_BRACE, LINE_DELIMITER};
const size_t STOP_BRACE_NL_LEN = ARR_LEN(STOP_BRACE_NL);
