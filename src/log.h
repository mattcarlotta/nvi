#ifndef NVI_LOG_H
#define NVI_LOG_H

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

namespace nvi {
    typedef enum LOGGER { API, ARG, CONFIG, GENERATOR, LEXER, PARSER, UNKNOWN_LOG } logger_t;

    typedef enum MESSAGES {
        // api
        INVALID_INPUT_KEY,
        INVALID_INPUT_SELECTION,
        REQUEST_ERROR,
        RESPONSE_ERROR,
        RESPONSE_SUCCESS,
        CURL_FAILED_TO_INIT,
        SAVED_ENV_FILE,
        INVALID_ENV_FILE,

        // arg
        CONFIG_FLAG_ERROR,
        DIR_FLAG_ERROR,
        COMMAND_FLAG_ERROR,
        ENV_FLAG_ERROR,
        FILES_FLAG_ERROR,
        PROJECT_FLAG_ERROR,
        REQUIRED_FLAG_ERROR,
        HELP_DOC,
        INVALID_FLAG_WARNING,
        NVI_VERSION,
        DEBUG_ARG,
        CONFLICTING_COMMAND_FLAG,
        CONFLICTING_CONFIG_FLAG,

        // config
        FILE_PARSE_ERROR,
        SELECTED_CONFIG_EMPTY_ERROR,
        API_ARG_ERROR,
        DEBUG_ARG_ERROR,
        DIR_ARG_ERROR,
        ENV_ARG_ERROR,
        FILES_ARG_ERROR,
        EMPTY_FILES_ARG_ERROR,
        EXEC_ARG_ERROR,
        PRINT_ARG_ERROR,
        PROJECT_ARG_ERROR,
        REQUIRED_ARG_ERROR,
        SAVE_ARG_ERROR,
        INVALID_PROPERTY_WARNING,
        INVALID_ARRAY_VALUE,
        INVALID_BOOLEAN_VALUE,
        INVALID_STRING_VALUE,
        DEBUG_CONFIG_TOKENS,
        DEBUG_CONFIG,
        CONFIG_FILE_ERROR,

        // parser
        EMPTY_KEY_WARNING,
        INTERPOLATION_WARNING,
        INTERPOLATION_ERROR,
        DEBUG_FILE_PROCESSED,
        DEBUG_RESPONSE_PROCESSED,
        REQUIRED_ENV_ERROR,
        EMPTY_ENVS_ERROR,
        EMPTY_RESPONSE_ENVS_ERROR,
        DEBUG_PARSER,

        // generator
        COMMAND_ENOENT_ERROR,
        COMMAND_FAILED_TO_RUN,
        NO_ACTION_ERROR,

        // lexer,
        DEBUG_LEXER,
        LEXER_FILE_ERROR,
        LEXER_FILE_ENOENT_ERROR,
        FILE_EXTENSION_ERROR,

        // shared
        FILE_ENOENT_ERROR,
    } messages_t;

    inline const std::string _get_logger_from_code(const unsigned int &log_code) {
        switch (log_code) {
        case API:
            return "Api";
        case ARG:
            return "Arg";
        case CONFIG:
            return "Config";
        case GENERATOR:
            return "Generator";
        case LEXER:
            return "Lexer";
        case PARSER:
            return "Parser";
        default:
            return "Unknown";
        }
    }

    inline const std::string _get_string_from_code(const unsigned int &error) {
        switch (error) {
        case INVALID_INPUT_KEY:
            return "INVALID_INPUT_KEY";
        case INVALID_INPUT_SELECTION:
            return "INVALID_INPUT_SELECTION";
        case REQUEST_ERROR:
            return "REQUEST_ERROR";
        case RESPONSE_ERROR:
            return "RESPONSE_ERROR";
        case RESPONSE_SUCCESS:
            return "RESPONSE_SUCCESS";
        case CURL_FAILED_TO_INIT:
            return "CURL_FAILED_TO_INIT";
        case SAVED_ENV_FILE:
            return "SAVED_ENV_FILE";
        case INVALID_ENV_FILE:
            return "INVALID_ENV_FILE";
        case CONFIG_FLAG_ERROR:
            return "CONFIG_FLAG_ERROR";
        case DIR_FLAG_ERROR:
            return "DIR_FLAG_ERROR";
        case COMMAND_FLAG_ERROR:
            return "COMMAND_FLAG_ERROR";
        case ENV_FLAG_ERROR:
            return "ENV_FLAG_ERROR";
        case FILES_FLAG_ERROR:
            return "FILES_FLAG_ERROR";
        case PROJECT_FLAG_ERROR:
            return "PROJECT_FLAG_ERROR";
        case REQUIRED_FLAG_ERROR:
            return "REQUIRED_FLAG_ERROR";
        case HELP_DOC:
            return "HELP_DOC";
        case INVALID_FLAG_WARNING:
            return "INVALID_FLAG_WARNING";
        case FILE_PARSE_ERROR:
            return "FILE_PARSE_ERROR";
        case SELECTED_CONFIG_EMPTY_ERROR:
            return "SELECTED_CONFIG_EMPTY_ERROR";
        case DEBUG_PARSER:
            return "DEBUG_PARSER";
        case API_ARG_ERROR:
            return "API_ARG_ERROR";
        case DEBUG_ARG_ERROR:
            return "DEBUG_ARG_ERROR";
        case DIR_ARG_ERROR:
            return "DIR_ARG_ERROR";
        case ENV_ARG_ERROR:
            return "ENV_ARG_ERROR";
        case FILES_ARG_ERROR:
            return "FILES_ARG_ERROR";
        case EMPTY_FILES_ARG_ERROR:
            return "EMPTY_FILES_ARG_ERROR";
        case EXEC_ARG_ERROR:
            return "EXEC_ARG_ERROR";
        case PRINT_ARG_ERROR:
            return "PRINT_ARG_ERROR";
        case PROJECT_ARG_ERROR:
            return "PROJECT_ARG_ERROR";
        case REQUIRED_ARG_ERROR:
            return "REQUIRED_ARG_ERROR";
        case SAVE_ARG_ERROR:
            return "SAVE_ARG_ERROR";
        case INVALID_PROPERTY_WARNING:
            return "INVALID_PROPERTY_WARNING";
        case INVALID_ARRAY_VALUE:
            return "INVALID_ARRAY_VALUE";
        case INVALID_BOOLEAN_VALUE:
            return "INVALID_BOOLEAN_VALUE";
        case INVALID_STRING_VALUE:
            return "INVALID_STRING_VALUE";
        case DEBUG_CONFIG_TOKENS:
            return "DEBUG_CONFIG_TOKENS";
        case DEBUG_CONFIG:
            return "DEBUG_CONFIG";
        case CONFIG_FILE_ERROR:
            return "CONFIG_FILE_ERROR";
        case EMPTY_KEY_WARNING:
            return "EMPTY_KEY_WARNING";
        case INTERPOLATION_WARNING:
            return "INTERPOLATION_WARNING";
        case INTERPOLATION_ERROR:
            return "INTERPOLATION_ERROR";
        case DEBUG_FILE_PROCESSED:
            return "DEBUG_FILE_PROCESSED";
        case DEBUG_RESPONSE_PROCESSED:
            return "DEBUG_RESPONSE_PROCESSED";
        case REQUIRED_ENV_ERROR:
            return "REQUIRED_ENV_ERROR";
        case FILE_EXTENSION_ERROR:
            return "FILE_EXTENSION_ERROR";
        case EMPTY_RESPONSE_ENVS_ERROR:
            return "EMPTY_RESPONSE_ENVS_ERROR";
        case EMPTY_ENVS_ERROR:
            return "EMPTY_ENVS_ERROR";
        case COMMAND_ENOENT_ERROR:
            return "COMMAND_ENOENT_ERROR";
        case COMMAND_FAILED_TO_RUN:
            return "COMMAND_FAILED_TO_RUN";
        case NO_ACTION_ERROR:
            return "NO_ACTION_ERROR";
        case DEBUG_LEXER:
            return "DEBUG_LEXER";
        case LEXER_FILE_ENOENT_ERROR:
            return "LEXER_FILE_ENOENT_ERROR";
        case LEXER_FILE_ERROR:
            return "LEXER_FILE_ERROR";
        case FILE_ENOENT_ERROR:
            return "FILE_ENOENT_ERROR";
        case NVI_VERSION:
            return "NVI_VERSION";
        case DEBUG_ARG:
            return "DEBUG_ARG";
        case CONFLICTING_COMMAND_FLAG:
            return "CONFLICTING_COMMAND_FLAG";
        case CONFLICTING_CONFIG_FLAG:
            return "CONFLICTING_CONFIG_FLAG";
        default:
            return "UNKNOWN_MESSAGE_TYPE";
        }
    }

} // namespace nvi

#endif
