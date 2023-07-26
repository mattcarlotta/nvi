#include "parser.h"
#include "format.h"
#include "log.h"
#include "options.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>

namespace nvi {

    inline const constexpr char NULL_TERMINATOR = '\0'; // 0x00
    inline const constexpr char COMMENT = '#';          // 0x23
    inline const constexpr char LINE_DELIMITER = '\n';  // 0x0a
    inline const constexpr char BACK_SLASH = '\\';      // 0x5c
    inline const constexpr char ASSIGN_OP = '=';        // 0x3d
    inline const constexpr char DOLLAR_SIGN = '$';      // 0x24
    inline const constexpr char OPEN_BRACE = '{';       // 0x7b
    inline const constexpr char CLOSE_BRACE = '}';      // 0x7d
    inline const constexpr char DOUBLE_QUOTE = '"';     // 0x22

    Parser::Parser(const options_t &options) : _options(options) {}

    Parser *Parser::parse() noexcept {
        while (_byte_count < _file.length()) {
            const std::string_view line = _file_view.substr(_byte_count, _file.length());
            const int line_delimiter_index = line.find(LINE_DELIMITER);

            // skip lines that begin with comments
            if (line[0] == COMMENT || line[0] == LINE_DELIMITER) {
                ++_line_count;
                _byte_count += line_delimiter_index + 1;
                continue;
            }

            // split key from value by assignment "="
            _assignment_index = line.find(ASSIGN_OP);
            if (line_delimiter_index >= 0 && _assignment_index >= 0) {
                _key = line.substr(0, _assignment_index);

                const std::string_view line_slice = line.substr(_assignment_index + 1, line.length());
                _val_byte_count = 0;
                _value = "";
                while (_val_byte_count < line_slice.length()) {
                    const char current_char = line_slice[_val_byte_count];
                    const char next_char = line_slice[_val_byte_count + 1];

                    // stop parsing if there's a new line character
                    if (current_char == LINE_DELIMITER) {
                        break;
                    }

                    // skip parsing escaped multi-line characters
                    if (current_char == BACK_SLASH && next_char == LINE_DELIMITER) {
                        ++_line_count;
                        _val_byte_count += 2;
                        continue;
                    }

                    // try interpolating key into a value
                    if (current_char == DOLLAR_SIGN && next_char == OPEN_BRACE) {
                        const std::string_view val_slice_str = line_slice.substr(_val_byte_count, line_slice.length());

                        const int interp_close_index = val_slice_str.find(CLOSE_BRACE);
                        if (interp_close_index >= 0) {
                            _key_prop = val_slice_str.substr(2, interp_close_index - 2);
                            const char *val_from_proc = std::getenv(_key_prop.c_str());

                            // interpolate the value from env first, otherwise fallback to the env_map
                            std::string interpolated_value = "";
                            if (val_from_proc != nullptr && *val_from_proc != NULL_TERMINATOR) {
                                interpolated_value = val_from_proc;
                            } else if (_env_map.count(_key_prop)) {
                                interpolated_value = _env_map.at(_key_prop);
                            } else if (_options.debug) {
                                log(INTERPOLATION_WARNING);
                            }

                            _value += interpolated_value;

                            // factor in the interpolated key length with the "$", "{" and "}" character bytes
                            _val_byte_count += _key_prop.length() + 3;

                            // continue the loop because the next character in the value may be another interpolated
                            // value
                            continue;
                        } else {
                            log(INTERPOLATION_ERROR);
                        }
                    }

                    _value += line_slice[_val_byte_count];

                    ++_val_byte_count;
                }

                if (_key.length()) {
                    _env_map[_key] = _value;
                    if (_options.debug) {
                        log(DEBUG);
                    }
                }

                _byte_count += _assignment_index + _val_byte_count + 1;
            } else {
                _byte_count = _file.length();
            }
        }

        if (_options.debug) {
            log(DEBUG_FILE_PROCESSED);
        }

        _env_file.close();

        return this;
    }

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

    Parser *Parser::parse_envs() noexcept {
        for (const std::string &env : _options.files) {
            read(env)->parse();
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
                while (i < value.size()) {
                    char current_char = value[i];
                    if (current_char == DOUBLE_QUOTE) {
                        esc_value += BACK_SLASH;
                    }
                    esc_value += current_char;
                    ++i;
                }
                const std::string comma = key != last_key ? "," : "";
                std::cout << std::setw(4) << "\"" << key << "\""
                          << ": \"" << esc_value << "\"" << comma << '\n';
            }
            std::cout << "}" << std::endl;
        }
    }

    const env_map_t &Parser::get_env_map() const noexcept { return _env_map; }

    Parser *Parser::read(const std::string &env_file_name) noexcept {
        _file_name = env_file_name;
        _byte_count = 0;
        _val_byte_count = 0;
        _line_count = 0;
        _assignment_index = -1;
        _key.clear();
        _key_prop.clear();
        _value.clear();
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
        if (_env_file.bad() || not _env_file.is_open()) {
            log(FILE_ERROR);
        }
        _file = std::string{std::istreambuf_iterator<char>(_env_file), std::istreambuf_iterator<char>()};
        if (not _file.length()) {
            log(EMPTY_ENVS_ERROR);
        }
        _file_view = std::string_view{_file};

        return this;
    }

    void Parser::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case INTERPOLATION_WARNING: {
            NVI_LOG_DEBUG(
                INTERPOLATION_WARNING,
                "[%s:%d:%d] The key \"%s\" contains an invalid interpolated variable: \"%s\". Unable to locate "
                "a value that corresponds to this key.",
                _file_name.c_str(), _line_count + 1, _assignment_index + _val_byte_count + 2, 
                _key.c_str(), _key_prop.c_str());
            break;
        }
        case INTERPOLATION_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                INTERPOLATION_ERROR,
                "[%s:%d:%d] The key \"%s\" contains an interpolated \"{\" operator, but appears to be missing a "
                "closing \"}\" operator.",
                _file_name.c_str(), _line_count + 1, _assignment_index + _val_byte_count + 2,
                _key.c_str());
            break;
        }
        case DEBUG: {
            NVI_LOG_DEBUG(
                DEBUG,
                "[%s:%d:%d] Set key \"%s\" to equal value \"%s\".",
                _file_name.c_str(), _line_count + 1, _assignment_index + _val_byte_count + 2,
                _key.c_str(), _value.c_str());
            break;
        }
        case DEBUG_FILE_PROCESSED: {
            NVI_LOG_DEBUG(
                DEBUG_FILE_PROCESSED,
                "(%s) Processed %d line%s and %d bytes!\n",
                _file_name.c_str(), _line_count, 
                (_line_count != 1 ? "s" : ""), _byte_count);
            break;
        }
        case REQUIRED_ENV_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUIRED_ENV_ERROR,
                "The following ENV keys are marked as required: \"%s\", but they are undefined after the list of "
                ".env files were parsed.",
                fmt::join(_undefined_keys, ", ").c_str());
            break;
        }
        case FILE_ENOENT_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_ENOENT_ERROR,
                "Unable to locate \"%s\". The .env file doesn't appear to exist at this path!",
                _file_path.c_str());
            break;
        }
        case FILE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_ERROR,
                "Unable to open \"%s\". The .env file is either invalid, has restricted access, or may be corrupted.",
                _file_path.c_str());
            break;
        }
        case FILE_EXTENSION_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_EXTENSION_ERROR,
                "The \"%s\" file is not a valid \".env\" file extension.",
                _file_name.c_str());
            break;
        }
        case EMPTY_ENVS_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EMPTY_ENVS_ERROR,
                "Unable to parse any ENVs! Please ensure the \"%s\" file is not empty.",
                _file_name.c_str());
            break;
        }
        case COMMAND_ENOENT_ERROR: {
            NVI_LOG_DEBUG(
                COMMAND_ENOENT_ERROR,
                "The specified command encountered an error. The command \"%s\" doesn't appear to exist or may "
                "not reside in a directory within the shell PATH.",
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
}; // namespace nvi
