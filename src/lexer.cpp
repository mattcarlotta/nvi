#include "lexer.h"
#include "log.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace nvi {
    inline constexpr char NULL_CHAR = '\0';      // 0x00
    inline constexpr char HASH = '#';            // 0x23
    inline constexpr char LINE_DELIMITER = '\n'; // 0x0a
    inline constexpr char BACK_SLASH = '\\';     // 0x5c
    inline constexpr char ASSIGN_OP = '=';       // 0x3d
    inline constexpr char DOLLAR_SIGN = '$';     // 0x24
    inline constexpr char OPEN_BRACE = '{';      // 0x7b
    inline constexpr char CLOSE_BRACE = '}';     // 0x7d

    Lexer::Lexer(const options_t &options) : _options(options) {}

    std::vector<Token> Lexer::get_tokens() const noexcept { return _tokens; }

    std::optional<char> Lexer::peek(int offset) const noexcept {
        if (_byte + offset >= _file.length()) {
            return std::nullopt;
        } else {
            return _file.at(_byte + offset);
        }
    }

    char Lexer::commit() noexcept { return _file.at(_byte++); }

    void Lexer::skip(int offset) noexcept { _byte += offset; }

    void Lexer::lex_file() {
        Token token;
        token.file = _file_name;

        std::string value;
        while (peek().has_value()) {
            const char current_char = peek().has_value() ? peek().value() : NULL_CHAR;

            if (std::isalnum(current_char)) {
                while (peek().has_value() && std::isalnum(peek().value())) {
                    value.push_back(commit());
                    ++_index;
                };
                continue;
            } else if (std::isblank(current_char)) {
                value.push_back(commit());
                ++_index;

                continue;
            } else if (std::ispunct(current_char)) {
                if (current_char == ASSIGN_OP) {
                    token.key = value;
                    ++_index;

                    value.clear();
                    skip();
                    continue;
                } else if (current_char == HASH) {
                    if (token.key->length()) {
                        // commit lines with hashes
                        value.push_back(commit());
                        ++_index;

                        continue;
                    } else {
                        // skip lines with comments
                        while (peek().has_value() && peek().value() != LINE_DELIMITER) {
                            skip();
                        };

                        ++_line;

                        // skip '\n'
                        skip();
                    }
                    continue;
                } else if (current_char == DOLLAR_SIGN && (peek(1).has_value() && peek(1).value() == OPEN_BRACE)) {
                    if (value.length()) {
                        token.values.push_back({ValueType::normal, value, _index, _line});
                        value.clear();
                    }

                    // skip "${"
                    skip(2);
                    _index += 2;

                    while (peek().has_value() && peek().value() != CLOSE_BRACE) {
                        if (peek().value() == LINE_DELIMITER) {
                            _token_key = token.key.value();
                            log(INTERPOLATION_ERROR);
                        } else {
                            ++_index;
                            value.push_back(commit());
                        }
                    }

                    token.values.push_back({ValueType::interpolated, value, _index, _line});
                    value.clear();

                    // skip '}'
                    skip();
                    ++_index;

                    // if the next value is a new line, store the token and then reset it
                    if (peek().has_value() && peek().value() == LINE_DELIMITER) {
                        _tokens.push_back(token);
                        token.key->clear();
                        token.values.clear();
                        ++_line;
                        _index = 0;

                        // skip '\n'
                        skip();
                    }

                    continue;
                } else if (current_char == BACK_SLASH && (peek(1).has_value() && peek(1).value() == LINE_DELIMITER)) {
                    if (value.length()) {
                        token.values.push_back({ValueType::normal, value, _index, _line});
                    }

                    // skip "\\n"
                    skip(2);
                    _index = 0;

                    // handle multiline values
                    value.clear();
                    while (peek().has_value()) {
                        const bool is_eol = peek().value() == LINE_DELIMITER;

                        if ((peek().value() == BACK_SLASH &&
                             (peek(1).has_value() && peek(1).value() == LINE_DELIMITER)) ||
                            is_eol) {

                            token.values.push_back(
                                {is_eol ? ValueType::normal : ValueType::multiline, value, _index, _line});
                            value.clear();

                            ++_line;
                            _index = 0;

                            // skip '\n' or "\\n"
                            skip(is_eol ? 1 : 2);

                            if (is_eol) {
                                break;
                            } else {
                                continue;
                            }
                        }
                        ++_index;
                        value.push_back(commit());
                    }

                    _tokens.push_back(token);
                    token.key->clear();

                    continue;
                } else if (current_char != LINE_DELIMITER) {
                    value.push_back(commit());
                    ++_index;
                    continue;
                }
            } else {
                if (token.key->length()) {
                    token.values.push_back({ValueType::normal, value, _index, _line});
                    _tokens.push_back(token);
                }

                token.key->clear();
                token.values.clear();
                value.clear();
                _index = 0;

                ++_line;
                skip();
            }
        };
    }
    Lexer *Lexer::read_files() noexcept {
        for (const std::string &env : _options.files) {
            _byte = 0;
            _index = 0;
            _line = 1;
            _file_name = env;
            _file.clear();
            _file_path.clear();

            _file_path = std::filesystem::current_path() / _options.dir / _file_name;
            if (not std::filesystem::exists(_file_path)) {
                log(FILE_ENOENT_ERROR);
            }

            if (std::string{_file_path.extension()}.find(".env") == std::string::npos &&
                std::string{_file_path.stem()}.find(".env") == std::string::npos) {
                log(FILE_EXTENSION_ERROR);
            }

            _env_file = std::ifstream{_file_path, std::ios_base::in};
            if (not _env_file.is_open()) {
                log(FILE_ERROR);
            }
            _file = std::string{std::istreambuf_iterator<char>(_env_file), std::istreambuf_iterator<char>()};
            if (not _file.length()) {
                log(EMPTY_ENVS_ERROR);
            }

            lex_file();

            _env_file.close();
        }

        return this;
    }

    void Lexer::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case INTERPOLATION_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                INTERPOLATION_ERROR,
                R"([%s:%d:%d] The key "%s" contains an interpolated "{" operator, but appears to be missing a closing "}" operator.)",
                _file_name.c_str(), _line, _index, _token_key.c_str());
            break;
        }
           case FILE_ENOENT_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_ENOENT_ERROR,
                R"(Unable to locate "%s". The .env file doesn't appear to exist at this path!)",
                _file_path.c_str());
            break;
        }
        case FILE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_ERROR,
                R"(Unable to open "%s". The .env file is either invalid, has restricted access, or may be corrupted.)",
                _file_path.c_str());
            break;
        }
        case FILE_EXTENSION_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_EXTENSION_ERROR,
                R"(The "%s" file is not a valid ".env" file extension.)",
                _file_name.c_str());
            break;
        } 
        case EMPTY_ENVS_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EMPTY_ENVS_ERROR,
                R"(Unable to parse any ENVs! Please ensure the "%s" file is not empty.)",
                _file.c_str());
            break;
        }
        default:
            break;
        }
        // clang-format on
    }

}; // namespace nvi
