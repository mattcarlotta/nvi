#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "format.h"
#include "log.h"
#include "options.h"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace nvi {
    enum class CONFIG_KEY {
        API,
        DEBUG,
        DIRECTORY,
        ENVIRONMENT,
        EXECUTE,
        FILES,
        PRINT,
        PROJECT,
        REQUIRED,
        SAVE,
        UNKNOWN
    };
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
        std::optional<std::variant<bool, std::string, std::vector<std::string>>> value{};
    };

    struct ConfigTokenToString {
        std::string operator()(const bool &b) const { return b > 0 ? "true" : "false"; }
        std::string operator()(const std::string &s) const { return s; }
        std::string operator()(const std::vector<std::string> &v) const { return (v.size() ? fmt::join(v, ", ") : ""); }
    };

    class Config {
        public:
        Config(const std::string &environment, const std::string env_dir = "");
        Config *generate_options() noexcept;
        const options_t &get_options() const noexcept;
        const std::vector<ConfigToken> &get_tokens() const noexcept;

        private:
        const std::string extract_value_within(char delimiter, bool error_at_new_line = false) noexcept;
        std::optional<char> peek(int offset = 0) const noexcept;
        char commit() noexcept;
        void skip(int offset = 1) noexcept;
        void skip_to_eol() noexcept;
        std::string get_value_type_string(const ConfigValueType &cvt) const noexcept;
        void log(const messages_t &code) const noexcept;

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
