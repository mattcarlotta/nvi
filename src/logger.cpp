#include "logger.h"
#include "config_token.h"
#include "format.h"
#include "lexer_token.h"
#include "log.h"
#include "version.h"
#include <cstdlib>
#include <ctime>
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>

namespace nvi {
    // generator
    Logger::Logger(logger_t code, const options_t &options) : _code(code), _options(options) {}

    // arg
    Logger::Logger(logger_t code, const options_t &options, const std::string &command, const std::string &invalid_args,
                   const std::string &invalid_flag)
        : _code(code), _options(options), _command(command), _invalid_args(invalid_args), _invalid_flag(invalid_flag) {}

    // config
    Logger::Logger(logger_t code, const options_t &options, const std::string &command,
                   const std::vector<ConfigToken> &config_tokens, const std::filesystem::path &file_path,
                   const std::string &key, const std::string &value_type)
        : _code(code), _options(options), _command(command), _config_tokens(config_tokens), _file_path(file_path),
          _key(key), _value_type(value_type) {}

    // api
    Logger::Logger(logger_t code, const options_t &options, const CURLcode &res, const std::string &res_data,
                   const std::filesystem::path &env_file_path, const unsigned int &res_status_code)
        : _code(code), _options(options), _res(res), _res_data(res_data), _env_file_path(env_file_path),
          _res_status_code(res_status_code) {}

    // parser
    Logger::Logger(logger_t code, const options_t &options, const std::string &key, const Token &token,
                   const ValueToken &value_token, const std::string &interp_key, const std::string &value)
        : _code(code), _options(options), _key(key), _token(token), _value_token(value_token), _interp_key(interp_key),
          _value(value) {}

    // lexer
    Logger::Logger(logger_t code, const options_t &options, const std::filesystem::path &file_path,
                   const std::vector<Token> &tokens, const size_t &line, const size_t &byte,
                   const std::string &file_name, const std::string &token_key)
        : _code(code), _options(options), _file_path(file_path), _tokens(tokens), _byte(byte), _line(line),
          _file_name(file_name), _token_key(token_key) {}

    void Logger::log_debug(const messages_t &message_code, const std::string &message) const noexcept {
        std::clog << "[nvi] (Logger::" << _get_logger_from_code(_code) << "::" << _get_string_from_code(message_code)
                  << ") " << message << '\n';
    }

    void Logger::log_error(const messages_t &message_code, const std::string &message) const noexcept {
        std::cerr << "[nvi] (Logger::" << _get_logger_from_code(_code) << "::" << _get_string_from_code(message_code)
                  << ") " << message << std::endl;
        std::exit(EXIT_FAILURE);
    }

    void Logger::debug(const messages_t &message_code) const noexcept {
        std::string message;

        switch (message_code) {
        // ARG
        case INVALID_FLAG_WARNING: {
            message = fmt::format(R"(The flag "%s"%s is not recognized. Skipping.)", _invalid_flag.c_str(),
                                  (_invalid_args.length() ? " with \"" + _invalid_args + "\" arguments" : "").c_str());
            break;
        }
        case DEBUG_ARG: {
            if (_options.commands.size() && _options.print) {
                log_debug(message_code,
                          R"(Found conflicting flags: When commands are present, then the "print" flag is ignored.)");
            }

            if (_options.config.length() && (_options.dir.length() || _options.commands.size() ||
                                             _options.files.size() > 1 || _options.required_envs.size())) {
                log_debug(
                    message_code,
                    R"(Found conflicting flags: When the "config" flag has been set, then other flags are ignored.)");
            }

            message = fmt::format(
                R"(The following arg options were set: api="%s", config="%s", debug="true", directory="%s", environment="%s", execute="%s", files="%s", print="%s", project="%s", required="%s", save="%s".)",
                (_options.api ? "true" : "false"), _options.config.c_str(), _options.dir.c_str(),
                _options.environment.c_str(), _command.c_str(), fmt::join(_options.files, ", ").c_str(),
                (_options.print ? "true" : "false"), _options.project.c_str(),
                fmt::join(_options.required_envs, ", ").c_str(), (_options.save ? "true" : "false"));
            break;
        }
        // CONFIG
        case INVALID_PROPERTY_WARNING: {
            message = fmt::format(R"(Found an invalid property: "%s" within the "%s" config. Skipping.)", _key.c_str(),
                                  _options.config.c_str());
            break;
        }
        case DEBUG_CONFIG_TOKENS: {
            for (size_t index = 0; index < _config_tokens.size(); ++index) {
                const ConfigToken &token = _config_tokens.at(index);

                std::stringstream ss;
                ss << "Created " << get_value_type_string(token.type) << " token with a key of "
                   << std::quoted(token.key);
                ss << " and a value of " << std::quoted(std::visit(ConfigTokenToString(), token.value.value())) << ".";
                if (index == _config_tokens.size()) {
                    ss << '\n';
                }

                log_debug(message_code, ss.str());
            }
            break;
        }
        case CONFLICTING_COMMAND_FLAG: {
            message = R"(Found conflicting flags. When commands are present, then the "print" flag is ignored.)";
            break;
        }
        case CONFLICTING_CONFIG_FLAG: {
            message = R"(Found conflicting flags. When the "config" flag has been set, then other flags are ignored.)";
            break;
        }
        case DEBUG_CONFIG: {
            log_debug(message_code, fmt::format(R"(Successfully parsed the "%s" configuration from the .nvi file.)",
                                                _options.config.c_str()));

            message = fmt::format(
                R"(The following config options were set: api="%s", debug="true", directory="%s", environment="%s", execute="%s", files="%s", print="%s", project="%s", required="%s", save="%s".)",
                (_options.api ? "true" : "false"), _options.dir.c_str(), _options.environment.c_str(), _command.c_str(),
                fmt::join(_options.files, ", ").c_str(), (_options.print ? "true" : "false"), _options.project.c_str(),
                fmt::join(_options.required_envs, ", ").c_str(), (_options.save ? "true" : "false"));
            break;
        }
        // API
        case RESPONSE_SUCCESS: {
            message = fmt::format("Successfully retrieved the %s ENVs from the nvi API.", _options.environment.c_str());
            break;
        }
        case SAVED_ENV_FILE: {
            message = fmt::format(R"(Successfully saved the "%s.env" file to disk (%s).)", _options.environment.c_str(),
                                  _env_file_path.c_str());
            break;
        }
        // PARSER
        case INTERPOLATION_WARNING: {
            message = fmt::format(
                R"([%s:%d:%d] The key "%s" contains an invalid interpolated variable: "%s". Unable to locate a value that corresponds to this key.)",
                _token.file.c_str(), _value_token.line, _value_token.byte, _token.key->c_str(), _interp_key.c_str());
            break;
        }
        case DEBUG_PARSER: {
            message = fmt::format(R"([%s:%d:%d] Set key "%s" to equal value "%s".)", _token.file.c_str(),
                                  _value_token.line, _value_token.byte, _key.c_str(), _value.c_str());
            break;
        }
        case DEBUG_RESPONSE_PROCESSED: {
            message = fmt::format(R"(Successfully parsed the remote "%s" project's "%s" environment ENVs!)",
                                  _options.project.c_str(), _options.environment.c_str());
            break;
        }
        case DEBUG_FILE_PROCESSED: {
            message = fmt::format("Successfully parsed the %s file%s!\n", fmt::join(_options.files, ", ").c_str(),
                                  (_options.files.size() > 1 ? "s" : ""));
            break;
        }
        // LEXER
        case DEBUG_LEXER: {
            for (const Token &token : _tokens) {
                std::stringstream ss;
                ss << "Created a token key " << std::quoted(token.key.value());
                ss << " with the following tokenized values(" << token.values.size() << "): \n";
                for (size_t index = 0; index < token.values.size(); ++index) {
                    const ValueToken &vt = token.values.at(index);
                    ss << std::setw(4) << index + 1 << ": [" << token.file << "::" << vt.line << "::" << vt.byte
                       << "] ";
                    ss << "A token value of " << std::quoted((vt.value.has_value() ? vt.value.value() : ""));
                    ss << " has been created as " << get_value_type_string(vt.type) << ".";
                    if (index + 1 != token.values.size()) {
                        ss << '\n';
                    }
                }
                log_debug(message_code, ss.str());
            }
            break;
        }
        default:
            break;
        }

        if (message.length()) {
            log_debug(message_code, message);
        }
    }

    void Logger::fatal(const messages_t &message_code) const noexcept {
        std::string message;

        switch (message_code) {
        // ARG
        case CONFIG_FLAG_ERROR: {
            message =
                R"(The "--config" flag must contain an environment name from the .nvi configuration file. Use flag "--help" for more information.)";
            break;
        }
        case DIR_FLAG_ERROR: {
            message =
                R"(The "--directory" flag must contain a valid directory path. Use flag "--help" for more information.)";
            break;
        }
        case COMMAND_FLAG_ERROR: {
            message =
                R"(The "--" (execute) flag must contain at least 1 system command. Use flag "--help" for more information.)";
            break;
        }
        case ENV_FLAG_ERROR: {
            message =
                R"(The "--environment" flag must contain a valid environment name. Use flag "--help" for more information.)";
            break;
        }
        case FILES_FLAG_ERROR: {
            message =
                R"(The "--files" flag must contain at least 1 .env file. Use flag "--help" for more information.)";
            break;
        }
        case PROJECT_FLAG_ERROR: {
            message =
                R"(The "--project" flag must contain a valid project name. Use flag "--help" for more information.)";
            break;
        }
        case REQUIRED_FLAG_ERROR: {
            message =
                R"(The "--required" flag must contain at least 1 ENV key. Use flag "--help" for more information.)";
            break;
        }
        case HELP_DOC: {
            message = R"(
┌───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ nvi cli documentation                                                                                                 │
├───────────────┬───────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ flag          │ description                                                                                           │
├───────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ --api         │ specifies whether or not to retrieve ENVs from the remote API. (ex: --api)                            │
│ --config      │ specifies which environment configuration to load from the .nvi file. (ex: --config dev)              │
│ --debug       │ specifies whether or not to log debug details. (ex: --debug)                                          │
│ --directory   │ specifies which directory the .env files are located within. (ex: --directory path/to/envs)           │
│ --environment │ specifies which environment config to use within a remote project. (ex: --environment dev)            │
│ --files       │ specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)            │
│ --project     │ specifies which remote project to select from the nvi API. (ex: --project my_project)                 │
│ --print       │ specifies whether or not to print ENVs to standard out. (ex: --print)                                 │
│ --required    │ specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)                │
│ --save        │ specifies whether or not to save remote ENVs to disk with the selected environment name. (ex: --save) │
│ --            │ specifies which system command to run in a child process with parsed ENVs. (ex: -- cargo run)         │
└───────────────┴───────────────────────────────────────────────────────────────────────────────────────────────────────┘

for more detailed information, please see the man documentation or the README.)";
            break;
        }
        case NVI_VERSION: {
            std::time_t current_time{std::time(nullptr)};
            std::tm const *time_stamp{std::localtime(&current_time)};
            message =
                fmt::format("nvi %s"
                            "Copyright (C) %d Matt Carlotta."
                            "This is free software licensed under the GPL-3.0 license; see the source LICENSE for "
                            "copying conditions."
                            "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",
                            NVI_VERSION, time_stamp->tm_year + 1900);

            break;
        }
        // CONFIG
        case FILE_ENOENT_ERROR: {
            message = fmt::format(
                R"(Unable to locate "%s". The .nvi configuration file doesn't appear to exist at this path!)",
                _file_path.c_str());
            break;
        }
        case CONFIG_FILE_ERROR: {
            message = fmt::format(
                R"(Unable to open "%s". The .nvi configuration file is either invalid, has restricted access, or may be corrupted.)",
                _file_path.c_str());
            break;
        }
        case FILE_PARSE_ERROR: {
            message = fmt::format(
                R"(Unable to load the "%s" configuration from the .nvi file (%s). The specified config doesn't appear to exist!)",
                _options.config.c_str(), _file_path.c_str());
            break;
        }
        case SELECTED_CONFIG_EMPTY_ERROR: {
            message = fmt::format(
                R"(While the "%s" configuration exists within the .nvi file (%s), the configuration appears to be empty.)",
                _options.config.c_str(), _file_path.c_str());
            break;
        }
        case INVALID_ARRAY_VALUE: {
            message = fmt::format(
                R"(The "%s" property within the "%s" config is not a valid array. It appears to be missing a closing bracket "]".)",
                _key.c_str(), _options.config.c_str());
            break;
        }
        case INVALID_BOOLEAN_VALUE: {
            message = fmt::format(
                R"(The "%s" property within the "%s" config contains an invalid boolean value. Expected the value to match true or false.)",
                _key.c_str(), _options.config.c_str());
            break;
        }
        case INVALID_STRING_VALUE: {
            message = fmt::format(
                R"(The "%s" property within the "%s" config contains an invalid string value. It appears to be empty or is missing a closing double quote.)",
                _key.c_str(), _options.config.c_str());
            break;
        }
        case API_ARG_ERROR: {
            message = fmt::format(
                R"(The "api" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case DEBUG_ARG_ERROR: {
            message = fmt::format(
                R"(The "debug" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case DIR_ARG_ERROR: {
            message = fmt::format(
                R"(The "directory" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case ENV_ARG_ERROR: {
            message = fmt::format(
                R"(The "environment" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case FILES_ARG_ERROR: {
            message = fmt::format(
                R"(The "files" property contains an invalid value. Expected an array of strings, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case EMPTY_FILES_ARG_ERROR: {
            message = fmt::format(
                R"(The "files" property within the "%s" environment configuration (%s) appears to be empty. You must specify at least 1 .env file to load!)",
                _options.config.c_str(), _file_path.c_str());
            break;
        }
        case EXEC_ARG_ERROR: {
            message = fmt::format(
                R"(The "execute" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case PRINT_ARG_ERROR: {
            message = fmt::format(
                R"(The "print" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case PROJECT_ARG_ERROR: {
            message = fmt::format(
                R"(The "project" property contains an invalid value. Expected a string value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case REQUIRED_ARG_ERROR: {
            message = fmt::format(
                R"(The "required" property contains an invalid value. Expected an array of strings, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        case SAVE_ARG_ERROR: {
            message = fmt::format(
                R"(The "save" property contains an invalid value. Expected a boolean value, but instead received: %s.)",
                _value_type.c_str());
            break;
        }
        // GENERATOR
        case COMMAND_ENOENT_ERROR: {
            message = fmt::format(
                R"(The specified command encountered an error. The command "%s" doesn't appear to exist or may not reside in a directory within the shell PATH.)",
                _options.commands[0]);
            break;
        }
        case COMMAND_FAILED_TO_RUN: {
            message = "Unable to run the specified command. See terminal logs for more information.";
            break;
        }
        case NO_ACTION_ERROR: {
            message =
                R"(Running the CLI tool without any system commands nor a "print" or "save" flag won't do anything. Use flag "-h" or "--help" for more information.)";
            break;
        }
        // API
        case INVALID_INPUT_KEY: {
            message =
                "The supplied input is not a valid API key. Please enter a valid API key with aA,zZ,0-9 characters.";
            break;
        }
        case INVALID_INPUT_SELECTION: {
            message = "The supplied number input was not a valid selection. Please try again.";
            break;
        }
        case REQUEST_ERROR: {
            message = fmt::format("The cURL command failed: %s.", curl_easy_strerror(_res));
            break;
        }
        case RESPONSE_ERROR: {
            message = fmt::format("The nvi API responded with a %d: %s.", _res_status_code, _res_data.c_str());
            break;
        }
        case CURL_FAILED_TO_INIT: {
            message = "Failed to initialize cURL. Are you sure it's installed?";
            break;
        }
        case INVALID_ENV_FILE: {
            message = fmt::format(
                R"(Unable to open "%s". The .env file is either invalid, has restricted access, or may be corrupted.)",
                _env_file_path.c_str());
            break;
        }
        // PARSER
        case EMPTY_ENVS_ERROR: {
            message = R"(Unable to parse any ENVs! Please ensure the provided .env files are not empty.)";
            break;
        }
        case REQUIRED_ENV_ERROR: {
            message = fmt::format(
                R"(The following ENV keys were marked as required: "%s", but they were undefined after the list of .env files were parsed.)",
                fmt::join(_options.required_envs, ", ").c_str());
            break;
        }
        // LEXER
        case INTERPOLATION_ERROR: {
            message = fmt::format(
                R"([%s:%d:%d] The key "%s" contains an interpolated "{" operator, but appears to be missing a closing "}" operator.)",
                _file_name.c_str(), _line, _byte, _token_key.c_str());
            break;
        }
        case LEXER_FILE_ENOENT_ERROR: {
            message = fmt::format(R"(Unable to locate "%s". The .env file doesn't appear to exist at this path!)",
                                  _file_path.c_str());
            break;
        }
        case LEXER_FILE_ERROR: {
            message = fmt::format(
                R"(Unable to open "%s". The .env file is either invalid, has restricted access, or may be corrupted.)",
                _file_path.c_str());
            break;
        }
        case FILE_EXTENSION_ERROR: {
            message = fmt::format(R"(The "%s" file is not a valid ".env" file extension.)", _file_name.c_str());
            break;
        }
        case EMPTY_RESPONSE_ENVS_ERROR: {
            message = fmt::format(
                R"(Unable to parse any ENVs! Please ensure the "%s" project has a(n) "%s" environment with at least 1 ENV.)",
                _file_path.c_str(), _file_name.c_str());
            break;
        }
        default:
            message = "Unknown error occured. This is probably a mistake.";
            break;
        }

        log_error(message_code, message);
    }
} // namespace nvi
