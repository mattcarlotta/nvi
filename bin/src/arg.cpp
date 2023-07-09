#include "arg.h"
#include "constants.h"
#include "format.h"
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
arg_parser::arg_parser(int &argc, char *argv[]) : argc(argc), argv(argv) {
    this->key = 1;
    while (this->key < this->argc) {
        const string arg = string(this->argv[this->key]);
        if (arg == "-c" || arg == "--config") {
            this->config = this->parse_single_arg(constants::ARG_CONFIG_FLAG_ERROR);
        } else if (arg == "-de" || arg == "--debug") {
            this->debug = true;
        } else if (arg == "-d" || arg == "--dir") {
            this->dir = this->parse_single_arg(constants::ARG_DIR_FLAG_ERROR);
        } else if (arg == "-f" || arg == "--files") {
            this->files = this->parse_multi_arg(constants::ARG_FILES_FLAG_ERROR);
        } else if (arg == "-h" || arg == "--help") {
            this->log(constants::ARG_HELP_DOC);
            std::exit(0);
        } else if (arg == "-r" || arg == "--required") {
            this->required_envs = this->parse_multi_arg(constants::ARG_REQUIRED_FLAG_ERROR);
        } else {
            this->invalid_arg = string(this->argv[this->key]);
            while (this->key < this->argc) {
                ++this->key;

                if (this->argv[this->key] == nullptr) {
                    break;
                }

                const string arg = string(this->argv[this->key]);
                if (arg.find("-") != string::npos) {
                    this->key -= 1;
                    break;
                }

                this->invalid_args += this->invalid_args.length() ? " " + arg : arg;
            }

            this->log(constants::ARG_INVALID_FLAG_WARNING);
        }

        ++this->key;
    }

    if (this->debug) {
        this->log(constants::ARG_DEBUG);
    }
};

string arg_parser::parse_single_arg(unsigned int code) {
    ++this->key;
    if (this->argv[this->key] == nullptr) {
        this->log(code);
        std::exit(1);
    }

    const string arg = string(this->argv[this->key]);
    if (arg.find("-") != string::npos) {
        this->log(code);
        std::exit(1);
    }

    return arg;
}

vector<string> arg_parser::parse_multi_arg(unsigned int code) {
    vector<string> arg;
    ++this->key;
    while (this->key < this->argc) {
        if (this->argv[this->key] == nullptr) {
            break;
        }

        const string next_arg = string(this->argv[this->key]);
        if (next_arg.find("-") != string::npos) {
            this->key -= 1;
            break;
        }

        arg.push_back(next_arg);
        ++this->key;
    }

    if (!arg.size()) {
        this->log(code);
        std::exit(1);
    }

    return arg;
}

void arg_parser::log(unsigned int code) const {
    switch (code) {
    case constants::ARG_CONFIG_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::CONFIG_FLAG_ERROR) The \"-c\" or \"--config\" flag must contain an "
                     "environment name from the env.config.json configuration file. Use flag \"-h\" or \"--help\" for "
                     "more information."
                  << std::endl;
        break;
    }
    case constants::ARG_DIR_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::DIR_FLAG_ERROR) The \"-d\" or \"--dir\" flag must contain a "
                     "valid directory path. Use flag \"-h\" or \"--help\" for more information."
                  << std::endl;
        break;
    }
    case constants::ARG_FILES_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::FILES_FLAG_ERROR) The \"-f\" or \"--files\" flag must contain at least "
                     "1 .env file. Use flag \"-h\" or \"--help\" for more information."
                  << std::endl;
        break;
    }
    case constants::ARG_REQUIRED_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::REQUIRED_FLAG_ERROR) The \"-r\" or \"--required\" flag must contain at "
                     "least 1 ENV key. Use flag \"-h\" or \"--help\" for more information."
                  << std::endl;
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
                     "│ -f, --files     │ Specifies which .env files to parse separated by a space. (ex: --files test.env test2.env)           │\n"
                     "│ -r, --required  │ Specifies which ENV keys are required separated by a space. (ex: --required KEY1 KEY2)               │\n"
                     "└─────────────────┴──────────────────────────────────────────────────────────────────────────────────────────────────────┘"
                  << std::endl;
        // clang-format on
        break;
    }
    case constants::ARG_INVALID_FLAG_WARNING: {
        const string args = this->invalid_args.length() ? " \"" + this->invalid_args + "\"" : "out";
        std::clog
            << format(
                   "[nvi] (arg::INVALID_FLAG_WARNING) The flag \"%s\" with\%s arguments is not recognized. Skipping.",
                   this->invalid_arg.c_str(), args.c_str())
            << std::endl;
        break;
    }
    case constants::ARG_EXCEPTION: {
        std::cerr << "[nvi] (arg::EXCEPTION) Failed to parse arguments. See below for more information." << std::endl;
        break;
    }
    case constants::ARG_DEBUG: {
        std::stringstream files;
        std::copy(this->files.begin(), this->files.end(), std::ostream_iterator<string>(files, ","));

        std::stringstream envs;
        std::copy(this->required_envs.begin(), this->required_envs.end(), std::ostream_iterator<string>(envs, ","));

        std::clog << format("[nvi] (arg::DEBUG) The following arguments were set: config=\"%s\", "
                            "debug=\"true\", dir=\"%s\", files=\"%s\", required=\"%s\".",
                            this->config.c_str(), this->dir.c_str(), files.str().c_str(), envs.str().c_str())
                  << std::endl;

        if (this->config.length() && (this->dir.length() || this->files.size() > 1 || this->required_envs.size())) {
            std::clog << "[nvi] (arg::DEBUG) Found conflicting flags. When the \"config\" argument has been set, then "
                         "\"dir,\" \"files,\" and \"required\" are ignored."
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
