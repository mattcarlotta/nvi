#include "config.h"
#include "format.h"
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
        DEBUG_ARG_ERROR = 2,
        DIR_ARG_ERROR = 3,
        FILES_ARG_ERROR = 4,
        MISSING_FILES_ARG_ERROR = 5,
        EXEC_ARG_ERROR = 6,
        REQUIRED_ARG_ERROR = 7,
        INVALID_PROPERTY_WARNING = 8,
        DEBUG = 9
    };

    constexpr char DEBUG_PROP[] = "debug";
    constexpr char DIR_PROP[] = "dir";
    constexpr char EXEC_PROP[] = "exec";
    constexpr char FILES_PROP[] = "files";
    constexpr char REQUIRED_PROP[] = "required";

    constexpr char COMMENT = '#';         // 0x23
    constexpr char SPACE = ' ';           // 0x00
    constexpr char OPEN_BRACKET = '[';    // 0x5b
    constexpr char CLOSE_BRACKET = ']';   // 0x5d
    constexpr char COMMA = ',';           // 0x2c
    constexpr char DOUBLE_QUOTE = '\"';   // 0x22
    constexpr char LINE_DELIMITER = '\n'; // 0x0a
    constexpr char ASSIGN_OP = '=';       // 0x3d

    Config::Config(const std::string &environment, const std::string _envdir) : _env(environment) {
        _file_path = std::string{std::filesystem::current_path() / _envdir / ".nvi"};
        if (not std::filesystem::exists(_file_path)) {
            log(MESSAGES::FILE_ERROR);
            std::exit(1);
        }

        std::ifstream config_file = std::ifstream{_file_path, std::ios_base::in};
        _file = std::string{std::istreambuf_iterator<char>(config_file), std::istreambuf_iterator<char>()};
        _file_view = std::string_view{_file};

        const int config_index = _file_view.find("[" + environment + "]");
        if (config_index < 0) {
            log(MESSAGES::FILE_PARSE_ERROR);
            std::exit(1);
        }

        // factor in '[' + ']' + '\n' character bytes
        std::string_view config_options =
            _file_view.substr(config_index + environment.length() + 3, _file_view.length());

        int eol_index = config_options.find(LINE_DELIMITER);
        while (eol_index >= 0) {
            std::string_view line = config_options.substr(0, eol_index);

            // skip comments in config options
            const int comment_index = line.find(COMMENT);
            if (comment_index >= 0) {
                config_options = config_options.substr(eol_index + 1, _file_view.length());
                eol_index = config_options.find('\n');
                continue;
            }

            // ensure current line is a key = value
            const int assignment_index = line.find(ASSIGN_OP);
            if (assignment_index < 0) {
                break;
            }

            _key = trim_surrounding_spaces(line.substr(0, assignment_index));
            _value = trim_surrounding_spaces(line.substr(assignment_index + 1, eol_index - 1));

            if (_key == DEBUG_PROP) {
                if (_value != "true" && _value != "false") {
                    log(MESSAGES::DEBUG_ARG_ERROR);
                    std::exit(1);
                }
                _options.debug = _value == "true";
            } else if (_key == DIR_PROP) {
                _options.dir = parse_string_arg(MESSAGES::DIR_ARG_ERROR);
            } else if (_key == FILES_PROP) {
                _options.files.clear();
                _options.files = parse_vector_arg(MESSAGES::FILES_ARG_ERROR);

                if (not _options.files.size()) {
                    log(MESSAGES::MISSING_FILES_ARG_ERROR);
                    std::exit(1);
                }
            } else if (_key == EXEC_PROP) {
                _command = std::string{parse_string_arg(MESSAGES::EXEC_ARG_ERROR)};
                std::stringstream commandiss{_command};
                std::string arg;

                while (commandiss >> arg) {
                    char *arg_cstr = new char[arg.length() + 1];
                    _options.commands.push_back(std::strcpy(arg_cstr, arg.c_str()));
                }

                _options.commands.push_back(nullptr);
            } else if (_key == REQUIRED_PROP) {
                _options.required_envs = parse_vector_arg(MESSAGES::REQUIRED_ARG_ERROR);
            } else {
                log(MESSAGES::INVALID_PROPERTY_WARNING);
            }

            config_options = config_options.substr(eol_index + 1, _file_view.length());
            eol_index = config_options.find('\n');
        }

        if (_options.debug) {
            log(MESSAGES::DEBUG);
        }
    };

    const Options &Config::get_options() const noexcept { return _options; }

    const std::string_view Config::trim_surrounding_spaces(const std::string_view &val) noexcept {
        int left_index = 0;
        int right_index = val.length() - 1;
        while (left_index < right_index) {
            if (val[left_index] != SPACE && val[right_index] != SPACE) {
                break;
            }
            if (val[left_index] == SPACE) {
                ++left_index;
            }
            if (val[right_index] == SPACE) {
                --right_index;
            }
        }

        return val.substr(left_index, right_index - left_index + 1);
    }

    const std::string_view Config::parse_string_arg(const uint_least8_t &code) const {
        if (_value[0] != DOUBLE_QUOTE || _value[_value.length() - 1] != DOUBLE_QUOTE) {
            log(code);
            std::exit(1);
        }

        return _value.substr(1, _value.length() - 2);
    }

    const std::vector<std::string> Config::parse_vector_arg(const uint_least8_t &code) const {
        if (_value[0] != OPEN_BRACKET || _value[_value.length() - 1] != CLOSE_BRACKET) {
            log(code);
            std::exit(1);
        }

        static const std::unordered_set<char> SPECIAL_CHARS = {OPEN_BRACKET, CLOSE_BRACKET, DOUBLE_QUOTE, COMMA, SPACE};
        std::string val;
        std::vector<std::string> arg;
        for (const char &c : _value) {
            if (SPECIAL_CHARS.find(c) == SPECIAL_CHARS.end()) {
                val += c;
                continue;
            } else if (val.length()) {
                arg.push_back(val);
            }

            val.clear();
        }

        return arg;
    }

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
            std::cerr << fmt::format("[nvi] (config::FILE_PARSE_ERROR) Unable to load a \"%s\" environment from the "
                                     ".nvi configuration file (%s). The specified environment doesn't appear to exist!",
                                     _env.c_str(), _file_path.c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::DEBUG_ARG_ERROR: {
            std::cerr << fmt::format("[nvi] (config::DEBUG_ARG_ERROR) The \"debug\" property contains an "
                                     "invalid value. Expected a boolean value, but instead received: %s.",
                                     std::string{_value}.c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::DIR_ARG_ERROR: {
            std::cerr << fmt::format("[nvi] (config::DIR_ARG_ERROR) The \"dir\" property contains an "
                                     "invalid value. Expected a string value, but instead received: %s.",
                                     std::string{_value}.c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::FILES_ARG_ERROR: {
            std::cerr << fmt::format("[nvi] (config::FILES_ARG_ERROR) The \"files\" property contains an "
                                     "invalid value. Expected a vector of strings, but instead received: %s.",
                                     std::string{_value}.c_str())
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
        case MESSAGES::EXEC_ARG_ERROR: {
            std::cerr << fmt::format("[nvi] (config::EXEC_ARG_ERROR) The \"exec\" property contains an "
                                     "invalid value. Expected a string value, but instead received: %s.",
                                     std::string{_value}.c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::REQUIRED_ARG_ERROR: {
            std::cerr << fmt::format("[nvi] (config::REQUIRED_ARG_ERROR) The \"required\" property contains an "
                                     "invalid value. Expected a vector of strings, but instead received: %s.",
                                     std::string{_value}.c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::INVALID_PROPERTY_WARNING: {
            std::clog << fmt::format("[nvi] (config::INVALID_PROPERTY_WARNING) Found an invalid property: \"%s\" "
                                     "within the \"%s\" config. Skipping.",
                                     std::string{_key}.c_str(), _env.c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::DEBUG: {
            std::clog << fmt::format(
                             "[nvi] (config::DEBUG) Successfully parsed the \"%s\" environment configuration and the "
                             "following options were set: debug=\"true\", dir=\"%s\", execute=\"%s\", files=\"%s\", "
                             "required=\"%s\".\n",
                             _env.c_str(), _options.dir.c_str(), _command.c_str(),
                             fmt::join(_options.files, ", ").c_str(), fmt::join(_options.required_envs, ", ").c_str())
                      << std::endl;
            break;
        }
        default:
            break;
        }
    }
} // namespace nvi
