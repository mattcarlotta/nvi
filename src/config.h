#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "format.h"
#include "log.h"
#include "options.h"
#include <filesystem>
#include <optional>
#include <string>
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
     * const string::string env = "development";
     * const std::string dir = "custom/path/to/envs";
     * nvi::Config config(env, dir);
     * nvi::options_t options = config.generate_options->get_options();
     */
    enum class ConfigValueType { array, boolean, string };

    struct ConfigToken {
        ConfigValueType type;
        std::string key = "";
        std::optional<std::variant<std::string, std::vector<std::string>>> value{};
    };

    struct ConfigTokenToString {
        std::string operator()(const std::string &x) const { return x; }
        std::string operator()(const std::vector<std::string> x) const { return (x.size() ? fmt::join(x, ", ") : ""); }
    };

    class Config {
        public:
        Config(const std::string &environment, const std::string env_dir = "");
        Config *generate_options() noexcept;
        const options_t &get_options() const noexcept;

        private:
        const std::string extract_value_within(char delimiter) noexcept;
        std::optional<char> peek(int offset = 0) const noexcept;
        char commit() noexcept;
        void skip(int offset = 1) noexcept;
        void skip_to_eol() noexcept;
        std::string get_value_type_string(const ConfigValueType &cvt) const noexcept;
        void log(const messages_t &code) const noexcept;

        options_t _options;
        std::string _file;
        std::string _config;
        size_t _byte = 0;
        std::vector<ConfigToken> _config_tokens;
        const std::string _env;
        std::filesystem::path _file_path;
        std::string _command;
        std::string _key;
        std::string _value_type;
    };
}; // namespace nvi

#endif
