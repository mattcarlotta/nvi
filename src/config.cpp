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

    inline const constexpr char DEBUG_PROP[] = "debug";
    inline const constexpr char DIR_PROP[] = "dir";
    inline const constexpr char EXEC_PROP[] = "exec";
    inline const constexpr char FILES_PROP[] = "files";
    inline const constexpr char REQUIRED_PROP[] = "required";

    inline const constexpr char COMMENT = '#';         // 0x23
    inline const constexpr char SPACE = ' ';           // 0x20
    inline const constexpr char OPEN_BRACKET = '[';    // 0x5b
    inline const constexpr char CLOSE_BRACKET = ']';   // 0x5d
    inline const constexpr char COMMA = ',';           // 0x2c
    inline const constexpr char DOUBLE_QUOTE = '\"';   // 0x22
    inline const constexpr char LINE_DELIMITER = '\n'; // 0x0a
    inline const constexpr char ASSIGN_OP = '=';       // 0x3d

    Config::Config(const std::string &environment, const std::string _envdir) : _env(environment) {
        _file_path = std::filesystem::current_path() / _envdir / ".nvi";
        if (not std::filesystem::exists(_file_path)) {
            log(FILE_ERROR);
        }

        std::ifstream config_file{_file_path, std::ios_base::in};
        if (config_file.bad() || not config_file.is_open()) {
            log(FILE_ERROR);
        }

        _file = std::string{std::istreambuf_iterator<char>(config_file), std::istreambuf_iterator<char>()};
        const int config_index = _file.find(environment);
        if (config_index < 0) {
            log(FILE_PARSE_ERROR);
        }

        std::string config{_file.substr(config_index, _file.length())};
        const int env_eol_index = config.find_first_of(LINE_DELIMITER);
        // remove environment name line
        std::istringstream configiss{config.substr(env_eol_index + 1, _file.length())};

        std::string line;
        while (std::getline(configiss, line)) {
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
                    log(MISSING_FILES_ARG_ERROR);
                }
            } else if (_key == EXEC_PROP) {
                _command = std::string{parse_string_arg(EXEC_ARG_ERROR)};
                std::stringstream commandiss{_command};
                std::string arg;

                while (commandiss >> arg) {
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

    const std::string Config::trim_surrounding_spaces(const std::string &val) noexcept {
        int l = 0;
        int r = val.length() - 1;
        while (l < r) {
            if (val[l] != SPACE && val[r] != SPACE) {
                break;
            }
            if (val[l] == SPACE) {
                ++l;
            }
            if (val[r] == SPACE) {
                --r;
            }
        }

        return val.substr(l, r - l + 1);
    }

    bool Config::parse_bool_arg(const unsigned int &code) const noexcept {
        if (_value != "true" && _value != "false") {
            log(code);
        }

        return _value == "true";
    }

    const std::string Config::parse_string_arg(const unsigned int &code) const noexcept {
        if (_value[0] != DOUBLE_QUOTE || _value[_value.length() - 1] != DOUBLE_QUOTE) {
            log(code);
        }

        return _value.substr(1, _value.length() - 2);
    }

    const std::vector<std::string> Config::parse_vector_arg(const unsigned int &code) const noexcept {
        if (_value[0] != OPEN_BRACKET || _value[_value.length() - 1] != CLOSE_BRACKET) {
            log(code);
        }

        static const std::unordered_set<char> SPECIAL_CHARS = {OPEN_BRACKET, CLOSE_BRACKET, DOUBLE_QUOTE, COMMA, SPACE};
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

    void Config::log(const unsigned int &code) const noexcept {
        // clang-format off
        switch (code) {
        case FILE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_ERROR,
                "Unable to locate \"%s\". The configuration file doesn't appear to exist!",
                _file_path.c_str());
            break;
        }
        case FILE_PARSE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_PARSE_ERROR,
                "Unable to load a \"%s\" environment from the .nvi configuration file (%s). The specified "
                "environment doesn't appear to exist!",
                _env.c_str(), _file_path.c_str());
            break;
        }
        case DEBUG_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                DEBUG_ARG_ERROR,
                "The \"debug\" property contains an invalid value. Expected a boolean value, but instead "
                "received: %s.",
                std::string{_value}.c_str());
            break;
        }
        case DIR_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                DIR_ARG_ERROR,
                "The \"dir\" property contains an invalid value. Expected a string value, but instead "
                "received: %s.",
                std::string{_value}.c_str());
            break;
        }
        case FILES_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILES_ARG_ERROR,
                "The \"files\" property contains an invalid value. Expected a vector of strings, but "
                "instead received: %s.",
                std::string{_value}.c_str());
            break;
        }
        case MISSING_FILES_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                MISSING_FILES_ARG_ERROR,
                "Unable to locate a \"files\" property within the \"%s\" environment configuration (%s). You"
                "must specify at least 1 .env file to load!",
                _env.c_str(), _file_path.c_str());
            break;
        }
        case EXEC_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EXEC_ARG_ERROR,
                "The \"exec\" property contains an invalid value. Expected a string value, but instead "
                "received: %s.",
                std::string{_value}.c_str());
            break;
        }
        case REQUIRED_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUIRED_ARG_ERROR,
                "The \"required\" property contains an invalid value. Expected a vector of strings, but instead "
                "received: %s.",
                std::string{_value}.c_str());
            break;
        }
        case INVALID_PROPERTY_WARNING: {
            NVI_LOG_DEBUG(
                INVALID_PROPERTY_WARNING,
                "Found an invalid property: \"%s\" within the \"%s\" config. Skipping.",
                std::string{_key}.c_str(), _env.c_str());
            break;
        }
        case DEBUG: {
            NVI_LOG_DEBUG(
                DEBUG,
                "Successfully parsed the \"%s\" environment configuration and the folowing options "
                "were set: debug=\"true\", dir=\"%s\", execute=\"%s\", files=\"%s\", required=\"%s\".",
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
