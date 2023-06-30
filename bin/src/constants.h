#ifndef NVI_CONSTANTS_H
#define NVI_CONSTANTS_H

namespace nvi {
namespace constants {

// arg
const int ARG_CONFIG_ERROR = 0;
const int ARG_DIR_ERROR = 1;
const int ARG_FILES_ERROR = 2;
const int ARG_REQUIRED_ERROR = 3;
const int ARG_HELP_DOC = 4;

// parser
const char COMMENT = '#';         // 0x23
const char LINE_DELIMITER = '\n'; // 0x0a
const char BACK_SLASH = '\\';     // 0x5c
const char ASSIGN_OP = '=';       // 0x3d
const char DOLLAR_SIGN = '$';     // 0x24
const char OPEN_BRACE = '{';      // 0x7b
const char CLOSE_BRACE = '}';     // 0x7d
} // namespace constants
}; // namespace nvi

#endif
