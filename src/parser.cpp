#include "parser.h"
#include "format.h"
#include "json.cpp"
#include "options.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace nvi {

    enum LOG {
        INTERPOLATION_WARNING = 0,
        INTERPOLATION_ERROR = 1,
        DEBUG = 2,
        DEBUG_FILE_PROCESSED = 3,
        REQUIRED_ENV_ERROR = 4,
        FILE_ERROR = 5,
        EMPTY_ENVS = 6,
    };

    constexpr char NULL_TERMINATOR = '\0'; // 0x00
    constexpr char COMMENT = '#';          // 0x23
    constexpr char LINE_DELIMITER = '\n';  // 0x0a
    constexpr char BACK_SLASH = '\\';      // 0x5c
    constexpr char ASSIGN_OP = '=';        // 0x3d
    constexpr char DOLLAR_SIGN = '$';      // 0x24
    constexpr char OPEN_BRACE = '{';       // 0x7b
    constexpr char CLOSE_BRACE = '}';      // 0x7d
    constexpr char DOUBLE_QUOTE = '"';     // 0x22

    Parser::Parser(const Options &options) : _options(options) {}

    const std::map<std::string, std::string> &Parser::get_env_map() const { return _env_map; }

    Parser *Parser::parse() {
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
                                log(LOG::INTERPOLATION_WARNING);
                            }

                            _value += interpolated_value;

                            // factor in the interpolated key length with the "$", "{" and "}" character bytes
                            _val_byte_count += _key_prop.length() + 3;

                            // continue the loop because the next character in the value may be another interpolated
                            // value
                            continue;
                        } else {
                            log(LOG::INTERPOLATION_ERROR);
                            std::exit(1);
                        }
                    }

                    _value += line_slice[_val_byte_count];

                    ++_val_byte_count;
                }

                if (_key.length()) {
                    _env_map[_key] = _value;
                    if (_options.debug) {
                        log(LOG::DEBUG);
                    }
                }

                _byte_count += _assignment_index + _val_byte_count + 1;
            } else {
                _byte_count = _file.length();
            }
        }

        if (_options.debug) {
            log(LOG::DEBUG_FILE_PROCESSED);
        }

        _env_file.close();

        return this;
    }

    Parser *Parser::check_envs() {
        if (_options.required_envs.size()) {
            for (const std::string &key : _options.required_envs) {
                if (not _env_map.count(key) || not _env_map.at(key).length()) {
                    _undefined_keys.push_back(key);
                }
            }

            if (_undefined_keys.size()) {
                log(LOG::REQUIRED_ENV_ERROR);
                std::exit(1);
            }
        }

        if (not _env_map.size()) {
            log(LOG::EMPTY_ENVS);
            std::exit(1);
        }

        return this;
    }

    void Parser::set_envs() {
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
                    std::cerr
                        << nvi::fmt::format(
                               "[nvi] (main::COMMAND_ENOENT_ERROR) The specified command encountered an error. The "
                               "command \"%s\" doesn't appear to exist or may not reside in a directory within the "
                               "shell PATH.",
                               _options.commands[0])
                        << std::endl;
                    _exit(-1);
                }
            } else if (pid > 0) {
                int status;
                wait(&status);
            } else {
                std::cerr << "[nvi] (main::COMMAND_FAILED_TO_START) Unable to run the specified command." << std::endl;
                std::exit(1);
            }
        } else {
            // if a command wasn't provided, print ENVs as a JSON formatted string to stdout
            const std::string last_key = std::prev(_env_map.end())->first;
            std::cout << "{" << std::endl;
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
                          << ": \"" << esc_value << "\"" << comma << std::endl;
            }
            std::cout << "}" << std::endl;
        }
    }

    Parser *Parser::read(const std::string &env_file_name) {
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

        _file_path = std::string(std::filesystem::current_path() / _options.dir / _file_name);
        if (not std::filesystem::exists(_file_path)) {
            log(LOG::FILE_ERROR);
            std::exit(1);
        }

        _env_file = std::ifstream(_file_path, std::ios_base::in);
        if (_env_file.bad()) {
            log(LOG::FILE_ERROR);
            std::exit(1);
        }
        _file = std::string{std::istreambuf_iterator<char>(_env_file), std::istreambuf_iterator<char>()};
        if (not _file.length()) {
            log(LOG::EMPTY_ENVS);
            std::exit(1);
        }
        _file_view = std::string_view(_file);

        return this;
    }

    Parser *Parser::parse_envs() noexcept {
        for (const std::string &env : _options.files) {
            read(env)->parse();
        }

        return this;
    }

    void Parser::log(uint8_t code) const noexcept {
        switch (code) {
        case LOG::INTERPOLATION_WARNING: {
            std::clog << fmt::format(
                             "[nvi] (Parser::INTERPOLATION_WARNING::%s:%d:%d) The key \"%s\" contains an invalid "
                             "interpolated variable: \"%s\". Unable to locate a value that corresponds to this key.",
                             _file_name.c_str(), _line_count + 1, _assignment_index + _val_byte_count + 2, _key.c_str(),
                             _key_prop.c_str())
                      << std::endl;
            break;
        }
        case LOG::INTERPOLATION_ERROR: {
            std::cerr << fmt::format("[nvi] (Parser::INTERPOLATION_ERROR::%s:%d:%d) The key \"%s\" contains an "
                                     "interpolated \"{\" operator, but appears to be missing a closing \"}\" operator.",
                                     _file_name.c_str(), _line_count + 1, _assignment_index + _val_byte_count + 2,
                                     _key.c_str())
                      << std::endl;
            break;
        }
        case LOG::DEBUG: {
            std::clog << fmt::format("[nvi] (Parser::DEBUG::%s:%d:%d) Set key \"%s\" to equal value \"%s\".",
                                     _file_name.c_str(), _line_count + 1, _assignment_index + _val_byte_count + 2,
                                     _key.c_str(), _value.c_str())
                      << std::endl;
            break;
        }
        case LOG::DEBUG_FILE_PROCESSED: {
            std::clog << fmt::format("[nvi] (Parser::DEBUG_FILE_PROCESSED::%s) Processed %d line%s and %d bytes!\n",
                                     _file_name.c_str(), _line_count, (_line_count != 1 ? "s" : ""), _byte_count)
                      << std::endl;
            break;
        }
        case LOG::REQUIRED_ENV_ERROR: {
            std::cerr << fmt::format(
                             "[nvi] (Parser::REQUIRED_ENV_ERROR) The following ENV keys are marked as required: \"%s\""
                             ", but they are undefined after all of the .env files were parsed.",
                             fmt::join(_undefined_keys, ", ").c_str())
                      << std::endl;
            break;
        }
        case LOG::FILE_ERROR: {
            std::cerr << fmt::format("[nvi] (Parser::FILE_ERROR) Unable to locate \"%s\". The .env file doesn't appear "
                                     "to exist or isn't a valid file format!",
                                     _file_path.c_str())
                      << std::endl;
            break;
        }
        case LOG::EMPTY_ENVS: {
            std::cerr << fmt::format(
                             "[nvi] (Parser::LOG::EMPTY_ENVS) Unable to parse any ENVS! Please ensure the \"%s\" file "
                             "is not empty.",
                             _file_name.c_str())
                      << std::endl;
            break;
        }
        default:
            break;
        }
    }
}; // namespace nvi
