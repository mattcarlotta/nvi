#ifndef NVI_LOGGER_H
#define NVI_LOGGER_H

#include "config_token.h"
#include "log.h"
#include "options.h"
// #include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace nvi {
    class Logger {

        public:
        // generator
        Logger(const options_t &options);
        // arg
        Logger(const options_t &options, const std::string &command, const std::string &invalid_args,
               const std::string &invalid_flag);
        // config
        Logger(const options_t &options, const std::string &command, const std::vector<ConfigToken> &config_tokens,
               const std::filesystem::path &file_path, const std::string &key, const std::string &value_type);

        void Arg(const messages_t &code) const noexcept;
        void Config(const messages_t &code) const noexcept;
        void Generator(const messages_t &code) const noexcept;

        private:
        const std::string empty_string = "";
        // const size_t empty_number = 0;
        const std::vector<ConfigToken> empty_config_token = {};
        const std::filesystem::path empty_path = "";

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
    };
}; // namespace nvi
#endif
