#ifndef NVI_CONFIG_TOKEN_H
#define NVI_CONFIG_TOKEN_H
#include "format.h"
#include <optional>
#include <string>
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
     * nvi::options_t options;
     * options.config = "development";
     * options.dir = "custom/path/to/envs";
     * nvi::Config config(options);
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

    inline std::string get_value_type_string(const ConfigValueType &cvt) noexcept {
        switch (cvt) {
        case ConfigValueType::array:
            return "an array";
        case ConfigValueType::boolean:
            return "a boolean";
        default:
            return "a string";
        }
    }

} // namespace nvi
#endif
