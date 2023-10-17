#include "arg.h"
#include "format.h"
#include "log.h"
#include "options.h"
#include "version.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace nvi {

    inline constexpr char API_FLAG[] = "--api";
    inline constexpr char CONFIG_FLAG[] = "--config";
    inline constexpr char DEBUG_FLAG[] = "--debug";
    inline constexpr char DIRECTORY_FLAG[] = "--directory";
    inline constexpr char ENVIRONMENT_FLAG[] = "--environment";
    inline constexpr char EXECUTE_FLAG[] = "--";
    inline constexpr char FILES_FLAG[] = "--files";
    inline constexpr char HELP_FLAG[] = "--help";
    inline constexpr char PROJECT_FLAG[] = "--project";
    inline constexpr char PRINT_FLAG[] = "--print";
    inline constexpr char REQUIRED_FLAG[] = "--required";
    inline constexpr char SAVE_FLAG[] = "--save";
    inline constexpr char VERSION_FLAG[] = "--version";

    Arg::Arg(int &argc, char *argv[]) : _argc(argc - 1), _argv(argv) {
        while (_index < _argc) {
            const std::string arg{_argv[++_index]};
            if (arg == API_FLAG) {
                _options.api = true;
            } else if (arg == CONFIG_FLAG) {
                _options.config = parse_single_arg(CONFIG_FLAG_ERROR);
            } else if (arg == DEBUG_FLAG) {
                _options.debug = true;
            } else if (arg == DIRECTORY_FLAG) {
                _options.dir = parse_single_arg(DIR_FLAG_ERROR);
            } else if (arg == ENVIRONMENT_FLAG) {
                _options.environment = parse_single_arg(ENV_FLAG_ERROR);
            } else if (arg == EXECUTE_FLAG) {
                parse_command_args();
                break;
            } else if (arg == FILES_FLAG) {
                _options.files = parse_multi_arg(FILES_FLAG_ERROR);
            } else if (arg == HELP_FLAG) {
                log(HELP_DOC);
            } else if (arg == PRINT_FLAG) {
                _options.print = true;
            } else if (arg == PROJECT_FLAG) {
                _options.project = parse_single_arg(PROJECT_FLAG_ERROR);
            } else if (arg == REQUIRED_FLAG) {
                _options.required_envs = parse_multi_arg(REQUIRED_FLAG_ERROR);
            } else if (arg == SAVE_FLAG) {
                _options.save = true;
            } else if (arg == VERSION_FLAG) {
                log(NVI_VERSION);
            } else {
                remove_invalid_flag();
            }
        }

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
            NVI_LOG_AND_EXIT(
                HELP_DOC,
                "\n"
                R"(
┌───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ NVI CLI Documentation                                                                                                 │
├───────────────┬───────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ flag          │ description                                                                                           │
├───────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ --api         │ Specifies whether or not to retrieve ENVs from the remote API. (ex: --api)                            │
│ --config      │ Specifies which environment configuration to load from the .nvi file. (ex: --config dev)              │
│ --debug       │ Specifies whether or not to log debug details. (ex: --debug)                                          │
│ --directory   │ Specifies which directory the .env files are located within. (ex: --directory path/to/envs)           │
│ --environment │ Specifies which environment config to use within a remote project. (ex: --environment dev)            │
│ --files       │ Specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)            │
│ --project     │ Specifies which remote project to select from the nvi API. (ex: --project my_project)                 │
│ --print       │ Specifies whether or not to print ENVs to standard out. (ex: --print)                                 │
│ --required    │ Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)                │
│ --save        │ Specifies whether or not to save remote ENVs to disk with the selected environment name. (ex: --save) │
│ --            │ Specifies which system command to run in a child process with parsed ENVS. (ex: -- cargo run)         │
└───────────────┴───────────────────────────────────────────────────────────────────────────────────────────────────────┘
For additional information, please see the man documentation or README.)"
                "\n",
                NULL);
            break;
        }
        case NVI_VERSION: {
            std::time_t current_time{std::time(nullptr)};
            std::tm const *time_stamp{std::localtime(&current_time)};

            std::clog << "nvi " << NVI_LIB_VERSION << '\n';
            std::clog << "Copyright (C) " << time_stamp->tm_year + 1900 << " Matt Carlotta." << '\n';
            std::clog << "This is free software licensed under the GPL-3.0 license; see the source LICENSE for copying conditions." << '\n';
            std::clog << "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << std::endl;
            std::exit(EXIT_SUCCESS);
            break;
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
