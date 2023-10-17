#include "arg.h"
#include "format.h"
#include "log.h"
#include "options.h"
#include "version.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace nvi {
    Arg::Arg(int &argc, char *argv[]) : _argc(argc - 1), _argv(argv) {
        while (_index < _argc) {
            const std::string_view flag{_argv[++_index]};

            switch (FLAGS.count(flag) ? FLAGS.at(flag) : FLAG::UNKNOWN) {
            case FLAG::API: {
                _options.api = true;
                break;
            }
            case FLAG::CONFIG: {
                _options.config = parse_single_arg(CONFIG_FLAG_ERROR);
                break;
            }
            case FLAG::DEBUG: {
                _options.debug = true;
                break;
            }
            case FLAG::DIRECTORY: {
                _options.dir = parse_single_arg(DIR_FLAG_ERROR);
                break;
            }
            case FLAG::ENVIRONMENT: {
                _options.environment = parse_single_arg(ENV_FLAG_ERROR);
                break;
            }
            case FLAG::EXECUTE: {
                parse_command_args();
                goto exit_flag_parsing;
            }
            case FLAG::FILES: {
                _options.files = parse_multi_arg(FILES_FLAG_ERROR);
                break;
            }
            case FLAG::HELP: {
                log(HELP_DOC);
            }
            case FLAG::PRINT: {
                _options.print = true;
                break;
            }
            case FLAG::PROJECT: {
                _options.project = parse_single_arg(PROJECT_FLAG_ERROR);
                break;
            }
            case FLAG::REQUIRED: {
                _options.required_envs = parse_multi_arg(REQUIRED_FLAG_ERROR);
                break;
            }
            case FLAG::SAVE: {
                _options.save = true;
                break;
            }
            case FLAG::VERSION: {
                log(NVI_VERSION);
            }
            default: {
                remove_invalid_flag();
                break;
            }
            };
        }

    exit_flag_parsing:
        if (_options.debug) {
            log(DEBUG);
        }
    };

    const options_t &Arg::get_options() const noexcept { return _options; }

    std::string Arg::parse_single_arg(const messages_t &code) noexcept {
        ++_index;
        if (_argv[_index] == nullptr) {
            log(code);
        }

        const std::string arg{_argv[_index]};
        if (arg.find("-") != std::string::npos) {
            log(code);
        }

        return arg;
    }

    std::vector<std::string> Arg::parse_multi_arg(const messages_t &code) noexcept {
        std::vector<std::string> arg;
        while (_index < _argc) {
            ++_index;

            if (_argv[_index] == nullptr) {
                break;
            }

            const std::string next_arg{_argv[_index]};
            if (next_arg.find("-") != std::string::npos) {
                --_index;
                break;
            }

            arg.push_back(next_arg);
        }

        if (not arg.size()) {
            log(code);
        }

        return arg;
    }

    void Arg::parse_command_args() noexcept {
        while (_index < _argc) {
            ++_index;

            if (_argv[_index] == nullptr) {
                break;
            }

            std::string next_arg{_argv[_index]};
            if (not _options.commands.size()) {
                _bin_name = next_arg;
            }

            _command += _command.length() > 0 ? " " + next_arg : next_arg;
            _options.commands.push_back(std::move(_argv[_index]));
        }

        if (not _options.commands.size()) {
            log(COMMAND_FLAG_ERROR);
        }

        _options.commands.push_back(nullptr);
    }

    void Arg::remove_invalid_flag() noexcept {
        _invalid_flag = std::string{_argv[_index]};
        _invalid_args.clear();
        while (_index < _argc) {
            ++_index;

            if (_argv[_index] == nullptr) {
                break;
            }

            const std::string arg{_argv[_index]};
            if (arg.find("-") != std::string::npos) {
                --_index;
                break;
            }

            _invalid_args += _invalid_args.length() ? " " + arg : arg;
        }

        log(INVALID_FLAG_WARNING);
    }

    void Arg::log(const messages_t &code) const noexcept {
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
} // namespace nvi
