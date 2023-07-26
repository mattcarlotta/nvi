
#ifndef NVI_LOG_H
#define NVI_LOG_H

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#define __CLASS__ get_class_name(__PRETTY_FUNCTION__)

/**
 * @detail Logs an message to stdlog.
 */
#define NVI_LOG_DEBUG(...)                                                                                             \
    do {                                                                                                               \
        ::nvi::debug_log(std::clog, __CLASS__, __VA_ARGS__);                                                           \
    } while (false)

/**
 * @detail Logs an error to stderr and then exits the process.
 */
#define NVI_LOG_ERROR_AND_EXIT(...)                                                                                    \
    do {                                                                                                               \
        ::nvi::error_log(std::cerr, __CLASS__, __VA_ARGS__);                                                           \
    } while (false)

namespace nvi {
    enum MESSAGES {
        FILE_ERROR,
        FILE_PARSE_ERROR,
        DEBUG_ARG_ERROR,
        DIR_ARG_ERROR,
        FILES_ARG_ERROR,
        MISSING_FILES_ARG_ERROR,
        EXEC_ARG_ERROR,
        REQUIRED_ARG_ERROR,
        INVALID_PROPERTY_WARNING,
        DEBUG
    };

    inline const std::string get_class_name(const std::string &class_name) {
        size_t colons = class_name.find("::");
        size_t begin = class_name.substr(0, colons).rfind(" ") + 1;
        size_t end = class_name.rfind("(") - begin;
        return class_name.substr(begin, end);
    }

    inline const std::string get_message_from_code(const unsigned int &error) {
        switch (error) {
        case FILE_ERROR:
            return "FILE_ERROR";
        case FILE_PARSE_ERROR:
            return "FILE_PARSE_ERROR";
        case DEBUG_ARG_ERROR:
            return "DEBUG_ARG_ERROR";
        case DIR_ARG_ERROR:
            return "DIR_ARG_ERROR";
        case FILES_ARG_ERROR:
            return "FILES_ARG_ERROR";
        case MISSING_FILES_ARG_ERROR:
            return "MISSING_FILES_ARG_ERROR";
        case EXEC_ARG_ERROR:
            return "EXEC_ARG_ERROR";
        case REQUIRED_ARG_ERROR:
            return "REQUIRED_ARG_ERROR";
        case INVALID_PROPERTY_WARNING:
            return "INVALID_PROPERTY_WARNING";
        case DEBUG:
            return "DEBUG";
        default:
            return "UNKNOWN_MESSAGE_TYPE";
        }
    }

    template <typename... A>
    void debug_log(std::ostream &log, const std::string &filename, const unsigned int &code, const char *fmt,
                   A... args) {
        const std::string message = get_message_from_code(code);
        size_t size = snprintf(nullptr, 0, fmt, args...);
        std::string buf;
        buf.reserve(size + 1);
        buf.resize(size);
        snprintf(&buf[0], size + 1, fmt, args...);
        log << "[nvi] (" << filename << "::" << message << ") " << buf << std::endl;
    }

    template <typename... A>
    void error_log(std::ostream &log, const std::string &filename, const unsigned int &error, const char *fmt,
                   A... args) {
        debug_log(log, filename, error, fmt, args...);
        std::exit(1);
    }
} // namespace nvi

#endif
