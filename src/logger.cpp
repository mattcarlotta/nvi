#include "logger.h"
#include "config_token.h"
#include "format.h"
#include "log.h"
#include "version.h"

namespace nvi {
    Logger::Logger(const options_t &options) : _options(options) {}

    Logger::Logger(const options_t &options, const std::string &command, const std::string &invalid_args,
                   const std::string &invalid_flag)
        : _options(options), _command(command), _invalid_args(invalid_args), _invalid_flag(invalid_flag) {}

    Logger::Logger(const options_t &options, const std::string &command, const std::vector<ConfigToken> &config_tokens,
                   const std::filesystem::path &file_path, const std::string &key, const std::string &value_type)
        : _options(options), _command(command), _config_tokens(config_tokens), _file_path(file_path), _key(key),
          _value_type(value_type) {}

    void Logger::Arg(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case CONFIG_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                CONFIG_FLAG_ERROR,
                R"(The "--config" flag must contain an environment name from the .nvi configuration file. Use flag "--help" for more information.)", 
                NULL);
            break;
        }
        case DIR_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                DIR_FLAG_ERROR,
                R"(The "--directory" flag must contain a valid directory path. Use flag "--help" for more information.)",
                NULL);
            break;
        }
        case COMMAND_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                COMMAND_FLAG_ERROR,
                R"(The "--" (execute) flag must contain at least 1 system command. Use flag "--help" for more information.)",
                NULL);
            break;
        }
        case ENV_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                ENV_FLAG_ERROR,
                R"(The "--environment" flag must contain a valid environment name. Use flag "--help" for more information.)",
                NULL);
            break;
        }
        case FILES_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILES_FLAG_ERROR,
                R"(The "--files" flag must contain at least 1 .env file. Use flag "--help" for more information.)",
                NULL);
            break;
        }
        case PROJECT_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                PROJECT_FLAG_ERROR,
                R"(The "--project" flag must contain a valid project name. Use flag "--help" for more information.)",
                NULL);
            break;
        }
        case REQUIRED_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUIRED_FLAG_ERROR,
                R"(The "--required" flag must contain at least 1 ENV key. Use flag "--help" for more information.)",
                NULL);
            break;
        }
        case HELP_DOC: {
            const std::string help_doc = R"(
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
            std::clog << help_doc << std::endl;
            std::exit(EXIT_SUCCESS);
        }
        case NVI_VERSION: {
            std::time_t current_time{std::time(nullptr)};
            std::tm const *time_stamp{std::localtime(&current_time)};

            std::clog << "nvi " << NVI_LIB_VERSION << '\n';
            std::clog << "Copyright (C) " << time_stamp->tm_year + 1900 << " Matt Carlotta." << '\n';
            std::clog << "This is free software licensed under the GPL-3.0 license; see the source LICENSE for copying conditions." << '\n';
            std::clog << "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << std::endl;
            std::exit(EXIT_SUCCESS);
        }
        case INVALID_FLAG_WARNING: {
            NVI_LOG_DEBUG(
                INVALID_FLAG_WARNING,
                R"(The flag "%s"%s is not recognized. Skipping.)",
                _invalid_flag.c_str(), 
                (_invalid_args.length() ? " with \"" + _invalid_args + "\" arguments" : "").c_str());
            break;
        }
        case DEBUG: {
            NVI_LOG_DEBUG(
                DEBUG,
                R"(The following arg options were set: api="%s", config="%s", debug="true", directory="%s", environment="%s", execute="%s", files="%s", print="%s", project="%s", required="%s", save="%s".)",
                (_options.api ? "true": "false"),
                _options.config.c_str(), 
                _options.dir.c_str(), 
                _options.environment.c_str(), 
                _command.c_str(),
                fmt::join(_options.files, ", ").c_str(),
                (_options.print ? "true": "false"),
                _options.project.c_str(), 
                fmt::join(_options.required_envs, ", ").c_str(),
                (_options.save ? "true": "false"));


            if (_options.commands.size() && _options.print) {
                NVI_LOG_DEBUG(
                    DEBUG,
                    R"(Found conflicting flags. When commands are present, then the "print" flag is ignored.)",
                    NULL);
            }

            if (_options.config.length() && 
                    (_options.dir.length() || 
                     _options.commands.size() ||
                     _options.files.size() > 1 || 
                     _options.required_envs.size())) {
                NVI_LOG_DEBUG(
                    DEBUG,
                    R"(Found conflicting flags. When the "config" flag has been set, then other flags are ignored.)",
                    NULL);
            }
            break;
        }
        default:
            break;
        }
        // clang-format on
    }

    void Logger::Config(const messages_t &code) const noexcept {
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

    void Logger::Generator(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
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
        case NO_ACTION_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                NO_ACTION_ERROR,
                R"(Running the CLI tool without any system commands nor a "print" or "save" flag won't do anything. Use flag "-h" or "--help" for more information.)",
                NULL);
            break;
        }
        default:
            break;
        }
        // clang-format on
    }
} // namespace nvi
