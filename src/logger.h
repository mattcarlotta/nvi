#ifndef NVI_LOGGER_H
#define NVI_LOGGER_H

#include "config_token.h"
#include "log.h"
#include "options.h"
// #include <cstddef>
#include <curl/curl.h>
#include <filesystem>
// #include <iostream>
#include <string>
#include <vector>

namespace nvi {
    class Logger {

        public:
        // generator
        Logger(logger_t code, const options_t &options);
        // arg
        Logger(logger_t code, const options_t &options, const std::string &command, const std::string &invalid_args,
               const std::string &invalid_flag);
        // config
        Logger(logger_t code, const options_t &options, const std::string &command,
               const std::vector<ConfigToken> &config_tokens, const std::filesystem::path &file_path,
               const std::string &key, const std::string &value_type);

        // api
        Logger(logger_t code, const options_t &options, const CURLcode &res, const std::string &res_data,
               const std::filesystem::path &env_file_path, const unsigned int &res_status_code);

        void fatal(const messages_t &message_code) const noexcept;
        void debug(const messages_t &message_code) const noexcept;

        private:
        void log_debug(const messages_t &message_code, const std::string &message) const noexcept;
        void log_error(const messages_t &message_code, const std::string &message) const noexcept;

        const std::string empty_string = "";
        const unsigned int empty_number = 0;
        // const size_t empty_number = 0;
        const std::vector<ConfigToken> empty_config_token = {};
        const std::filesystem::path empty_path = "";
        const CURLcode empty_curl = CURLE_UNSUPPORTED_PROTOCOL;

        const logger_t _code = LOGGER::UNKNOWN_LOG;
        // arg/generator
        const options_t &_options;
        const std::string &_command = empty_string;

        // arg
        const std::string &_invalid_args = empty_string;
        const std::string &_invalid_flag = empty_string;

        // config
        const std::vector<ConfigToken> &_config_tokens = empty_config_token;
        const std::filesystem::path &_file_path = empty_path;
        const std::string &_key = empty_string;
        const std::string &_value_type = empty_string;

        // api
        const CURLcode &_res = empty_curl;
        const std::string &_res_data = empty_string;
        const std::filesystem::path &_env_file_path = empty_path;
        const unsigned int &_res_status_code = empty_number;
    };
}; // namespace nvi
#endif
