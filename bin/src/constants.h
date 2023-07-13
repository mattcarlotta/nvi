#ifndef NVI_CONSTANTS_H
#define NVI_CONSTANTS_H

namespace nvi {
namespace constants {

// arg
constexpr unsigned int ARG_CONFIG_FLAG_ERROR = 0;
constexpr unsigned int ARG_DIR_FLAG_ERROR = 1;
constexpr unsigned int ARG_COMMAND_FLAG_ERROR = 2;
constexpr unsigned int ARG_FILES_FLAG_ERROR = 3;
constexpr unsigned int ARG_REQUIRED_FLAG_ERROR = 4;
constexpr unsigned int ARG_HELP_DOC = 5;
constexpr unsigned int ARG_INVALID_FLAG_WARNING = 6;
constexpr unsigned int ARG_EXCEPTION = 7;
constexpr unsigned int ARG_DEBUG = 8;
constexpr unsigned int ARG_NO_PARSEABLE_ENVS = 9;
constexpr unsigned int ARG_COMMAND_WHICH_NOT_FOUND_ERROR = 10;
constexpr unsigned int ARG_COMMAND_BIN_NOT_FOUND_ERROR = 11;

// config
constexpr unsigned int CONFIG_FILE_ERROR = 0;
constexpr unsigned int CONFIG_FILE_PARSE_ERROR = 1;
constexpr unsigned int CONFIG_DEBUG = 2;
constexpr unsigned int CONFIG_MISSING_FILES_ARG_ERROR = 3;

// parser
constexpr unsigned int PARSER_INTERPOLATION_WARNING = 0;
constexpr unsigned int PARSER_INTERPOLATION_ERROR = 1;
constexpr unsigned int PARSER_DEBUG = 2;
constexpr unsigned int PARSER_DEBUG_FILE_PROCESSED = 3;
constexpr unsigned int PARSER_REQUIRED_ENV_ERROR = 4;
constexpr unsigned int PARSER_FILE_ERROR = 5;
constexpr unsigned int PARSER_EMPTY_ENVS = 6;
constexpr char COMMENT = '#';         // 0x23
constexpr char LINE_DELIMITER = '\n'; // 0x0a
constexpr char BACK_SLASH = '\\';     // 0x5c
constexpr char ASSIGN_OP = '=';       // 0x3d
constexpr char DOLLAR_SIGN = '$';     // 0x24
constexpr char OPEN_BRACE = '{';      // 0x7b
constexpr char CLOSE_BRACE = '}';     // 0x7d
constexpr char DOUBLE_QUOTE = '"';    // 0x22
} // namespace constants
}; // namespace nvi

#endif
