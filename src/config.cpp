#include "config.h"
#include "format.h"
#include "log.h"
#include "options.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace nvi {

    inline constexpr char COMMENT = '#';         // 0x23
    inline constexpr char SPACE = ' ';           // 0x20
    inline constexpr char OPEN_BRACKET = '[';    // 0x5b
    inline constexpr char CLOSE_BRACKET = ']';   // 0x5d
    inline constexpr char COMMA = ',';           // 0x2c
    inline constexpr char DOUBLE_QUOTE = '\"';   // 0x22
    inline constexpr char LINE_DELIMITER = '\n'; // 0x0a
    inline constexpr char ASSIGN_OP = '=';       // 0x3d

    Config::Config(options_t &options) : _options(options) {
        _file_path = std::filesystem::current_path() / _options.dir / ".nvi";
        if (not std::filesystem::exists(_file_path)) {
            log(FILE_ENOENT_ERROR);
        }

        std::ifstream config_file{_file_path, std::ios_base::in};
        if (not config_file.is_open()) {
            log(FILE_ERROR);
        }

        _file = std::string{std::istreambuf_iterator{config_file}, {}};
        _byte = _file.find(OPEN_BRACKET + _options.config + CLOSE_BRACKET);
        if (_byte == std::string::npos) {
            log(FILE_PARSE_ERROR);
        }

        const size_t eol_index = _file.substr(_byte, _file.length()).find_first_of(LINE_DELIMITER);
        if (eol_index == std::string::npos) {
            log(FILE_PARSE_ERROR);
        }

        // remove config name line: [environment]
        _config = _file.substr(eol_index + 1, _file.length());

        if (not peek().has_value()) {
            log(SELECTED_CONFIG_EMPTY_ERROR);
        }

        while (peek().has_value()) {
            std::optional<char> current_char = peek();

            // stop parsing if another config is found
            if (current_char.value() == OPEN_BRACKET) {
                break;
            }

            // skip empty spaces and new lines
            if (std::isspace(current_char.value())) {
                skip();
                continue;
            }

            // skip lines with comments
            if (current_char.value() == COMMENT) {
                skip_to_eol();
                continue;
            }

            // parse key = value pairs
            if (std::isalpha(current_char.value())) {
                ConfigToken token;

                // parse and assign token key
                while (current_char.has_value() && current_char.value() != ASSIGN_OP) {
                    current_char = peek();
                    if (not std::isalpha(current_char.value())) {
                        skip();
                    } else {
                        token.key.push_back(commit());
                    }
                }

                // assign current token.key to _key just for logging errors
                _key = token.key;

                // parse and assign value
                while (current_char.has_value()) {
                    current_char = peek();

                    // skip empty spaces and new lines
                    if (std::isblank(current_char.value())) {
                        skip();
                        continue;
                    }

                    // parse and extract a boolean value
                    if (std::isalpha(current_char.value())) {
                        token.type = ConfigValueType::boolean;

                        std::string extracted_value = extract_value_within(LINE_DELIMITER);
                        const bool a_true_value = extracted_value.find("true") != std::string::npos;
                        const bool a_false_value = extracted_value.find("false") != std::string::npos;

                        // ensure value is a bool
                        if (not a_true_value && not a_false_value) {
                            log(INVALID_BOOLEAN_VALUE);
                        }

                        token.value = a_true_value;
                        _config_tokens.push_back(token);

                        break;
                    }

                    // parse and extract a string value
                    if (current_char.value() == DOUBLE_QUOTE) {
                        token.type = ConfigValueType::string;
                        // skip over double quote
                        skip();

                        std::string value = extract_value_within(DOUBLE_QUOTE, true);

                        // ensure a value exists to prevent consuming other properties
                        if (value.length() == 0) {
                            log(INVALID_STRING_VALUE);
                        }

                        token.value = value;
                        _config_tokens.push_back(token);

                        // skip over double quote
                        skip();
                        break;
                    }

                    // parse and extract an array of values
                    if (current_char.value() == OPEN_BRACKET) {
                        token.type = ConfigValueType::array;

                        // skip over "["
                        skip();

                        std::vector<std::string> values;
                        while (_byte < _config.length()) {
                            current_char = peek();

                            // running into an assignment or open bracket means a closing bracket is missing
                            if (not current_char.has_value() || current_char.value() == ASSIGN_OP ||
                                current_char.value() == OPEN_BRACKET) {
                                log(INVALID_ARRAY_VALUE);
                            } else if (current_char.value() == CLOSE_BRACKET) {
                                break;
                            } else if (current_char.value() == DOUBLE_QUOTE) {
                                // skip over double quote
                                skip();

                                const std::string next_value = extract_value_within(DOUBLE_QUOTE);

                                // prevent duplicate values
                                if (std::find(values.begin(), values.end(), next_value) == values.end()) {
                                    values.push_back(next_value);
                                }
                            }

                            // skip characters not within quotes
                            skip();
                        }

                        // ensure the current character is a closing bracket, otherwise it's missing,
                        // therefore the extracted values aren't correct
                        if (current_char.has_value() && current_char.value() != CLOSE_BRACKET) {
                            log(INVALID_ARRAY_VALUE);
                        }

                        token.value = values;
                        _config_tokens.push_back(token);

                        // skip over "]"
                        skip();

                        break;
                    }

                    break;
                }
            }
            skip();
        }

        if (_config_tokens.size() == 0) {
            log(SELECTED_CONFIG_EMPTY_ERROR);
        }

        config_file.close();
    };

    const std::vector<ConfigToken> &Config::get_tokens() const noexcept { return _config_tokens; }

    Config *Config::generate_options() noexcept {
        for (const ConfigToken &ct : _config_tokens) {
            _key = ct.key;
            _value_type = get_value_type_string(ct.type);

            if (not ct.value.has_value()) {
                continue;
            }

            switch (CONFIG_KEYS.count(ct.key) ? CONFIG_KEYS.at(ct.key) : CONFIG_KEY::UNKNOWN) {
            case CONFIG_KEY::API: {
                if (ct.type != ConfigValueType::boolean) {
                    log(API_ARG_ERROR);
                }

                _options.api = std::get<bool>(ct.value.value());
                break;
            }
            case CONFIG_KEY::DEBUG: {
                if (ct.type != ConfigValueType::boolean) {
                    log(DEBUG_ARG_ERROR);
                }

                _options.debug = std::get<bool>(ct.value.value());
                break;
            }
            case CONFIG_KEY::DIRECTORY: {
                if (ct.type != ConfigValueType::string) {
                    log(DIR_ARG_ERROR);
                }

                _options.dir = std::move(std::get<std::string>(ct.value.value()));
                break;
            }
            case CONFIG_KEY::FILES: {
                if (ct.type != ConfigValueType::array) {
                    log(FILES_ARG_ERROR);
                }

                _options.files.clear();
                _options.files = std::move(std::get<std::vector<std::string>>(ct.value.value()));

                if (not _options.files.size()) {
                    log(EMPTY_FILES_ARG_ERROR);
                }
                break;
            }
            case CONFIG_KEY::ENVIRONMENT: {
                if (ct.type != ConfigValueType::string) {
                    log(ENV_ARG_ERROR);
                }

                _options.environment = std::move(std::get<std::string>(ct.value.value()));
                break;
            }
            case CONFIG_KEY::EXECUTE: {
                if (ct.type != ConfigValueType::string) {
                    log(EXEC_ARG_ERROR);
                }

                _command = std::move(std::get<std::string>(ct.value.value()));
                std::stringstream command_iss{_command};
                std::string arg;

                while (command_iss >> arg) {
                    char *arg_cstr = new char[arg.length() + 1];
                    _options.commands.push_back(std::strcpy(arg_cstr, arg.c_str()));
                }

                _options.commands.push_back(nullptr);
                break;
            }
            case CONFIG_KEY::PRINT: {
                if (ct.type != ConfigValueType::boolean) {
                    log(PRINT_ARG_ERROR);
                }

                _options.print = std::get<bool>(ct.value.value());
                break;
            }
            case CONFIG_KEY::PROJECT: {
                if (ct.type != ConfigValueType::string) {
                    log(PROJECT_ARG_ERROR);
                }

                _options.project = std::move(std::get<std::string>(ct.value.value()));
                break;
            }
            case CONFIG_KEY::REQUIRED: {
                if (ct.type != ConfigValueType::array) {
                    log(REQUIRED_ARG_ERROR);
                }

                _options.required_envs = std::move(std::get<std::vector<std::string>>(ct.value.value()));
                break;
            }
            case CONFIG_KEY::SAVE: {
                if (ct.type != ConfigValueType::boolean) {
                    log(SAVE_ARG_ERROR);
                }

                _options.save = std::get<bool>(ct.value.value());
                break;
            }
            default: {
                log(INVALID_PROPERTY_WARNING);
                break;
            }
            }
        }

        if (_options.debug) {
            log(DEBUG);
        }

        return this;
    }

    void Config::skip_to_eol() noexcept {
        while (peek().has_value() && peek().value() != LINE_DELIMITER) {
            skip();
        }
    }

    std::optional<char> Config::peek(int offset) const noexcept {
        if (_byte + offset >= _config.length()) {
            return std::nullopt;
        } else {
            return _config.at(_byte + offset);
        }
    }

    char Config::commit() noexcept { return _config.at(_byte++); }

    void Config::skip(int offset) noexcept { _byte += offset; }

    const std::string Config::extract_value_within(char delimiter, bool error_at_new_line) noexcept {
        std::string value;
        while (peek().has_value() && peek().value() != delimiter) {
            if (error_at_new_line && peek().value() == LINE_DELIMITER) {
                // handle values missing closing tags
                return "";
            } else if (peek().value() == COMMENT) {
                skip_to_eol();
            } else {
                value.push_back(commit());
            }
        }
        return value;
    }

    std::string Config::get_value_type_string(const ConfigValueType &cvt) const noexcept {
        switch (cvt) {
        case ConfigValueType::array:
            return "an array";
        case ConfigValueType::boolean:
            return "a boolean";
        default:
            return "a string";
        }
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
                R"(Unable to load the "%s" configuration from the .nvi file (%s). The specified config doesn't appear to exist!)",
                _options.config.c_str(), _file_path.c_str());
            break;
        }
        case SELECTED_CONFIG_EMPTY_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                SELECTED_CONFIG_EMPTY_ERROR,
                R"(While the "%s" configuration exists within the .nvi file (%s), the configuration appears to be empty.)",
                _options.config.c_str(), _file_path.c_str());
            break;
        }
        case INVALID_ARRAY_VALUE: {
            NVI_LOG_ERROR_AND_EXIT(
                INVALID_ARRAY_VALUE,
                R"(The "%s" property within the "%s" config is not a valid array. It appears to be missing a closing bracket "]".)",
                _key.c_str(), _options.config.c_str());
            break;
        }
        case INVALID_BOOLEAN_VALUE: {
            NVI_LOG_ERROR_AND_EXIT(
                INVALID_BOOLEAN_VALUE,
                R"(The "%s" property within the "%s" config contains an invalid boolean value. Expected the value to match true or false.)",
                _key.c_str(), _options.config.c_str());
            break;
        }
        case INVALID_STRING_VALUE: {
            NVI_LOG_ERROR_AND_EXIT(
                INVALID_STRING_VALUE,
                R"(The "%s" property within the "%s" config contains an invalid string value. It appears to be empty or is missing a closing double quote.)",
                _key.c_str(), _options.config.c_str());
            break;
        }
        case API_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                API_ARG_ERROR,
                R"(The "api" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case DEBUG_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                DEBUG_ARG_ERROR,
                R"(The "debug" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case DIR_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                DIR_ARG_ERROR,
                R"(The "directory" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case ENV_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                ENV_ARG_ERROR,
                R"(The "environment" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case FILES_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILES_ARG_ERROR,
                R"(The "files" property contains an invalid value. Expected an array of strings, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case EMPTY_FILES_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EMPTY_FILES_ARG_ERROR,
                R"(The "files" property within the "%s" environment configuration (%s) appears to be empty. You must specify at least 1 .env file to load!)",
                _options.config.c_str(), _file_path.c_str());
            break;
        }
        case EXEC_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EXEC_ARG_ERROR,
                R"(The "execute" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case PRINT_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                PRINT_ARG_ERROR,
                R"(The "print" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case PROJECT_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                PROJECT_ARG_ERROR,
                R"(The "project" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case REQUIRED_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUIRED_ARG_ERROR,
                R"(The "required" property contains an invalid value. Expected an array of strings, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case SAVE_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                SAVE_ARG_ERROR,
                R"(The "save" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case INVALID_PROPERTY_WARNING: {
            NVI_LOG_DEBUG(
                INVALID_PROPERTY_WARNING,
                R"(Found an invalid property: "%s" within the "%s" config. Skipping.)",
                _key.c_str(), _options.config.c_str());
            break;
        }
        case DEBUG: {
            for (size_t index = 0; index < _config_tokens.size(); ++index) {
                const ConfigToken &token = _config_tokens.at(index);

                std::stringstream ss;
                ss << "Created " << get_value_type_string(token.type) << " token with a key of " << std::quoted(token.key);
                ss << " and a value of " << std::quoted(std::visit(ConfigTokenToString(), token.value.value())) << ".";
                if(index == _config_tokens.size()) {
                    ss << '\n';
                }

                NVI_LOG_DEBUG(DEBUG, ss.str().c_str(), NULL);
            }

            NVI_LOG_DEBUG(
                DEBUG,
                R"(Successfully parsed the "%s" configuration from the .nvi file.)",
                _options.config.c_str());

            if (_options.commands.size() && _options.print) {
                NVI_LOG_DEBUG(
                    DEBUG,
                    R"(Found conflicting flags. When commands are present, then the "print" flag is ignored.)",
                    NULL);
            }

            NVI_LOG_DEBUG(
                DEBUG,
                R"(The following config options were set: api="%s", debug="true", directory="%s", environment="%s", execute="%s", files="%s", print="%s", project="%s", required="%s", save="%s".)",
                (_options.api ? "true": "false"),
                _options.dir.c_str(), 
                _options.environment.c_str(), 
                _command.c_str(), 
                fmt::join(_options.files, ", ").c_str(), 
                (_options.print ? "true": "false"),
                _options.project.c_str(), 
                fmt::join(_options.required_envs, ", ").c_str(),
                (_options.save ? "true": "false"));
            break;
        }
        default:
            break;
        }
        // clang-format on
    }
} // namespace nvi
