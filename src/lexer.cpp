#include "lexer.h"
#include "log.h"
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace nvi {
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
        if (_index + offset >= _file.length()) {
            return std::nullopt;
        } else {
            return _file.at(_index + offset);
        }
    }

    char Lexer::commit() noexcept {
        increase_byte_count();
        return _file.at(_index++);
    }

    void Lexer::skip(int offset) noexcept { _index += offset; }

    void Lexer::reset_byte_count() noexcept { _byte = 1; }

    void Lexer::increase_byte_count(size_t offset) noexcept { _byte += offset; }

    void Lexer::increase_line_count() noexcept { ++_line; }

    void Lexer::parse_file() noexcept {
        Token token;
        token.file = _file_name;

        std::string value;
        while (peek().has_value()) {
            const char current_char = peek().value();

            if (std::isalnum(current_char)) {
                while (peek().has_value() && std::isalnum(peek().value())) {
                    value.push_back(commit());
                };
                continue;
            } else if (std::isblank(current_char) || std::ispunct(current_char)) {
                if (current_char == ASSIGN_OP) {
                    token.key = value;
                    increase_byte_count();
                    value.clear();
                    // skip '='
                    skip();
                    continue;
                } else if (current_char == HASH) {
                    if (token.key->length()) {
                        // commit lines with hashes
                        value.push_back(commit());
                        continue;
                    } else {
                        // skip lines with comments
                        while (peek().has_value() && peek().value() != LINE_DELIMITER) {
                            skip();
                        };

                        increase_line_count();
                        // skip '\n'
                        skip();
                    }
                    continue;
                } else if (current_char == DOLLAR_SIGN && (peek(1).has_value() && peek(1).value() == OPEN_BRACE)) {
                    if (value.length()) {
                        token.values.push_back({ValueType::normal, value, _byte, _line});
                        value.clear();
                    }

                    // skip "${"
                    skip(2);
                    increase_byte_count(2);

                    while (peek().has_value() && peek().value() != CLOSE_BRACE) {
                        if (peek().value() == LINE_DELIMITER) {
                            _token_key = token.key.value();
                            log(INTERPOLATION_ERROR);
                        } else {
                            value.push_back(commit());
                        }
                    }

                    token.values.push_back({ValueType::interpolated, value, _byte, _line});
                    value.clear();

                    // skip '}'
                    skip();
                    increase_byte_count();

                    // if the next value is a new line, store the token and then reset it
                    if (peek().has_value() && peek().value() == LINE_DELIMITER) {
                        _tokens.push_back(token);
                        token.key->clear();
                        token.values.clear();
                        reset_byte_count();
                        value.clear();
                        increase_line_count();

                        // skip '\n'
                        skip();
                    }
                    continue;
                } else if (current_char == BACK_SLASH && (peek(1).has_value() && peek(1).value() == LINE_DELIMITER)) {
                    if (value.length()) {
                        token.values.push_back({ValueType::normal, value, _byte, _line});
                    }

                    // skip "\\n"
                    skip(2);
                    reset_byte_count();

                    // handle multiline values
                    value.clear();
                    while (peek().has_value()) {
                        const bool is_eol = peek().value() == LINE_DELIMITER;

                        if ((peek().value() == BACK_SLASH &&
                             (peek(1).has_value() && peek(1).value() == LINE_DELIMITER)) ||
                            is_eol) {

                            increase_line_count();
                            token.values.push_back(
                                {is_eol ? ValueType::normal : ValueType::multiline, value, _byte, _line});

                            value.clear();
                            reset_byte_count();

                            // skip '\n' or "\\n"
                            skip(is_eol ? 1 : 2);

                            if (is_eol) {
                                break;
                            } else {
                                continue;
                            }
                        }

                        value.push_back(commit());
                    }

                    _tokens.push_back(token);
                    token.key->clear();
                    token.values.clear();
                    value.clear();
                    reset_byte_count();
                    increase_line_count();
                    continue;
                } else if (current_char != LINE_DELIMITER) {
                    value.push_back(commit());
                    continue;
                } else {
                    continue;
                }
            } else {
                if (token.key->length()) {
                    token.values.push_back({ValueType::normal, value, _byte, _line});
                    _tokens.push_back(token);
                }

                token.key->clear();
                token.values.clear();
                value.clear();
                reset_byte_count();
                increase_line_count();
                skip();
                continue;
            }
        };
    }
    Lexer *Lexer::parse_files() noexcept {
        for (const std::string &env : _options.files) {
            _index = 0;
            _byte = 1;
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

            parse_file();

            _env_file.close();
        }

        if (_options.debug) {
            log(DEBUG);
        }

        return this;
    }

    std::string Lexer::get_value_type_string(const ValueType &vt) const noexcept {
        switch (vt) {
        case ValueType::normal:
            return "a normal value";
        case ValueType::interpolated:
            return "an interpolated key";
        default:
            return "a multiline value";
        }
    }

    void Lexer::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case INTERPOLATION_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                INTERPOLATION_ERROR,
                R"([%s:%d:%d] The key "%s" contains an interpolated "{" operator, but appears to be missing a closing "}" operator.)",
                _file_name.c_str(), _line, _byte, _token_key.c_str());
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
        case DEBUG: {
            for (const Token &token : _tokens) {
                std::stringstream ss;
                ss << "Created a token key " << std::quoted(token.key.value());
                ss << " with the following tokenized values(" << token.values.size() << "): \n";
                for (size_t index = 0; index < token.values.size(); ++index) {
                    const ValueToken &vt = token.values.at(index);
                    ss << std::setw(4) << index + 1 << ": [" << token.file << "::" << vt.line << "::" << vt.byte << "] ";
                    ss << "A token value of " << std::quoted((vt.value.has_value() ? vt.value.value() : ""));
                    ss << " has been created as " << get_value_type_string(vt.type) << ".";
                    if(index + 1 != token.values.size()) {
                        ss << '\n';
                    }
                }
                NVI_LOG_DEBUG(DEBUG, ss.str().c_str(), NULL);
            }
            break;
        }
        default:
            break;
        }
        // clang-format on
    }
}; // namespace nvi
