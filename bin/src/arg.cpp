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
    if (argv == nullptr)
        return;

    for (unsigned int i = 1; i < argc; ++i) {
        try {
            const string arg = string(argv[i]);
            if (arg == "-c" || arg == "--config") {
                if (argv[i + 1] == nullptr)
                    this->log(constants::ARG_CONFIG_FLAG_ERROR);

                const string next_arg = string(argv[i + 1]);
                if (next_arg.find("-") != string::npos)
                    this->log(constants::ARG_CONFIG_FLAG_ERROR);

                this->config = next_arg;
                ++i;
            } else if (arg == "-de" || arg == "--debug") {
                this->debug = true;
            } else if (arg == "-d" || arg == "--dir") {
                if (argv[i + 1] == nullptr)
                    this->log(constants::ARG_DIR_FLAG_ERROR);

                const string next_arg = string(argv[i + 1]);
                if (next_arg.find("-") != string::npos)
                    this->log(constants::ARG_DIR_FLAG_ERROR);

                this->dir = next_arg;
                i++;
            } else if (arg == "-f" || arg == "--files") {
                ++i;
                std::vector<string> files;
                while (i < argc) {
                    if (argv[i] == nullptr)
                        break;

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
                } else {
                    this->files = files;
                }
            } else if (arg == "-h" || arg == "--help") {
                this->log(constants::ARG_HELP_DOC);
            } else if (arg == "-r" || arg == "--required") {
                ++i;
                while (i < argc) {
                    if (argv[i] == nullptr)
                        break;

                    const string next_arg = string(argv[i]);
                    if (next_arg.find("-") != string::npos) {
                        i -= 1;
                        break;
                    }

                    this->required_envs.push_back(next_arg);
                    ++i;
                }

                if (!this->required_envs.size())
                    this->log(constants::ARG_REQUIRED_FLAG_ERROR);
            } else {
                const string invalid_arg = string(argv[i]);
                string invalid_args;
                while (i < argc) {
                    ++i;

                    if (argv[i] == nullptr)
                        break;

                    const string next_arg = string(argv[i]);
                    if (next_arg.find("-") != string::npos) {
                        i -= 1;
                        break;
                    }

                    invalid_args += invalid_args.length() ? " " + next_arg : next_arg;
                }

                const string invalid_args_message = invalid_args.length() ? " '" + invalid_args + "'" : "out";

                std::clog << "[nvi] WARNING(ARG_INVALID_FLAG): The flag '" << invalid_arg << "' with"
                          << invalid_args_message << " arguments is not recognized. Skipping." << std::endl;
            }
        } catch (std::exception &e) {
            std::cout << "[nvi] ERROR: Failed to parse arguments. See below for more information." << std::endl;
            std::cout << e.what() << std::endl;
            exit(1);
        }
    }
};

void arg_parser::log(int code) const {
    switch (code) {
    case constants::ARG_CONFIG_FLAG_ERROR: {
        std::clog << "[nvi] ERROR(ARG_CONFIG_FLAG_ERROR): The '-c' or '--config' flag must contain an envionment name.";
        break;
    }
    case constants::ARG_DIR_FLAG_ERROR: {
        std::clog << "[nvi] ERROR(ARG_DIR_FLAG_ERROR): The '-d' or '--dir' flag must contain an envionment name.";
        break;
    }
    case constants::ARG_FILES_FLAG_ERROR: {
        std::clog << "[nvi] ERROR(ARG_FILES_FLAG_ERROR): The '-f' or '--files' flag must contain at least 1 .env file.";
        break;
    }
    case constants::ARG_REQUIRED_FLAG_ERROR: {
        std::clog
            << "[nvi] ERROR(ARG_REQUIRED_FLAG_ERROR): The '-r' or '--required' flag must contain at least 1 ENV key.";
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
                     "│ -f, --files     │ Specifies which .env file to load. (ex: --files test.env test2.env)                                  │\n"
                     "│ -r, --required  │ Specifies which ENV keys are required. (ex: --required KEY1 KEY2)                                    │\n"
                     "└─────────────────┴──────────────────────────────────────────────────────────────────────────────────────────────────────┘"
                  << std::endl;
        // clang-format on
        exit(0);
    }
    default:
        break;
    }

    std::cout << " Use flag '-h' or '--help' for more information." << std::endl;

    exit(1);
}

} // namespace nvi
