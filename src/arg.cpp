#include "arg.h"
#include "constants.h"
#include "format.h"
#include "options.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace nvi {
    arg_parser::arg_parser(int &argc, char *argv[]) : argc_(argc), argv_(argv) {
        index_ = 1;
        while (index_ < argc_) {
            const std::string arg = std::string(argv_[index_]);
            if (arg == constants::ARG_CONFIG_SHORT || arg == constants::ARG_CONFIG_LONG) {
                options_.config = parse_single_arg(constants::ARG_CONFIG_FLAG_ERROR);
            } else if (arg == constants::ARG_DEBUG_SHORT || arg == constants::ARG_DEBUG_LONG) {
                options_.debug = true;
            } else if (arg == constants::ARG_DIRECTORY_SHORT || arg == constants::ARG_DIRECTORY_LONG) {
                options_.dir = parse_single_arg(constants::ARG_DIR_FLAG_ERROR);
            } else if (arg == constants::ARG_EXECUTE_SHORT || arg == constants::ARG_EXECUTE_LONG) {
                parse_command_args();
            } else if (arg == constants::ARG_FILES_SHORT || arg == constants::ARG_FILES_LONG) {
                options_.files = parse_multi_arg(constants::ARG_FILES_FLAG_ERROR);
            } else if (arg == constants::ARG_HELP_SHORT || arg == constants::ARG_HELP_LONG) {
                log(constants::ARG_HELP_DOC);
                std::exit(0);
            } else if (arg == constants::ARG_REQUIRED_SHORT || arg == constants::ARG_REQUIRED_LONG) {
                options_.required_envs = parse_multi_arg(constants::ARG_REQUIRED_FLAG_ERROR);
            } else {
                invalid_arg_ = std::string(argv_[index_]);
                invalid_args_ = "";
                while (index_ < argc_) {
                    ++index_;

                    if (argv_[index_] == nullptr) {
                        break;
                    }

                    const std::string arg = std::string(argv_[index_]);
                    if (arg.find("-") != std::string::npos) {
                        index_ -= 1;
                        break;
                    }

                    invalid_args_ += invalid_args_.length() ? " " + arg : arg;
                }

                log(constants::ARG_INVALID_FLAG_WARNING);
            }

            ++index_;
        }

        if (options_.debug) {
            log(constants::ARG_DEBUG);
        }
    };

    std::string arg_parser::parse_single_arg(unsigned int code) {
        ++index_;
        if (argv_[index_] == nullptr) {
            log(code);
            std::exit(1);
        }

        const std::string arg = std::string(argv_[index_]);
        if (arg.find("-") != std::string::npos) {
            log(code);
            std::exit(1);
        }

        return arg;
    }

    std::vector<std::string> arg_parser::parse_multi_arg(unsigned int code) {
        std::vector<std::string> arg;
        ++index_;
        while (index_ < argc_) {
            if (argv_[index_] == nullptr) {
                break;
            }

            const std::string next_arg = std::string(argv_[index_]);
            if (next_arg.find("-") != std::string::npos) {
                index_ -= 1;
                break;
            }

            arg.push_back(next_arg);
            ++index_;
        }

        if (!arg.size()) {
            log(code);
            std::exit(1);
        }

        return arg;
    }

    void arg_parser::parse_command_args() {
        // currently, it's impossible to determine when an "--exec" flag with arguments end and another
        // flag begins, for example: nvi --debug --exec cargo run --release --required KEY1 KEY2
        // where "cargo run --release" needs to be separated from the other known flags. A work-around
        // is to assume the command won't contain any of the used ARG_FLAGS defined below
        static std::unordered_set<std::string> RESERVED_FLAGS = {
            constants::ARG_CONFIG_SHORT,  constants::ARG_CONFIG_LONG,     constants::ARG_DEBUG_SHORT,
            constants::ARG_DEBUG_LONG,    constants::ARG_DIRECTORY_SHORT, constants::ARG_DIRECTORY_LONG,
            constants::ARG_EXECUTE_SHORT, constants::ARG_EXECUTE_LONG,    constants::ARG_FILES_SHORT,
            constants::ARG_FILES_LONG,    constants::ARG_HELP_SHORT,      constants::ARG_REQUIRED_SHORT,
            constants::ARG_REQUIRED_LONG};

        ++index_;
        while (index_ < argc_) {
            if (argv_[index_] == nullptr) {
                break;
            }

            std::string next_arg = std::string(argv_[index_]);
            if (next_arg.find("-") != std::string::npos && RESERVED_FLAGS.find(next_arg) != RESERVED_FLAGS.end()) {
                index_ -= 1;
                break;
            } else if (!options_.commands.size()) {
                bin_name_ = next_arg;
            }

            char *arg_str = new char[next_arg.size() + 1];
            std::strcpy(arg_str, next_arg.c_str());

            command_ += command_.size() > 0 ? " " + next_arg : next_arg;
            options_.commands.push_back(arg_str);
            ++index_;
        }

        if (!options_.commands.size()) {
            log(constants::ARG_COMMAND_FLAG_ERROR);
            std::exit(1);
        }

        options_.commands.push_back(nullptr);
    }

    const options &arg_parser::get_options() const noexcept { return options_; }

    void arg_parser::log(uint8_t code) const noexcept {
        switch (code) {
        case constants::ARG_CONFIG_FLAG_ERROR: {
            std::cerr
                << "[nvi] (arg::CONFIG_FLAG_ERROR) The \"-c\" or \"--config\" flag must contain an "
                   "environment name from the env.config.json configuration file. Use flag \"-h\" or \"--help\" for "
                   "more infmt::formation."
                << std::endl;
            break;
        }
        case constants::ARG_DIR_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::DIR_FLAG_ERROR) The \"-d\" or \"--dir\" flag must contain a "
                         "valid directory path. Use flag \"-h\" or \"--help\" for more infmt::formation."
                      << std::endl;
            break;
        }
        case constants::ARG_COMMAND_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::COMMAND_FLAG_ERROR) The \"-e\" or \"--exec\" flag must contain at least "
                         "1 command. Use flag \"-h\" or \"--help\" for more infmt::formation."
                      << std::endl;
            break;
        }
        case constants::ARG_FILES_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::FILES_FLAG_ERROR) The \"-f\" or \"--files\" flag must contain at least "
                         "1 .env file. Use flag \"-h\" or \"--help\" for more infmt::formation."
                      << std::endl;
            break;
        }
        case constants::ARG_REQUIRED_FLAG_ERROR: {
            std::cerr << "[nvi] (arg::REQUIRED_FLAG_ERROR) The \"-r\" or \"--required\" flag must contain at "
                         "least 1 ENV key. Use flag \"-h\" or \"--help\" for more infmt::formation."
                      << std::endl;
            break;
        }
        case constants::ARG_HELP_DOC: {
            // clang-fmt::format off
            std::clog << "┌────────────────────────────────────────────────────────────────────────────────────────────"
                         "────────────────────────────┐\n"
                         "│ NVI CLI Documentation                                                                      "
                         "                            │\n"
                         "├─────────────────┬──────────────────────────────────────────────────────────────────────────"
                         "────────────────────────────┤\n"
                         "│ flag            │ description                                                              "
                         "                            │\n"
                         "├─────────────────┼──────────────────────────────────────────────────────────────────────────"
                         "────────────────────────────┤\n"
                         "│ -c, --config    │ Specifies which environment configuration to load from the "
                         "env.config.json file. (ex: --config dev)  │\n"
                         "│ -de, --debug    │ Specifies whether or not to log debug details. (ex: --debug)             "
                         "                            │\n"
                         "│ -d, --dir       │ Specifies which directory the env file is located within. (ex: --dir "
                         "path/to/env)                    │\n"
                         "│ -e, --exec      │ Specifies which command to run in a separate process with parsed ENVS. "
                         "(ex: --exec node index.js)    │\n"
                         "│ -f, --files     │ Specifies which .env files to parse separated by a space. (ex: --files "
                         "test.env test2.env)           │\n"
                         "│ -r, --required  │ Specifies which ENV keys are required separated by a space. (ex: "
                         "--required KEY1 KEY2)               │\n"
                         "└─────────────────┴──────────────────────────────────────────────────────────────────────────"
                         "────────────────────────────┘"
                      << std::endl;
            // clang-fmt::format on
            break;
        }
        case constants::ARG_INVALID_FLAG_WARNING: {
            std::clog << fmt::format(
                             "[nvi] (arg::INVALID_FLAG_WARNING) The flag \"%s\" with%s arguments is not recognized. "
                             "Skipping.",
                             invalid_arg_.c_str(),
                             (invalid_args_.length() ? " \"" + invalid_args_ + "\"" : "out").c_str())
                      << std::endl;
            break;
        }
        case constants::ARG_DEBUG: {
            std::clog << fmt::format("[nvi] (arg::DEBUG) The following flags were set: config=\"%s\", "
                                     "debug=\"true\", dir=\"%s\", execute=\"%s\", files=\"%s\", required=\"%s\".",
                                     options_.config.c_str(), options_.dir.c_str(), command_.c_str(),
                                     fmt::join(options_.files, ", ").c_str(),
                                     fmt::join(options_.required_envs, ", ").c_str())
                      << std::endl;

            if (options_.config.length() && (options_.dir.length() || options_.commands.size() ||
                                             options_.files.size() > 1 || options_.required_envs.size())) {
                std::clog << "[nvi] (arg::DEBUG) Found conflicting flags. When the \"config\" flag has been set, then "
                             "\"dir\", \"exec\", \"files\", and \"required\" flags are ignored."
                          << std::endl;
            }
            std::clog << std::endl;
            break;
        }
        default:
            break;
        }
    }
} // namespace nvi
