#include "parser.h"
#include "format.h"
#include "lexer.h"
#include "log.h"
#include "options.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

namespace nvi {
    inline constexpr char NULL_CHAR = '\0';   // 0x00
    inline constexpr char BACK_SLASH = '\\';  // 0x5c
    inline constexpr char DOUBLE_QUOTE = '"'; // 0x22

    Parser::Parser(const tokens_t &tokens, const options_t &options) : _tokens(tokens), _options(options) {}

    const env_map_t &Parser::get_env_map() const noexcept { return _env_map; }

    Parser *Parser::check_envs() noexcept {
        if (_options.required_envs.size()) {
            for (const std::string &key : _options.required_envs) {
                if (not _env_map.count(key) || not _env_map.at(key).length()) {
                    _undefined_keys.push_back(key);
                }
            }

            if (_undefined_keys.size()) {
                log(REQUIRED_ENV_ERROR);
            }
        }

        if (not _env_map.size()) {
            log(EMPTY_ENVS_ERROR);
        }

        return this;
    }

    void Parser::set_or_print_envs() noexcept {
        if (_options.commands.size()) {
            pid_t pid = fork();

            if (pid == 0) {
                for (const auto &[key, value] : _env_map) {
                    setenv(key.c_str(), value.c_str(), 0);
                }

                // NOTE: Unfortunately, have to set ENVs to the parent process because
                // "execvpe" doesn't exist on POSIX, but also it doesn't allow binaries
                // that were called to locate other binaries found within shell "PATH"
                // for example: "npm run dev" won't work because "npm" can't find "node"
                execvp(_options.commands[0], _options.commands.data());
                if (errno == ENOENT) {
                    log(COMMAND_ENOENT_ERROR);
                    _exit(-1);
                }
            } else if (pid > 0) {
                int status;
                wait(&status);
            } else {
                log(COMMAND_FAILED_TO_RUN);
            }
        } else {
            // if a command wasn't provided, print ENVs as a JSON formatted string to stdout
            const std::string last_key = std::prev(_env_map.end())->first;
            std::cout << "{" << '\n';
            for (auto const &[key, value] : _env_map) {
                std::string esc_value;
                size_t i = 0;
                while (i < value.length()) {
                    const char current_char = value[i];
                    if (current_char == DOUBLE_QUOTE) {
                        esc_value += BACK_SLASH;
                    }
                    esc_value += current_char;
                    ++i;
                }
                std::cout << std::setw(4) << DOUBLE_QUOTE << key << DOUBLE_QUOTE << ": " << DOUBLE_QUOTE << esc_value
                          << DOUBLE_QUOTE << (key != last_key ? "," : "") << '\n';
            }
            std::cout << "}" << std::endl;
        }
    }

    Parser *Parser::parse_tokens() noexcept {
        if (!_tokens.size()) {
            return this;
        }

        for (const Token &t : _tokens) {
            _token = t;
            _key.clear();
            _key_prop.clear();
            _value.clear();

            if (not _token.values.size()) {
                continue;
            } else {
                for (const auto &vt : _token.values) {
                    _value_token = vt;
                    _key = _token.key.value();
                    if (_value_token.type == ValueType::interpolated) {
                        _key_prop = _value_token.value.has_value() ? _value_token.value.value() : "";
                        const char *val_from_env = std::getenv(_key_prop.c_str());

                        if (val_from_env != nullptr && *val_from_env != NULL_CHAR) {
                            _value += val_from_env;
                        } else if (_env_map.count(_key_prop)) {
                            _value += _env_map.at(_key_prop);
                        } else if (_options.debug) {
                            log(INTERPOLATION_WARNING);
                        }
                    } else {
                        _value += _value_token.value.value();
                    }
                }

                if (_token.key.has_value()) {
                    _env_map[_token.key.value()] = _value;
                    if (_options.debug) {
                        log(DEBUG);
                    }
                }
            }
        }

        if (_options.debug) {
            log(DEBUG_FILE_PROCESSED);
        }

        return this;
    }

    void Parser::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case INTERPOLATION_WARNING: {
            NVI_LOG_DEBUG(
                INTERPOLATION_WARNING,
                R"([%s:%d:%d] The key "%s" contains an invalid interpolated variable: "%s". Unable to locate a value that corresponds to this key.)",
                _token.file.c_str(), _value_token.line, _value_token.byte, _token.key->c_str(), _key_prop.c_str());
            break;
        }
        case DEBUG: {
            NVI_LOG_DEBUG(
                DEBUG,
                R"([%s:%d:%d] Set key "%s" to equal value "%s".)",
                _token.file.c_str(), _value_token.line, _value_token.byte, _key.c_str(), _value.c_str());
            break;
        }
        case DEBUG_FILE_PROCESSED: {
            NVI_LOG_DEBUG(
                DEBUG,
                "Successfully parsed the %s file!\n",
                _token.file.c_str())
            break;
        }
        case REQUIRED_ENV_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUIRED_ENV_ERROR,
                R"(The following ENV keys are marked as required: "%s", but they are undefined after the list of .env files were parsed.)",
                fmt::join(_undefined_keys, ", ").c_str());
            break;
        }
        case EMPTY_ENVS_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EMPTY_ENVS_ERROR,
                R"(Unable to parse any ENVs! Please ensure the "%s" file is not empty.)",
                _token.file.c_str());
            break;
        }
        case COMMAND_ENOENT_ERROR: {
            NVI_LOG_DEBUG(
                COMMAND_ENOENT_ERROR,
                R"(The specified command encountered an error. The command "%s" doesn't appear to exist or may not reside in a directory within the shell PATH.)",
                _options.commands[0]);
            break;
        }
        case COMMAND_FAILED_TO_RUN: {
            NVI_LOG_ERROR_AND_EXIT(
                COMMAND_FAILED_TO_RUN,
                "Unable to run the specified command. See terminal logs for more information.",
                NULL);
            break;
        }
        default:
            break;
        }
        // clang-format on
    }
} // namespace nvi
