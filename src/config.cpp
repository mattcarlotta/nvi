#include "config.h"
#include "format.h"
#include "log.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace nvi {

    inline constexpr char DEBUG_PROP[] = "debug";
    inline constexpr char DIR_PROP[] = "dir";
    inline constexpr char EXEC_PROP[] = "exec";
    inline constexpr char FILES_PROP[] = "files";
    inline constexpr char REQUIRED_PROP[] = "required";

    inline constexpr char COMMENT = '#';         // 0x23
    inline constexpr char SPACE = ' ';           // 0x20
    inline constexpr char OPEN_BRACKET = '[';    // 0x5b
    inline constexpr char CLOSE_BRACKET = ']';   // 0x5d
    inline constexpr char COMMA = ',';           // 0x2c
    inline constexpr char DOUBLE_QUOTE = '\"';   // 0x22
    inline constexpr char LINE_DELIMITER = '\n'; // 0x0a
    inline constexpr char ASSIGN_OP = '=';       // 0x3d

    Config::Config(const std::string &environment, const std::string _envdir) : _env(environment) {
        _file_path = std::filesystem::current_path() / _envdir / ".nvi";
        if (not std::filesystem::exists(_file_path)) {
            log(FILE_ENOENT_ERROR);
        }

        std::ifstream config_file{_file_path, std::ios_base::in};
        if (not config_file.is_open()) {
            log(FILE_ERROR);
        }

        _file = std::string{std::istreambuf_iterator<char>(config_file), std::istreambuf_iterator<char>()};
        const int config_index = _file.find(OPEN_BRACKET + environment + CLOSE_BRACKET);
        if (config_index < 0) {
            log(FILE_PARSE_ERROR);
        }

        std::string config{_file.substr(config_index, _file.length())};
        const int env_eol_index = config.find_first_of(LINE_DELIMITER);
        // remove environment name line
        std::istringstream config_iss{config.substr(env_eol_index + 1, _file.length())};

        std::string line;
        while (std::getline(config_iss, line)) {
            // skip comments in config options
            const int comment_index = line.find(COMMENT);
            if (comment_index >= 0) {
                continue;
            }

            // ensure current line is a key = value
            const int assignment_index = line.find(ASSIGN_OP);
            if (assignment_index < 0) {
                break;
            }

            _key = trim_surrounding_spaces(line.substr(0, assignment_index));
            _value = trim_surrounding_spaces(line.substr(assignment_index + 1, line.length() - 1));

            if (_key == DEBUG_PROP) {
                _options.debug = parse_bool_arg(DEBUG_ARG_ERROR);
            } else if (_key == DIR_PROP) {
                _options.dir = parse_string_arg(DIR_ARG_ERROR);
            } else if (_key == FILES_PROP) {
                _options.files.clear();
                _options.files = parse_vector_arg(FILES_ARG_ERROR);

                if (not _options.files.size()) {
                    log(EMPTY_FILES_ARG_ERROR);
                }
            } else if (_key == EXEC_PROP) {
                _command = parse_string_arg(EXEC_ARG_ERROR);
                std::stringstream command_iss{_command};
                std::string arg;

                while (command_iss >> arg) {
                    char *arg_cstr = new char[arg.length() + 1];
                    _options.commands.push_back(std::strcpy(arg_cstr, arg.c_str()));
                }

                _options.commands.push_back(nullptr);
            } else if (_key == REQUIRED_PROP) {
                _options.required_envs = parse_vector_arg(REQUIRED_ARG_ERROR);
            } else {
                log(INVALID_PROPERTY_WARNING);
            }
        }

        if (_options.debug) {
            log(DEBUG);
        }

        config_file.close();
    };

    const options_t &Config::get_options() const noexcept { return _options; }

    const std::string Config::trim_surrounding_spaces(const std::string &val) const noexcept {
        size_t begin = 0;
        size_t end = val.length() - 1;
        while (begin < end) {
            if (val[begin] != SPACE && val[end] != SPACE) {
                break;
            }
            if (val[begin] == SPACE) {
                ++begin;
            }
            if (val[end] == SPACE) {
                --end;
            }
        }

        return val.substr(begin, end - begin + 1);
    }

    bool Config::parse_bool_arg(const messages_t &code) const noexcept {
        if (_value != "true" && _value != "false") {
            log(code);
        }

        return _value == "true";
    }

    const std::string Config::parse_string_arg(const messages_t &code) const noexcept {
        if (_value[0] != DOUBLE_QUOTE || _value[_value.length() - 1] != DOUBLE_QUOTE) {
            log(code);
        }

        return _value.substr(1, _value.length() - 2);
    }

    const std::vector<std::string> Config::parse_vector_arg(const messages_t &code) const noexcept {
        if (_value[0] != OPEN_BRACKET || _value[_value.length() - 1] != CLOSE_BRACKET) {
            log(code);
        }

        static const std::unordered_set<char> SPECIAL_CHARS{OPEN_BRACKET, CLOSE_BRACKET, DOUBLE_QUOTE, COMMA, SPACE};
        std::string temp_val;
        std::vector<std::string> arg;
        for (const char &c : _value) {
            if (SPECIAL_CHARS.find(c) == SPECIAL_CHARS.end()) {
                temp_val += c;
                continue;
            } else if (temp_val.length()) {
                arg.push_back(temp_val);
            }

            temp_val.clear();
        }

        return arg;
    }

    void Config::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case FILE_ENOENT_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_ENOENT_ERROR,
                R"(Unable to locate "%s". The .nvi configuration file doesn't appear to exist at this path!)",
                _file_path.c_str());
            break;
        }
        case FILE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_ERROR,
                R"(Unable to open "%s". The .nvi configuration file is either invalid, has restricted access, or may be corrupted.)",
                _file_path.c_str());
            break;
        }
        case FILE_PARSE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_PARSE_ERROR,
                R"(Unable to load the "%s" environment from the .nvi configuration file (%s). The specified environment doesn't appear to exist!)",
                _env.c_str(), _file_path.c_str());
            break;
        }
        case DEBUG_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                DEBUG_ARG_ERROR,
                R"(The "debug" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value.c_str());
            break;
        }
        case DIR_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                DIR_ARG_ERROR,
                R"(The "dir" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value.c_str());
            break;
        }
        case FILES_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILES_ARG_ERROR,
                R"(The "files" property contains an invalid value. Expected a vector of strings, but instead received: %s.)",
                _value.c_str());
            break;
        }
        case EMPTY_FILES_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EMPTY_FILES_ARG_ERROR,
                R"(The "files" property within the "%s" environment configuration (%s) appears to be empty. You must specify at least 1 .env file to load!)",
                _env.c_str(), _file_path.c_str());
            break;
        }
        case EXEC_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EXEC_ARG_ERROR,
                R"(The "exec" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value.c_str());
            break;
        }
        case REQUIRED_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUIRED_ARG_ERROR,
                R"(The "required" property contains an invalid value. Expected an array of strings, but instead received: %s.)",
                _value.c_str());
            break;
        }
        case INVALID_PROPERTY_WARNING: {
            NVI_LOG_DEBUG(
                INVALID_PROPERTY_WARNING,
                R"(Found an invalid property: "%s" within the "%s" config. Skipping.)",
                _key.c_str(), _env.c_str());
            break;
        }
        case DEBUG: {
            NVI_LOG_DEBUG(
                DEBUG,
                R"(Successfully parsed the "%s" environment configuration and the folowing options were set: debug="true", dir="%s", execute="%s", files="%s", required="%s".)",
                _env.c_str(), _options.dir.c_str(), _command.c_str(), fmt::join(_options.files, ", ").c_str(),
                fmt::join(_options.required_envs, ", ").c_str());
            break;
        }
        default:
            break;
        }
        // clang-format on
    }
} // namespace nvi
