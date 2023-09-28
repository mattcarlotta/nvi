#include "config.h"
#include "format.h"
#include "log.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace nvi {

    inline constexpr char DEBUG_PROP[] = "debug";
    inline constexpr char DIR_PROP[] = "dir";
    inline constexpr char ENV_PROP[] = "env";
    inline constexpr char EXEC_PROP[] = "exec";
    inline constexpr char FILES_PROP[] = "files";
    inline constexpr char PROJECT_PROP[] = "project";
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
        _byte = _file.find(OPEN_BRACKET + environment + CLOSE_BRACKET);
        if (_byte == std::string::npos) {
            log(FILE_PARSE_ERROR);
        }

        const size_t eol_index = _file.substr(_byte, _file.length()).find_first_of(LINE_DELIMITER);
        if (eol_index == std::string::npos) {
            log(FILE_PARSE_ERROR);
        }

        // remove config name line: [environment]
        _config = _file.substr(eol_index + 1, _file.length());

        // TODO(carlotta): add log error message here
        if (_config.length() == 0) {
            std::cerr << "Invalid config file" << std::endl;
            std::exit(EXIT_FAILURE);
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
                        token.key += commit();
                    }
                }

                // parse and assign value
                while (current_char.has_value()) {
                    current_char = peek();

                    // skip empty spaces and new lines
                    if (std::isblank(current_char.value())) {
                        skip();
                        continue;
                    }

                    // most likely a boolean value
                    if (std::isalpha(current_char.value())) {
                        token.type = ConfigValueType::boolean;

                        token.value = extract_value_within(LINE_DELIMITER);
                        _config_tokens.push_back(token);

                        break;
                    }

                    // parse and extract a string valud
                    if (current_char.value() == DOUBLE_QUOTE) {
                        token.type = ConfigValueType::string;
                        // skip over double quote
                        skip();

                        token.value = extract_value_within(DOUBLE_QUOTE);
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
                        while (current_char.has_value() && current_char.value() != CLOSE_BRACKET) {
                            current_char = peek();
                            if (current_char.value() == ASSIGN_OP) {
                                // TODO(carlotta): add error message log here
                                std::clog << "ERROR: The key \"" << token.key
                                          << "\" appears to be missing a closing \"]\"!" << std::endl;
                                std::exit(EXIT_FAILURE);
                            } else if (current_char.value() == DOUBLE_QUOTE) {
                                // skip over double quote
                                skip();

                                values.push_back(extract_value_within(DOUBLE_QUOTE));
                            }

                            skip();
                        }

                        token.value = values;
                        _config_tokens.push_back(token);

                        break;
                    }

                    break;
                }
            }
            skip();
        }

        // TODO(carlotta): add debug log here
        for (const auto &ct : _config_tokens) {
            std::clog << "TOKEN TYPE: " << get_value_type_string(ct.type) << ", KEY: " << ct.key;
            if (ct.value.has_value()) {
                std::cout << ", VALUE: " << std::visit(ConfigTokenToString(), ct.value.value()) << "\n";
            }
        }
        std::clog << std::endl;

        config_file.close();
    };

    const options_t &Config::get_options() const noexcept { return _options; }

    const std::vector<ConfigToken> &Config::get_tokens() const noexcept { return _config_tokens; }

    Config *Config::generate_options() noexcept {
        for (const auto &ct : _config_tokens) {
            _key = ct.key;
            _value_type = get_value_type_string(ct.type);
            if (not ct.value.has_value()) {
                continue;
            } else if (ct.key == DEBUG_PROP) {
                if (ct.type != ConfigValueType::boolean) {
                    // TODO(carlotta): add error message log here
                    std::cerr << "debug is not a valid type." << std::endl;
                    std::exit(EXIT_FAILURE);
                }
                _options.debug = std::get<std::string>(ct.value.value()).find("true") != std::string::npos;
            } else if (_key == DIR_PROP) {
                if (ct.type != ConfigValueType::string) {
                    log(DIR_ARG_ERROR);
                }
                _options.dir = std::move(std::get<std::string>(ct.value.value()));
            } else if (_key == FILES_PROP) {
                if (ct.type != ConfigValueType::array) {
                    log(FILES_ARG_ERROR);
                }

                _options.files.clear();
                _options.files = std::move(std::get<std::vector<std::string>>(ct.value.value()));

                if (not _options.files.size()) {
                    log(EMPTY_FILES_ARG_ERROR);
                }
            } else if (_key == ENV_PROP) {
                if (ct.type != ConfigValueType::string) {
                    log(ENV_ARG_ERROR);
                }
                _options.environment = std::move(std::get<std::string>(ct.value.value()));
            } else if (_key == EXEC_PROP) {
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
            } else if (_key == PROJECT_PROP) {
                if (ct.type != ConfigValueType::string) {
                    log(PROJECT_ARG_ERROR);
                }
                _options.project = std::move(std::get<std::string>(ct.value.value()));
            } else if (_key == REQUIRED_PROP) {
                if (ct.type != ConfigValueType::array) {
                    log(REQUIRED_ARG_ERROR);
                }
                _options.required_envs = std::move(std::get<std::vector<std::string>>(ct.value.value()));
            } else {
                log(INVALID_PROPERTY_WARNING);
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

    const std::string Config::extract_value_within(char delimiter) noexcept {
        std::string value;
        while (peek().has_value() && peek().value() != delimiter) {
            if (peek().value() == COMMENT) {
                skip_to_eol();
            } else {
                value += commit();
            }
        }
        return value;
    }

    std::string Config::get_value_type_string(const ConfigValueType &cvt) const noexcept {
        switch (cvt) {
        case ConfigValueType::array:
            return "array";
        case ConfigValueType::boolean:
            return "boolean";
        default:
            return "string";
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
                R"(Unable to load the "%s" configuration from the .nvi file (%s). The specified environment doesn't appear to exist!)",
                _env.c_str(), _file_path.c_str());
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
                R"(The "dir" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case ENV_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                ENV_ARG_ERROR,
                R"(The "env" property contains an invalid value. Expected a string value, but instead received: %s.)",
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
                _env.c_str(), _file_path.c_str());
            break;
        }
        case EXEC_ARG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                EXEC_ARG_ERROR,
                R"(The "exec" property contains an invalid value. Expected a string value, but instead received: %s.)",
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
                R"(Successfully parsed the "%s" configuration from the .nvi file and the folowing options were set: debug="true", dir="%s", environment="%s", execute="%s", files="%s", project="%s", required="%s".)",
                _env.c_str(), 
                _options.dir.c_str(), 
                _options.environment.c_str(), 
                _command.c_str(), 
                fmt::join(_options.files, ", ").c_str(), 
                _options.project.c_str(), 
                fmt::join(_options.required_envs, ", ").c_str());
            break;
        }
        default:
            break;
        }
        // clang-format on
    }
} // namespace nvi
