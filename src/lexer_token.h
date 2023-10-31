#ifndef NVI_ENV_LEXER_TOKEN_H
#define NVI_ENV_LEXER_TOKEN_H

#include <optional>
#include <string>
#include <vector>

namespace nvi {
    enum class ValueType { normal, interpolated, multiline, unknown };

    struct ValueToken {
        ValueType type;
        std::optional<std::string> value{};
        size_t byte = 0;
        size_t line = 0;
    };

    struct Token {
        std::optional<std::string> key = "";
        std::vector<ValueToken> values{};
        std::string file;
    };

    typedef std::vector<Token> tokens_t;

    inline std::string get_value_type_string(const ValueType &vt) noexcept {
        switch (vt) {
        case ValueType::normal:
            return "a normal value";
        case ValueType::interpolated:
            return "an interpolated key";
        default:
            return "a multiline value";
        }
    }
} // namespace nvi
#endif
