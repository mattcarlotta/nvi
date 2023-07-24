#include "config.h"
#include "format.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace nvi {

    enum MESSAGES {
        FILE_ERROR = 0,
        FILE_PARSE_ERROR = 1,
        DEBUG = 2,
        MISSING_FILES_ARG_ERROR = 3,
    };

    constexpr char SPACE = ' ';           // 0x00
    constexpr char OPEN_BRACKET = '[';    // 0x5b
    constexpr char CLOSE_BRACKET = ']';   // 0x5d
    constexpr char COMMA = ',';           // 0x2c
    constexpr char DOUBLE_QUOTE = '\"';   // 0x22
    constexpr char LINE_DELIMITER = '\n'; // 0x0a
    constexpr char ASSIGN_OP = '=';       // 0x3d

    Config::Config(const std::string &environment, const std::string _envdir) : _env(environment) {
        static const std::unordered_set<char> INVALID_CHARS = {OPEN_BRACKET, CLOSE_BRACKET, DOUBLE_QUOTE, COMMA, SPACE};
        _file_path = std::string{std::filesystem::current_path() / _envdir / ".nvi"};
        if (not std::filesystem::exists(_file_path)) {
            log(MESSAGES::FILE_ERROR);
            std::exit(1);
        }

        std::ifstream config_file = std::ifstream{_file_path, std::ios_base::in};
        _file = std::string{std::istreambuf_iterator<char>(config_file), std::istreambuf_iterator<char>()};
        _file_view = std::string_view{_file};

        const int open_bracket_index = _file_view.find("[" + environment + "]");
        if (open_bracket_index < 0) {
            log(MESSAGES::FILE_PARSE_ERROR);
            std::exit(1);
        }

        // factor in '[' + ']' + '\n' character bytes
        std::string_view config_options =
            _file_view.substr(open_bracket_index + environment.length() + 3, _file_view.length());

        int eol_index = config_options.find(LINE_DELIMITER);
        while (eol_index >= 0) {
            std::string_view line = config_options.substr(0, eol_index);

            // ensure current line is a key = value
            const int assignment_index = line.find(ASSIGN_OP);
            if (assignment_index < 0) {
                break;
            }

            std::string key;
            // remove all spaces
            for (const char &c : line.substr(0, assignment_index)) {
                if (c != SPACE) {
                    key += c;
                }
            }

            std::string_view value;
            // remove any leading spaces
            for (int i = 1; i < eol_index; ++i) {
                if (line[assignment_index + i] != SPACE) {
                    value = line.substr(assignment_index + i, eol_index - 1);
                    break;
                }
            }
            const char first_char = value[0];
            const char last_char = value[value.length() - 1];

            if (key == "debug") {
                if (value != "true" && value != "false") {
                    std::cerr << "Invalid debug value" << std::endl;
                    std::exit(1);
                }
                _options.debug = value == "true";
            } else if (key == "dir") {
                if (first_char != DOUBLE_QUOTE || last_char != DOUBLE_QUOTE) {
                    std::cerr << "Invalid dir value" << std::endl;
                    std::exit(1);
                }
                _options.dir = value.substr(1, value.length() - 2);
            } else if (key == "files") {
                if (first_char != OPEN_BRACKET || last_char != CLOSE_BRACKET) {
                    std::cerr << "Invalid files value" << std::endl;
                    std::exit(1);
                }
                _options.files.clear();
                std::string temp_value;
                for (const char &c : value) {
                    if (INVALID_CHARS.find(c) == INVALID_CHARS.end()) {
                        temp_value += c;
                        continue;
                    }

                    if (temp_value.length()) {
                        _options.files.push_back(temp_value);
                    }

                    temp_value.clear();
                }

                if (not _options.files.size()) {
                    log(MESSAGES::MISSING_FILES_ARG_ERROR);
                    std::exit(1);
                }
            } else if (key == "exec") {
                if (first_char != DOUBLE_QUOTE || last_char != DOUBLE_QUOTE) {
                    std::cerr << "Invalid exec value" << std::endl;
                    std::exit(1);
                }
                _command = std::string{value.substr(1, value.size() - 2)};
                std::stringstream _commandiss(_command);
                std::string arg;

                while (_commandiss >> arg) {
                    char *arg_cstr = new char[arg.length() + 1];
                    std::strcpy(arg_cstr, arg.c_str());
                    _options.commands.push_back(arg_cstr);
                }

                _options.commands.push_back(nullptr);
            } else if (key == "required") {
                if (first_char != OPEN_BRACKET || last_char != CLOSE_BRACKET) {
                    std::cerr << "Invalid required value" << std::endl;
                    std::exit(1);
                }
                std::string temp_value;
                for (const char &c : value) {
                    if (INVALID_CHARS.find(c) == INVALID_CHARS.end()) {
                        temp_value += c;
                        continue;
                    }

                    if (temp_value.length()) {
                        _options.required_envs.push_back(temp_value);
                    }

                    temp_value.clear();
                }
            } else {
                // invalid character
            }

            config_options = config_options.substr(eol_index + 1, _file_view.length());
            eol_index = config_options.find('\n');
        }

        if (_options.debug) {
            log(MESSAGES::DEBUG);
        }
    };

    const Options &Config::get_options() const noexcept { return _options; }

    void Config::log(const uint_least8_t &code) const noexcept {
        switch (code) {
        case MESSAGES::FILE_ERROR: {
            std::cerr
                << fmt::format(
                       "[nvi] (config::FILE_ERROR) Unable to locate \"%s\". The configuration file doesn't appear "
                       "to exist!",
                       _file_path.c_str())
                << std::endl;
            break;
        }
        case MESSAGES::FILE_PARSE_ERROR: {
            std::cerr << fmt::format(
                             "[nvi] (config::FILE_PARSE_ERROR) Unable to load a \"%s\" environment from the "
                             "nvi.json configuration file (%s). The specified environment doesn't appear to exist!",
                             _env.c_str(), _file_path.c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::DEBUG: {
            std::clog << fmt::format("[nvi] (config::DEBUG) The following flags were set: "
                                     "debug=\"true\", dir=\"%s\", execute=\"%s\", files=\"%s\", required=\"%s\".\n",
                                     _options.dir.c_str(), _command.c_str(), fmt::join(_options.files, ", ").c_str(),
                                     fmt::join(_options.required_envs, ", ").c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::MISSING_FILES_ARG_ERROR: {
            std::cerr << fmt::format(
                             "[nvi] (config::MISSING_FILES_ARG_ERROR) Unable to locate a \"files\" property within the "
                             "\"%s\" environment configuration (%s). You must specify at least 1 .env file to load!",
                             _env.c_str(), _file_path.c_str())
                      << std::endl;
            break;
        }
        default:
            break;
        }
    }
} // namespace nvi
