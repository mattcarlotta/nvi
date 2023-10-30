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
