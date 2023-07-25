#include "arg.h"
#include "format.h"
#include "options.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace nvi {

    enum MESSAGES {
        CONFIG_FLAG_ERROR = 0,
        DIR_FLAG_ERROR = 1,
        COMMAND_FLAG_ERROR = 2,
        FILES_FLAG_ERROR = 3,
        REQUIRED_FLAG_ERROR = 4,
        HELP_DOC = 5,
        INVALID_FLAG_WARNING = 6,
        DEBUG = 7,
    };

    constexpr char CONFIG_SHORT[] = "-c";
    constexpr char CONFIG_LONG[] = "--config";
    constexpr char DEBUG_SHORT[] = "-de";
    constexpr char DEBUG_LONG[] = "--debug";
    constexpr char DIRECTORY_SHORT[] = "-d";
    constexpr char DIRECTORY_LONG[] = "--dir";
    constexpr char EXECUTE_SHORT[] = "-e";
    constexpr char EXECUTE_LONG[] = "--exec";
    constexpr char FILES_SHORT[] = "-f";
    constexpr char FILES_LONG[] = "--files";
    constexpr char HELP_SHORT[] = "-h";
    constexpr char HELP_LONG[] = "--help";
    constexpr char REQUIRED_SHORT[] = "-r";
    constexpr char REQUIRED_LONG[] = "--required";

    Arg_Parser::Arg_Parser(int &argc, char *argv[]) : _argc(argc), _argv(argv) {
        _index = 1;
        while (_index < _argc) {
            const std::string arg = std::string{_argv[_index]};
            if (arg == CONFIG_SHORT || arg == CONFIG_LONG) {
                _options.config = parse_single_arg(MESSAGES::CONFIG_FLAG_ERROR);
            } else if (arg == DEBUG_SHORT || arg == DEBUG_LONG) {
                _options.debug = true;
            } else if (arg == DIRECTORY_SHORT || arg == DIRECTORY_LONG) {
                _options.dir = parse_single_arg(MESSAGES::DIR_FLAG_ERROR);
            } else if (arg == EXECUTE_SHORT || arg == EXECUTE_LONG) {
                parse_command_args();
            } else if (arg == FILES_SHORT || arg == FILES_LONG) {
                _options.files = parse_multi_arg(MESSAGES::FILES_FLAG_ERROR);
            } else if (arg == HELP_SHORT || arg == HELP_LONG) {
                log(MESSAGES::HELP_DOC);
                std::exit(0);
            } else if (arg == REQUIRED_SHORT || arg == REQUIRED_LONG) {
                _options.required_envs = parse_multi_arg(MESSAGES::REQUIRED_FLAG_ERROR);
            } else {
                remove_invalid_arg();
            }

            ++_index;
        }

        if (_options.debug) {
            log(MESSAGES::DEBUG);
        }
    };

    const Options &Arg_Parser::get_options() const noexcept { return _options; }

    std::string Arg_Parser::parse_single_arg(const uint_least8_t &code) {
        ++_index;
        if (_argv[_index] == nullptr) {
            log(code);
            std::exit(1);
        }

        const std::string arg = std::string{_argv[_index]};
        if (arg.find("-") != std::string::npos) {
            log(code);
            std::exit(1);
        }

        return arg;
    }

    std::vector<std::string> Arg_Parser::parse_multi_arg(const uint_least8_t &code) {
        std::vector<std::string> arg;
        ++_index;
        while (_index < _argc) {
            if (_argv[_index] == nullptr) {
                break;
            }

            const std::string next_arg = std::string{_argv[_index]};
            if (next_arg.find("-") != std::string::npos) {
                _index -= 1;
                break;
            }

            arg.push_back(next_arg);
            ++_index;
        }

        if (not arg.size()) {
            log(code);
            std::exit(1);
        }

        return arg;
    }

    void Arg_Parser::parse_command_args() {
        // currently, it's impossible to determine when an "--exec" flag with arguments end and another
        // flag begins, for example: nvi --debug --exec cargo run --release --required KEY1 KEY2
        // where "cargo run --release" needs to be separated from the other known flags. A work-around
        // is to assume the command won't contain any of the RESERVED_FLAGS defined below
        static const std::unordered_set<std::string> RESERVED_FLAGS = {
            CONFIG_SHORT, CONFIG_LONG, DEBUG_SHORT, DEBUG_LONG, DIRECTORY_SHORT, DIRECTORY_LONG, EXECUTE_SHORT,
            EXECUTE_LONG, FILES_SHORT, FILES_LONG,  HELP_SHORT, REQUIRED_SHORT,  REQUIRED_LONG};

        ++_index;
        while (_index < _argc) {
            if (_argv[_index] == nullptr) {
                break;
            }

            std::string next_arg = std::string{_argv[_index]};
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
            log(MESSAGES::COMMAND_FLAG_ERROR);
            std::exit(1);
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

            const std::string arg = std::string{_argv[_index]};
            if (arg.find("-") != std::string::npos) {
                _index -= 1;
                break;
            }

            _invalid_args += _invalid_args.length() ? " " + arg : arg;
        }

        log(MESSAGES::INVALID_FLAG_WARNING);
    }

    void Arg_Parser::log(const uint_least8_t &code) const noexcept {
        switch (code) {
        case MESSAGES::CONFIG_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::CONFIG_FLAG_ERROR) The \"-c\" or \"--config\" flag must contain an "
                         "environment name from the .nvi configuration file. Use flag \"-h\" or \"--help\" for "
                         "more information."
                      << std::endl;
            break;
        }
        case MESSAGES::DIR_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::DIR_FLAG_ERROR) The \"-d\" or \"--dir\" flag must contain a "
                         "valid directory path. Use flag \"-h\" or \"--help\" for more information."
                      << std::endl;
            break;
        }
        case MESSAGES::COMMAND_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::COMMAND_FLAG_ERROR) The \"-e\" or \"--exec\" flag must contain at least "
                         "1 command. Use flag \"-h\" or \"--help\" for more information."
                      << std::endl;
            break;
        }
        case MESSAGES::FILES_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::FILES_FLAG_ERROR) The \"-f\" or \"--files\" flag must contain at least "
                         "1 .env file. Use flag \"-h\" or \"--help\" for more information."
                      << std::endl;
            break;
        }
        case MESSAGES::REQUIRED_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::REQUIRED_FLAG_ERROR) The \"-r\" or \"--required\" flag must contain at "
                         "least 1 ENV key. Use flag \"-h\" or \"--help\" for more information."
                      << std::endl;
            break;
        }
        case MESSAGES::HELP_DOC: {
            // clang-format off
            std::clog << "┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n"
                         "│ NVI CLI Documentation                                                                                                  │\n"
                         "├─────────────────┬──────────────────────────────────────────────────────────────────────────────────────────────────────┤\n"
                         "│ flag            │ description                                                                                          │\n"
                         "├─────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────┤\n"
                         "│ -c, --config    │ Specifies which environment configuration to load from the .nvi file. (ex: --config dev)             │\n"
                         "│ -de, --debug    │ Specifies whether or not to log debug details. (ex: --debug)                                         │\n"
                         "│ -d, --dir       │ Specifies which directory the env file is located within. (ex: --dir path/to/env)                    │\n"
                         "│ -e, --exec      │ Specifies which command to run in a separate process with parsed ENVS. (ex: --exec node index.js)    │\n"
                         "│ -f, --files     │ Specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)           │\n"
                         "│ -r, --required  │ Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)               │\n"
                         "└─────────────────┴──────────────────────────────────────────────────────────────────────────────────────────────────────┘"
                      << std::endl;
            // clang-format on
            break;
        }
        case MESSAGES::INVALID_FLAG_WARNING: {
            std::clog << fmt::format(
                             "[nvi] (arg::INVALID_FLAG_WARNING) The flag \"%s\" with%s arguments is not recognized. "
                             "Skipping.",
                             _invalid_arg.c_str(),
                             (_invalid_args.length() ? " \"" + _invalid_args + "\"" : "out").c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::DEBUG: {
            std::clog << fmt::format("[nvi] (arg::DEBUG) The following options were set: config=\"%s\", "
                                     "debug=\"true\", dir=\"%s\", execute=\"%s\", files=\"%s\", required=\"%s\".",
                                     _options.config.c_str(), _options.dir.c_str(), _command.c_str(),
                                     fmt::join(_options.files, ", ").c_str(),
                                     fmt::join(_options.required_envs, ", ").c_str())
                      << '\n';

            if (_options.config.length() && (_options.dir.length() || _options.commands.size() ||
                                             _options.files.size() > 1 || _options.required_envs.size())) {
                std::clog << "[nvi] (arg::DEBUG) Found conflicting flags. When the \"config\" flag has been set, then "
                             "\"dir\", \"exec\", \"files\", and \"required\" flags are ignored."
                          << '\n';
            }

            std::clog << std::endl;
            break;
        }
        default:
            break;
        }
    }
} // namespace nvi
