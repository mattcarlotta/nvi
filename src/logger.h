#ifndef NVI_LOGGER_H
#define NVI_LOGGER_H

#include "config_token.h"
#include "lexer_token.h"
#include "log.h"
#include "options.h"
#include <cstddef>
#include <curl/curl.h>
#include <filesystem>
#include <optional>
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

        // parser
        Logger(logger_t code, const options_t &options, const std::string &key, const Token &token,
               const ValueToken &value_token, const std::string &interp_key, const std::string &value);

        // lexer
        Logger(logger_t code, const options_t &options, const std::filesystem::path &file_path,
               const std::vector<Token> &tokens, const size_t &byte, const size_t &line, const std::string &file_name,
               const std::string &token_key);

        void fatal(const messages_t &message_code) const noexcept;
        void debug(const messages_t &message_code) const noexcept;
        void log_and_exit(const messages_t &message_code) const noexcept;
        void log_debug(const messages_t &message_code, const std::string &message) const noexcept;

        private:
        void log_error(const messages_t &message_code, const std::string &message) const noexcept;

        const std::string empty_string = "";
        const unsigned int empty_number = 0;
        const size_t empty_size_number = 0;
        const std::vector<ConfigToken> empty_config_token = {};
        const std::filesystem::path empty_path = "";
        const CURLcode empty_curl = CURLE_UNSUPPORTED_PROTOCOL;
        const Token empty_token = {std::optional<std::string>{}, std::vector<ValueToken>{}, ""};
        const std::vector<Token> empty_tokens = std::vector<Token>{empty_token};
        const ValueToken empty_value_token = {ValueType::unknown, std::optional<std::string>{}, 0, 0};

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

        // parser
        const Token &_token = empty_token;
        const ValueToken &_value_token = empty_value_token;
        const std::string &_interp_key = empty_string;
        const std::string &_value = empty_string;

        // lexer
        const std::vector<Token> &_tokens = empty_tokens;
        const size_t &_byte = empty_size_number;
        const size_t &_line = empty_size_number;
        const std::string &_file_name = empty_string;
        const std::string &_token_key = empty_string;
    };
}; // namespace nvi
#endif
