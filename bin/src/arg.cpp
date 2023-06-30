#include "arg.h"
#include "constants.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using std::string;

namespace nvi {
arg_parser::arg_parser(int &argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (argv == nullptr)
            break;

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
            } else if (arg == "-dg" || arg == "--debug") {
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
                std::vector<string> files;
                for (int j = i + 1; j < argc; ++j) {
                    if (argv[j] == nullptr) {
                        i += j;
                        break;
                    }

                    const string next_arg = string(argv[j]);
                    if (next_arg.find("-") != string::npos)
                        break;

                    files.push_back(next_arg);
                }

                if (!files.size()) {
                    this->log(constants::ARG_FILES_FLAG_ERROR);
                } else {
                    this->files = files;
                }
            } else if (arg == "-h" || arg == "--help") {
                this->log(constants::ARG_HELP_DOC);
            } else if (arg == "-r" || arg == "--required") {
                for (int j = i + 1; j < argc; ++j) {
                    if (argv[j] == nullptr) {
                        i += j;
                        break;
                    }

                    const string next_arg = string(argv[j]);
                    if (next_arg.find("-") != string::npos)
                        break;

                    this->required_envs.push_back(next_arg);
                }

                if (!this->required_envs.size())
                    this->log(constants::ARG_REQUIRED_FLAG_ERROR);
            }
        } catch (std::out_of_range &e) {
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
                     "│ -dg, --debug    │ Specifies whether or not to log file parsing details. (ex: --debug)                                  │\n"
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
