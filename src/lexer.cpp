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
#include <utility>
#include <vector>

namespace nvi {
    inline constexpr char HASH = '#';            // 0x23
    inline constexpr char LINE_DELIMITER = '\n'; // 0x0a
    inline constexpr char BACK_SLASH = '\\';     // 0x5c
    inline constexpr char ASSIGN_OP = '=';       // 0x3d
    inline constexpr char DOLLAR_SIGN = '$';     // 0x24
    inline constexpr char OPEN_BRACE = '{';      // 0x7b
    inline constexpr char CLOSE_BRACE = '}';     // 0x7d

    Lexer::Lexer(const options_t &options)
        : _options(options), logger(LOGGER::LEXER, options, _file_path, _tokens, _byte, _line, _file_name, _token_key) {
    }

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
                            logger.fatal(INTERPOLATION_ERROR);
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

    Lexer *Lexer::parse_api_response(std::string &&envs) noexcept {
        _index = 0;
        _byte = 1;
        _line = 1;
        _file_name = _options.environment;
        _file_path = _options.project;

        _file = envs;
        if (not _file.length()) {
            logger.fatal(EMPTY_RESPONSE_ENVS_ERROR);
        }

        parse_file();

        if (_options.debug) {
            logger.debug(DEBUG_LEXER);
        }

        return this;
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
                logger.fatal(LEXER_FILE_ENOENT_ERROR);
            }

            if (std::string{_file_path.extension()}.find(".env") == std::string::npos &&
                std::string{_file_path.stem()}.find(".env") == std::string::npos) {
                logger.fatal(FILE_EXTENSION_ERROR);
            }

            _env_file = std::ifstream{_file_path, std::ios_base::in};
            if (not _env_file.is_open()) {
                logger.fatal(LEXER_FILE_ERROR);
            }
            _file = std::string{std::istreambuf_iterator{_env_file}, {}};
            if (not _file.length()) {
                logger.fatal(EMPTY_ENVS_ERROR);
            }

            parse_file();

            _env_file.close();
        }

        if (_options.debug) {
            logger.debug(DEBUG_LEXER);
        }

        return this;
    }
}; // namespace nvi
