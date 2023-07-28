
#ifndef NVI_LOG_H
#define NVI_LOG_H

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#define __CLASS__ get_class_name(__PRETTY_FUNCTION__)

/**
 * @detail Logs a message to stdlog (stderr).
 */
#define NVI_LOG_DEBUG(...)                                                                                             \
    do {                                                                                                               \
        ::nvi::_log(std::clog, __CLASS__, __VA_ARGS__);                                                                \
    } while (false)

/**
 * @detail Logs a message to stdlog (stderr) and exits.
 */
#define NVI_LOG_AND_EXIT(...)                                                                                          \
    do {                                                                                                               \
        ::nvi::_log_and_exit(std::clog, /* exit code */ 0, __CLASS__, __VA_ARGS__);                                    \
    } while (false)

/**
 * @detail Logs an error to stderr and then exits the process.
 */
#define NVI_LOG_ERROR_AND_EXIT(...)                                                                                    \
    do {                                                                                                               \
        ::nvi::_log_and_exit(std::cerr, /* exit code */ 1, __CLASS__, __VA_ARGS__);                                    \
    } while (false)

namespace nvi {
    typedef enum MESSAGES {
        // arg
        CONFIG_FLAG_ERROR,
        DIR_FLAG_ERROR,
        COMMAND_FLAG_ERROR,
        FILES_FLAG_ERROR,
        REQUIRED_FLAG_ERROR,
        HELP_DOC,
        INVALID_FLAG_WARNING,
        NVI_VERSION,
        // config
        FILE_PARSE_ERROR,
        DEBUG_ARG_ERROR,
        DIR_ARG_ERROR,
        FILES_ARG_ERROR,
        EMPTY_FILES_ARG_ERROR,
        EXEC_ARG_ERROR,
        REQUIRED_ARG_ERROR,
        INVALID_PROPERTY_WARNING,
        // parser
        INTERPOLATION_WARNING,
        INTERPOLATION_ERROR,
        DEBUG_FILE_PROCESSED,
        REQUIRED_ENV_ERROR,
        FILE_EXTENSION_ERROR,
        EMPTY_ENVS_ERROR,
        COMMAND_ENOENT_ERROR,
        COMMAND_FAILED_TO_RUN,
        // shared
        FILE_ERROR,
        FILE_ENOENT_ERROR,
        DEBUG
    } messages_t;

    inline const std::string get_class_name(const std::string &class_name) {
        size_t colons = class_name.find("::");
        size_t begin = class_name.substr(0, colons).rfind(" ") + 1;
        size_t end = class_name.rfind("(") - begin;
        // remove namespace "nvi::"
        return class_name.substr(begin + 5, end - 5);
    }

    inline const std::string get_message_from_code(const unsigned int &error) {
        switch (error) {
        case CONFIG_FLAG_ERROR:
            return "CONFIG_FLAG_ERROR";
        case DIR_FLAG_ERROR:
            return "DIR_FLAG_ERROR";
        case COMMAND_FLAG_ERROR:
            return "COMMAND_FLAG_ERROR";
        case FILES_FLAG_ERROR:
            return "FILES_FLAG_ERROR";
        case REQUIRED_FLAG_ERROR:
            return "REQUIRED_FLAG_ERROR";
        case HELP_DOC:
            return "HELP_DOC";
        case INVALID_FLAG_WARNING:
            return "INVALID_FLAG_WARNING";
        case FILE_PARSE_ERROR:
            return "FILE_PARSE_ERROR";
        case DEBUG_ARG_ERROR:
            return "DEBUG_ARG_ERROR";
        case DIR_ARG_ERROR:
            return "DIR_ARG_ERROR";
        case FILES_ARG_ERROR:
            return "FILES_ARG_ERROR";
        case EMPTY_FILES_ARG_ERROR:
            return "EMPTY_FILES_ARG_ERROR";
        case EXEC_ARG_ERROR:
            return "EXEC_ARG_ERROR";
        case REQUIRED_ARG_ERROR:
            return "REQUIRED_ARG_ERROR";
        case INVALID_PROPERTY_WARNING:
            return "INVALID_PROPERTY_WARNING";
        case INTERPOLATION_WARNING:
            return "INTERPOLATION_WARNING";
        case INTERPOLATION_ERROR:
            return "INTERPOLATION_ERROR";
        case DEBUG_FILE_PROCESSED:
            return "DEBUG_FILE_PROCESSED";
        case REQUIRED_ENV_ERROR:
            return "REQUIRED_ENV_ERROR";
        case FILE_EXTENSION_ERROR:
            return "FILE_EXTENSION_ERROR";
        case EMPTY_ENVS_ERROR:
            return "EMPTY_ENVS_ERROR";
        case COMMAND_ENOENT_ERROR:
            return "COMMAND_ENOENT_ERROR";
        case COMMAND_FAILED_TO_RUN:
            return "COMMAND_FAILED_TO_RUN";
        case FILE_ERROR:
            return "FILE_ERROR";
        case FILE_ENOENT_ERROR:
            return "FILE_ENOENT_ERROR";
        case DEBUG:
            return "DEBUG";
        case NVI_VERSION:
            return "NVI_VERSION";
        default:
            return "UNKNOWN_MESSAGE_TYPE";
        }
    }

    template <typename... A>
    void _log(std::ostream &log, const std::string &filename, const messages_t &code, const char *fmt, A... args) {
        const std::string message = get_message_from_code(code);
        size_t size = snprintf(nullptr, 0, fmt, args...);
        std::string buf;
        buf.reserve(size + 1);
        buf.resize(size);
        snprintf(&buf[0], size + 1, fmt, args...);
        log << "[nvi] (" << filename << "::" << message << ") " << buf << std::endl;
    }

    template <typename... A>
    void _log_and_exit(std::ostream &log, const int &exit_code, const std::string &filename, const messages_t &error,
                       const char *fmt, A... args) {
        _log(log, filename, error, fmt, args...);
        std::exit(exit_code);
    }
} // namespace nvi

#endif
