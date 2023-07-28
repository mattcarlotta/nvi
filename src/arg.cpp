#include "arg.h"
#include "format.h"
#include "log.h"
#include "options.h"
#include "version.h"
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace nvi {

    inline const constexpr char CONFIG_SHORT[] = "-c";
    inline const constexpr char CONFIG_LONG[] = "--config";
    inline const constexpr char DEBUG_SHORT[] = "-de";
    inline const constexpr char DEBUG_LONG[] = "--debug";
    inline const constexpr char DIRECTORY_SHORT[] = "-d";
    inline const constexpr char DIRECTORY_LONG[] = "--dir";
    inline const constexpr char EXECUTE_SHORT[] = "-e";
    inline const constexpr char EXECUTE_LONG[] = "--exec";
    inline const constexpr char FILES_SHORT[] = "-f";
    inline const constexpr char FILES_LONG[] = "--files";
    inline const constexpr char HELP_SHORT[] = "-h";
    inline const constexpr char HELP_LONG[] = "--help";
    inline const constexpr char REQUIRED_SHORT[] = "-r";
    inline const constexpr char REQUIRED_LONG[] = "--required";
    inline const constexpr char VERSION_SHORT[] = "-v";
    inline const constexpr char VERSION_LONG[] = "--version";

    Arg_Parser::Arg_Parser(int &argc, char *argv[]) : _argc(argc), _argv(argv) {
        _index = 1;
        while (_index < _argc) {
            const std::string arg{_argv[_index]};
            if (arg == CONFIG_SHORT || arg == CONFIG_LONG) {
                _options.config = parse_single_arg(CONFIG_FLAG_ERROR);
            } else if (arg == DEBUG_SHORT || arg == DEBUG_LONG) {
                _options.debug = true;
            } else if (arg == DIRECTORY_SHORT || arg == DIRECTORY_LONG) {
                _options.dir = parse_single_arg(DIR_FLAG_ERROR);
            } else if (arg == EXECUTE_SHORT || arg == EXECUTE_LONG) {
                parse_command_args();
            } else if (arg == FILES_SHORT || arg == FILES_LONG) {
                _options.files = parse_multi_arg(FILES_FLAG_ERROR);
            } else if (arg == HELP_SHORT || arg == HELP_LONG) {
                log(HELP_DOC);
            } else if (arg == REQUIRED_SHORT || arg == REQUIRED_LONG) {
                _options.required_envs = parse_multi_arg(REQUIRED_FLAG_ERROR);
            } else if (arg == VERSION_SHORT || arg == VERSION_LONG) {
                log(NVI_VERSION);
            } else {
                remove_invalid_arg();
            }

            ++_index;
        }

        if (_options.debug) {
            log(DEBUG);
        }
    };

    const options_t &Arg_Parser::get_options() const noexcept { return _options; }

    std::string Arg_Parser::parse_single_arg(const messages_t &code) noexcept {
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

    std::vector<std::string> Arg_Parser::parse_multi_arg(const messages_t &code) noexcept {
        std::vector<std::string> arg;
        ++_index;
        while (_index < _argc) {
            if (_argv[_index] == nullptr) {
                break;
            }

            const std::string next_arg{_argv[_index]};
            if (next_arg.find("-") != std::string::npos) {
                _index -= 1;
                break;
            }

            arg.push_back(next_arg);
            ++_index;
        }

        if (not arg.size()) {
            log(code);
        }

        return arg;
    }

    void Arg_Parser::parse_command_args() noexcept {
        // currently, it's impossible to determine when an "--exec" flag with arguments end and another
        // flag begins, for example: nvi --debug --exec cargo run --release --required KEY1 KEY2
        // where "cargo run --release" needs to be separated from the other known flags. A work-around
        // is to assume the command won't contain any of the RESERVED_FLAGS defined below
        static const std::unordered_set<std::string> RESERVED_FLAGS{
            CONFIG_SHORT, CONFIG_LONG, DEBUG_SHORT, DEBUG_LONG, DIRECTORY_SHORT, DIRECTORY_LONG, EXECUTE_SHORT,
            EXECUTE_LONG, FILES_SHORT, FILES_LONG,  HELP_SHORT, REQUIRED_SHORT,  REQUIRED_LONG};

        ++_index;
        while (_index < _argc) {
            if (_argv[_index] == nullptr) {
                break;
            }

            std::string next_arg{_argv[_index]};
            if (next_arg.find("-") != std::string::npos && RESERVED_FLAGS.find(next_arg) != RESERVED_FLAGS.end()) {
                _index -= 1;
                break;
            } else if (not _options.commands.size()) {
                _bin_name = next_arg;
            }

            char *arg_str = new char[next_arg.size() + 1];
            std::strcpy(arg_str, next_arg.c_str());

            _command += _command.size() > 0 ? " " + next_arg : next_arg;
            _options.commands.push_back(arg_str);
            ++_index;
        }

        if (not _options.commands.size()) {
            log(COMMAND_FLAG_ERROR);
        }

        _options.commands.push_back(nullptr);
    }

    void Arg_Parser::remove_invalid_arg() noexcept {
        _invalid_arg = std::string{_argv[_index]};
        _invalid_args = "";
        while (_index < _argc) {
            ++_index;

            if (_argv[_index] == nullptr) {
                break;
            }

            const std::string arg{_argv[_index]};
            if (arg.find("-") != std::string::npos) {
                _index -= 1;
                break;
            }

            _invalid_args += _invalid_args.length() ? " " + arg : arg;
        }

        log(INVALID_FLAG_WARNING);
    }

    void Arg_Parser::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case CONFIG_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                CONFIG_FLAG_ERROR,
                "The \"-c\" or \"--config\" flag must contain an environment name from the .nvi configuration "
                "file. Use flag \"-h\" or \"--help\" for more information.", 
                NULL);
            break;
        }
        case DIR_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                DIR_FLAG_ERROR,
                "The \"-d\" or \"--dir\" flag must contain a valid directory path. Use flag \"-h\" or "
                "\"--help\" for more information.",
                NULL);
            break;
        }
        case COMMAND_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                COMMAND_FLAG_ERROR,
                "The \"-e\" or \"--exec\" flag must contain at least 1 command. Use flag \"-h\" or \"--help\""
                " for more information.",
                NULL);
            break;
        }
        case FILES_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILES_FLAG_ERROR,
                "The \"-f\" or \"--files\" flag must contain at least 1 .env file. Use flag \"-h\" or "
                "\"--help\" for more information.",
                NULL);
            break;
        }
        case REQUIRED_FLAG_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUIRED_FLAG_ERROR,
                "The \"-r\" or \"--required\" flag must contain at least 1 ENV key. Use flag \"-h\" or "
                "\"--help\" for more information.",
                NULL);
            break;
        }
        case HELP_DOC: {
            NVI_LOG_AND_EXIT(
                HELP_DOC,
                "\n\n"
                "┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n"
                "│ NVI CLI Documentation                                                                                                  │\n"
                "├─────────────────┬──────────────────────────────────────────────────────────────────────────────────────────────────────┤\n"
                "│ flag            │ description                                                                                          │\n"
                "├─────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────┤\n"
                "│ -c, --config    │ Specifies which environment configuration to load from the .nvi file. (ex: --config dev)             │\n"
                "│ -de, --debug    │ Specifies whether or not to log debug details. (ex: --debug)                                         │\n"
                "│ -d, --dir       │ Specifies which directory the .env files are located within. (ex: --dir path/to/envs)                │\n"
                "│ -e, --exec      │ Specifies which command to run in a separate process with parsed ENVS. (ex: --exec node index.js)    │\n"
                "│ -f, --files     │ Specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)           │\n"
                "│ -r, --required  │ Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)               │\n"
                "└─────────────────┴──────────────────────────────────────────────────────────────────────────────────────────────────────┘\n"
                "For additional information, please see the source README.\n",
                NULL);
            break;
        }
        case NVI_VERSION: {
            NVI_LOG_AND_EXIT(
                NVI_VERSION,
                "\n\nnvi %s \n"
                "Copyright (C) 2023 Matt Carlotta.\n"
                "This is free software licensed under the GPL-3.0 license; see the source LICENSE for copying conditions.\n" 
                "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
                LIB_VERSION);
            break;
        }
        case INVALID_FLAG_WARNING: {
            NVI_LOG_DEBUG(
                INVALID_FLAG_WARNING,
                "The flag \"%s\" with%s arguments is not recognized. Skipping.",
                _invalid_arg.c_str(), 
                (_invalid_args.length() ? " \"" + _invalid_args + "\"" : "out").c_str());
            break;
        }
        case DEBUG: {
            NVI_LOG_DEBUG(
                DEBUG,
                "The following options were set: config=\"%s\", debug=\"true\", dir=\"%s\", execute=\"%s\", "
                "files=\"%s\", required=\"%s\".",
                _options.config.c_str(), _options.dir.c_str(), _command.c_str(),
                fmt::join(_options.files, ", ").c_str(),
                fmt::join(_options.required_envs, ", ").c_str());

            if (_options.config.length() && 
                    (_options.dir.length() || 
                     _options.commands.size() ||
                     _options.files.size() > 1 || 
                     _options.required_envs.size())) 
            {
                NVI_LOG_DEBUG(
                    DEBUG,
                    "Found conflicting flags. When the \"config\" flag has been set, then \"dir\", \"exec\", "
                    "\"files\", and \"required\" flags are ignored.",
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
