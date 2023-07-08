#include "arg.h"
#include "constants.h"
#include <exception>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using std::string;

namespace nvi {
arg_parser::arg_parser(int &argc, char *argv[]) {
    for (unsigned int i = 1; i < argc; ++i) {
        try {
            const string arg = string(argv[i]);
            if (arg == "-c" || arg == "--config") {
                ++i;
                if (argv[i] == nullptr) {
                    this->log(constants::ARG_CONFIG_FLAG_ERROR);
                    std::exit(1);
                }

                const string next_arg = string(argv[i]);
                if (next_arg.find("-") != string::npos) {
                    this->log(constants::ARG_CONFIG_FLAG_ERROR);
                    std::exit(1);
                }

                this->config = next_arg;
            } else if (arg == "-de" || arg == "--debug") {
                this->debug = true;
            } else if (arg == "-d" || arg == "--dir") {
                i++;
                if (argv[i] == nullptr) {
                    this->log(constants::ARG_DIR_FLAG_ERROR);
                    std::exit(1);
                }

                const string next_arg = string(argv[i]);
                if (next_arg.find("-") != string::npos) {
                    this->log(constants::ARG_DIR_FLAG_ERROR);
                    std::exit(1);
                }

                this->dir = next_arg;
            } else if (arg == "-f" || arg == "--files") {
                ++i;
                std::vector<string> files;
                while (i < argc) {
                    if (argv[i] == nullptr) {
                        break;
                    }

                    const string next_arg = string(argv[i]);
                    if (next_arg.find("-") != string::npos) {
                        i -= 1;
                        break;
                    }

                    files.push_back(next_arg);
                    ++i;
                }

                if (!files.size()) {
                    this->log(constants::ARG_FILES_FLAG_ERROR);
                    std::exit(1);
                }

                this->files = files;
            } else if (arg == "-h" || arg == "--help") {
                this->log(constants::ARG_HELP_DOC);
                std::exit(0);
            } else if (arg == "-r" || arg == "--required") {
                ++i;
                while (i < argc) {
                    if (argv[i] == nullptr) {
                        break;
                    }

                    const string next_arg = string(argv[i]);
                    if (next_arg.find("-") != string::npos) {
                        i -= 1;
                        break;
                    }

                    this->required_envs.push_back(next_arg);
                    ++i;
                }

                if (!this->required_envs.size()) {
                    this->log(constants::ARG_REQUIRED_FLAG_ERROR);
                    std::exit(1);
                }
            } else {
                this->invalid_arg = string(argv[i]);
                while (i < argc) {
                    ++i;

                    if (argv[i] == nullptr) {
                        break;
                    }

                    const string next_arg = string(argv[i]);
                    if (next_arg.find("-") != string::npos) {
                        i -= 1;
                        break;
                    }

                    this->invalid_args += this->invalid_args.length() ? " " + next_arg : next_arg;
                }

                this->log(constants::ARG_INVALID_ARG_WARNING);
            }
        } catch (std::exception &e) {
            this->log(constants::ARG_EXCEPTION);
            std::cerr << e.what() << std::endl;
            std::exit(1);
        }
    }

    if (this->debug) {
        this->log(constants::ARG_DEBUG);
    }
};

void arg_parser::log(unsigned int code) const {
    switch (code) {
    case constants::ARG_CONFIG_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::CONFIG_FLAG_ERROR) The '-c' or '--config' flag must contain an "
                     "environment name from the env.config.json configuration file.";
        std::cerr << " Use flag '-h' or '--help' for more information." << std::endl;
        break;
    }
    case constants::ARG_DIR_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::DIR_FLAG_ERROR) The '-d' or '--dir' flag must contain a "
                     "valid directory path.";
        std::cerr << " Use flag '-h' or '--help' for more information." << std::endl;
        break;
    }
    case constants::ARG_FILES_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::FILES_FLAG_ERROR) The '-f' or '--files' flag must contain at least "
                     "1 .env file.";
        std::cerr << " Use flag '-h' or '--help' for more information." << std::endl;
        break;
    }
    case constants::ARG_REQUIRED_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::REQUIRED_FLAG_ERROR) The '-r' or '--required' flag must contain at "
                     "least 1 ENV key.";
        std::cerr << " Use flag '-h' or '--help' for more information." << std::endl;
        break;
    }
    case constants::ARG_HELP_DOC: {
        // clang-format off
        std::clog << "┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n"
                     "│ NVI CLI Documentation                                                                                                  │\n"
                     "├─────────────────┬──────────────────────────────────────────────────────────────────────────────────────────────────────┤\n"
                     "│ flag            │ description                                                                                          │\n"
                     "├─────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────┤\n"
                     "│ -c, --config    │ Specifies which environment configuration to load from the env.config.json file. (ex: --config dev)  │\n"
                     "│ -de, --debug    │ Specifies whether or not to log file parsing details. (ex: --debug)                                  │\n"
                     "│ -d, --dir       │ Specifies which directory the env file is located within. (ex: --dir path/to/env)                    │\n"
                     "│ -f, --files     │ Specifies which .env file to parse separated by a space. (ex: --files test.env test2.env)            │\n"
                     "│ -r, --required  │ Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)               │\n"
                     "└─────────────────┴──────────────────────────────────────────────────────────────────────────────────────────────────────┘"
                  << std::endl;
        // clang-format on
        break;
    }
    case constants::ARG_INVALID_ARG_WARNING: {
        string args = this->invalid_args.length() ? " '" + this->invalid_args + "'" : "out";
        std::clog << "[nvi] (arg::INVALID_FLAG_WARNING) The flag '" << this->invalid_arg << "' with" << args
                  << " arguments is not recognized. Skipping." << std::endl;
        break;
    }
    case constants::ARG_EXCEPTION: {
        std::cerr << "[nvi] (arg::EXCEPTION) Failed to parse arguments. See below for more information." << std::endl;
        break;
    }
    case constants::ARG_DEBUG: {
        std::clog << "[nvi] (arg::DEBUG) The following arguments were set: ";
        std::clog << "config='" << this->config << "', ";
        std::clog << "debug='true', ";
        std::clog << "dir='" << this->dir << "', ";
        std::stringstream files;
        std::copy(this->files.begin(), this->files.end(), std::ostream_iterator<string>(files, ","));
        std::clog << "files='" << files.str() << "', ";
        std::stringstream envs;
        std::copy(this->required_envs.begin(), this->required_envs.end(), std::ostream_iterator<string>(envs, ","));
        std::clog << "required='" << envs.str() << "'." << std::endl;
        const bool conflicting_flags =
            this->config.length() && (this->dir.length() || this->files.size() > 1 || this->required_envs.size());
        if (conflicting_flags) {
            std::clog
                << "[nvi] (arg::DEBUG) Found conflicting arguments. When the 'config' argument has been set, then "
                   "'dir', 'files', and 'required' are ignored."
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
