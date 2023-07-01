#include "arg.h"
#include "constants.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using std::string;

namespace nvi {
arg_parser::arg_parser(int &argc, char *argv[]) {
    if (argv == nullptr) {
        return;
    }

    for (unsigned int i = 1; i < argc; ++i) {
        try {
            const string arg = string(argv[i]);
            if (arg == "-c" || arg == "--config") {
                ++i;
                if (argv[i] == nullptr) {
                    this->log(constants::ARG_CONFIG_FLAG_ERROR);
                }

                const string next_arg = string(argv[i]);
                if (next_arg.find("-") != string::npos) {
                    this->log(constants::ARG_CONFIG_FLAG_ERROR);
                }

                this->config = next_arg;
            } else if (arg == "-de" || arg == "--debug") {
                this->debug = true;
            } else if (arg == "-d" || arg == "--dir") {
                i++;
                if (argv[i] == nullptr) {
                    this->log(constants::ARG_DIR_FLAG_ERROR);
                }

                const string next_arg = string(argv[i]);
                if (next_arg.find("-") != string::npos) {
                    this->log(constants::ARG_DIR_FLAG_ERROR);
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
                }

                this->files = files;
            } else if (arg == "-h" || arg == "--help") {
                this->log(constants::ARG_HELP_DOC);
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
                }
            } else {
                this->invalid_arg = string(argv[i]);
                string invalid_args;
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

                    invalid_args += invalid_args.length() ? " " + next_arg : next_arg;
                }

                this->invalid_args_message = invalid_args.length() ? " '" + invalid_args + "'" : "out";
                this->log(constants::ARG_INVALID_ARG_WARNING);
            }

        } catch (std::exception &e) {
            std::cerr << "[nvi] (arg_parser::ERROR_EXCEPTION) Failed to parse arguments. See below for more "
                         "information."
                      << std::endl;
            std::cerr << e.what() << std::endl;
            exit(1);
        }
    }

    if (this->debug) {
        this->log(constants::ARG_DEBUG);
    }
};

void arg_parser::log(unsigned int code) const {
    switch (code) {
    case constants::ARG_CONFIG_FLAG_ERROR: {
        std::cerr << "[nvi] (arg_parser::ARG_CONFIG_FLAG_ERROR) The '-c' or '--config' flag must contain an "
                     "environment name.";
        std::cerr << " Use flag '-h' or '--help' for more information." << std::endl;
        exit(1);
    }
    case constants::ARG_DIR_FLAG_ERROR: {
        std::cerr << "[nvi] (arg_parser::ARG_DIR_FLAG_ERROR) The '-d' or '--dir' flag must contain a "
                     "valid directory path.";
        std::cerr << " Use flag '-h' or '--help' for more information." << std::endl;
        exit(1);
    }
    case constants::ARG_FILES_FLAG_ERROR: {
        std::cerr << "[nvi] (arg_parser::ARG_FILES_FLAG_ERROR) The '-f' or '--files' flag must contain at least "
                     "1 .env file.";
        std::cerr << " Use flag '-h' or '--help' for more information." << std::endl;
        exit(1);
    }
    case constants::ARG_REQUIRED_FLAG_ERROR: {
        std::cerr << "[nvi] (arg_parser::ARG_REQUIRED_FLAG_ERROR) The '-r' or '--required' flag must contain at "
                     "least 1 ENV key.";
        std::cerr << " Use flag '-h' or '--help' for more information." << std::endl;
        exit(1);
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
                     "│ -f, --files     │ Specifies which .env file to load. (ex: --files test.env test2.env)                                  │\n"
                     "│ -r, --required  │ Specifies which ENV keys are required. (ex: --required KEY1 KEY2)                                    │\n"
                     "└─────────────────┴──────────────────────────────────────────────────────────────────────────────────────────────────────┘"
                  << std::endl;
        // clang-format on
        exit(0);
    }
    case constants::ARG_DEBUG: {
        std::clog << "[nvi] (arg_parser::DEBUG) The following arguments were set: ";
        std::clog << "config='" << this->config << "', ";
        std::clog << "debug='true', ";
        std::clog << "dir='" << this->dir << "', ";
        std::clog << "files='";
        for (unsigned int i = 0; i < this->files.size(); ++i) {
            std::clog << files[i];
            if (i < this->files.size() - 1) {
                std::clog << ", ";
            }
        }
        std::clog << "', ";
        std::clog << "required='";
        for (unsigned int i = 0; i < this->required_envs.size(); ++i) {
            std::clog << required_envs[i];
            if (i < this->required_envs.size() - 1) {
                std::clog << ", ";
            }
        }
        std::clog << "'." << std::endl;
        break;
    }
    case constants::ARG_INVALID_ARG_WARNING: {
        std::clog << "[nvi] (arg_parser::ARG_INVALID_FLAG_WARNING) The flag '" << this->invalid_arg << "' with"
                  << this->invalid_args_message << " arguments is not recognized. Skipping." << std::endl;
        break;
    }
    default:
        break;
    }
}

} // namespace nvi
