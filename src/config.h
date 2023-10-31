#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "config_token.h"
#include "format.h"
#include "log.h"
#include "logger.h"
#include "options.h"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace nvi {
    /**
     * @detail Reads an `.nvi` configuration file from the project root directory or a specified directory and
     * converts the selected environment into `options`.
     * @param `env` contains the name of the environment to load from the configuration file.
     * @param `dir` is an optional argument to specify where the .nvi file resides according to current directory
     * @example Initializing a config
     *
     * nvi::options_t options;
     * options.config = "development";
     * options.dir = "custom/path/to/envs";
     * nvi::Config config(options);
     */
    class Config {
        public:
        Config(options_t &options);
        Config *generate_options() noexcept;
        const config_tokens_t &get_tokens() const noexcept;

        private:
        const std::string extract_value_within(char delimiter, bool error_at_new_line = false) noexcept;
        std::optional<char> peek(int offset = 0) const noexcept;
        char commit() noexcept;
        void skip(int offset = 1) noexcept;
        void skip_to_eol() noexcept;

        options_t &_options;
        std::unordered_map<std::string_view, CONFIG_KEY> CONFIG_KEYS{
            {"api", CONFIG_KEY::API},
            {"debug", CONFIG_KEY::DEBUG},
            {"directory", CONFIG_KEY::DIRECTORY},
            {"environment", CONFIG_KEY::ENVIRONMENT},
            {"execute", CONFIG_KEY::EXECUTE},
            {"files", CONFIG_KEY::FILES},
            {"print", CONFIG_KEY::PRINT},
            {"project", CONFIG_KEY::PROJECT},
            {"required", CONFIG_KEY::REQUIRED},
            {"save", CONFIG_KEY::SAVE},
        };
        std::string _file;
        std::string _config;
        size_t _byte = 0;
        config_tokens_t _config_tokens;
        std::filesystem::path _file_path;
        std::string _command;
        std::string _key;
        std::string _value_type;
        Logger logger;
    };
}; // namespace nvi

#endif
