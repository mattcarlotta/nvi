#ifndef NVI_CONSTANTS_H
#define NVI_CONSTANTS_H

namespace nvi {
namespace constants {

// arg
const unsigned int ARG_CONFIG_FLAG_ERROR = 0;
const unsigned int ARG_DIR_FLAG_ERROR = 1;
const unsigned int ARG_FILES_FLAG_ERROR = 2;
const unsigned int ARG_REQUIRED_FLAG_ERROR = 3;
const unsigned int ARG_HELP_DOC = 4;
const unsigned int ARG_INVALID_ARG_WARNING = 5;
const unsigned int ARG_EXCEPTION = 6;
const unsigned int ARG_DEBUG = 7;

// config
const unsigned int CONFIG_FILE_ERROR = 0;
const unsigned int CONFIG_FILE_PARSE_ERROR = 1;
const unsigned int CONFIG_DEBUG = 3;
const unsigned int CONFIG_MISSING_FILES_ARG_ERROR = 4;

// parser
const unsigned int PARSER_INTERPOLATION_WARNING = 0;
const unsigned int PARSER_INTERPOLATION_ERROR = 1;
const unsigned int PARSER_DEBUG = 2;
const unsigned int PARSER_DEBUG_FILE_PROCESSED = 3;
const unsigned int PARSER_REQUIRED_ENV_ERROR = 4;
const unsigned int PARSER_FILE_ERROR = 5;
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
