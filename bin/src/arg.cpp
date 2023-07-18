#include "arg.h"
#include "constants.h"
#include "format.h"
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
arg_parser::arg_parser(int &argc, char *argv[]) : argc(argc), argv(argv) {
    this->index = 1;
    while (this->index < this->argc) {
        const string arg = string(this->argv[this->index]);
        if (arg == "-c" || arg == "--config") {
            this->config = this->parse_single_arg(constants::ARG_CONFIG_FLAG_ERROR);
        } else if (arg == "-de" || arg == "--debug") {
            this->debug = true;
        } else if (arg == "-d" || arg == "--dir") {
            this->dir = this->parse_single_arg(constants::ARG_DIR_FLAG_ERROR);
        } else if (arg == "-e" || arg == "--exec") {
            this->parse_command_args();
        } else if (arg == "-f" || arg == "--files") {
            this->files = this->parse_multi_arg(constants::ARG_FILES_FLAG_ERROR);
        } else if (arg == "-h" || arg == "--help") {
            this->log(constants::ARG_HELP_DOC);
            std::exit(0);
        } else if (arg == "-r" || arg == "--required") {
            this->required_envs = this->parse_multi_arg(constants::ARG_REQUIRED_FLAG_ERROR);
        } else {
            this->invalid_arg = string(this->argv[this->index]);
            this->invalid_args = "";
            while (this->index < this->argc) {
                ++this->index;

                if (this->argv[this->index] == nullptr) {
                    break;
                }

                const string arg = string(this->argv[this->index]);
                if (arg.find("-") != string::npos) {
                    this->index -= 1;
                    break;
                }

                this->invalid_args += this->invalid_args.length() ? " " + arg : arg;
            }

            this->log(constants::ARG_INVALID_FLAG_WARNING);
        }

        ++this->index;
    }

    if (this->debug) {
        this->log(constants::ARG_DEBUG);
    }
};

string arg_parser::parse_single_arg(unsigned int code) {
    ++this->index;
    if (this->argv[this->index] == nullptr) {
        this->log(code);
        std::exit(1);
    }

    const string arg = string(this->argv[this->index]);
    if (arg.find("-") != string::npos) {
        this->log(code);
        std::exit(1);
    }

    return arg;
}

vector<string> arg_parser::parse_multi_arg(unsigned int code) {
    vector<string> arg;
    ++this->index;
    while (this->index < this->argc) {
        if (this->argv[this->index] == nullptr) {
            break;
        }

        const string next_arg = string(this->argv[this->index]);
        if (next_arg.find("-") != string::npos) {
            this->index -= 1;
            break;
        }

        arg.push_back(next_arg);
        ++this->index;
    }

    if (!arg.size()) {
        this->log(code);
        std::exit(1);
    }

    return arg;
}

void arg_parser::parse_command_args() {
    ++this->index;
    while (this->index < this->argc) {
        if (this->argv[this->index] == nullptr) {
            break;
        }

        string next_arg = string(this->argv[this->index]);
        if (next_arg.find("-") != string::npos) {
            this->index -= 1;
            break;
        } else if (!this->commands.size()) {
            this->bin_name = next_arg;
        }

        char *arg_str = new char[next_arg.size() + 1];
        std::strcpy(arg_str, next_arg.c_str());

        this->command += this->command.size() > 0 ? " " + next_arg : next_arg;
        this->commands.push_back(arg_str);
        ++this->index;
    }

    if (!this->commands.size()) {
        this->log(constants::ARG_COMMAND_FLAG_ERROR);
        std::exit(1);
    }

    this->commands.push_back(nullptr);
}

void arg_parser::log(unsigned int code) const noexcept {
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
    case constants::ARG_COMMAND_FLAG_ERROR: {
        std::cerr << "[nvi] (arg::COMMAND_FLAG_ERROR) The \"-e\" or \"--exec\" flag must contain at least "
                     "1 command. Use flag \"-h\" or \"--help\" for more information."
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
    case constants::ARG_INVALID_FLAG_WARNING: {
        std::clog
            << format(
                   "[nvi] (arg::INVALID_FLAG_WARNING) The flag \"%s\" with\%s arguments is not recognized. Skipping.",
                   this->invalid_arg.c_str(),
                   (this->invalid_args.length() ? " \"" + this->invalid_args + "\"" : "out").c_str())
            << std::endl;
        break;
    }
    case constants::ARG_DEBUG: {
        std::clog << format("[nvi] (arg::DEBUG) The following flags were set: config=\"%s\", "
                            "debug=\"true\", dir=\"%s\", execute=\"%s\", files=\"%s\", required=\"%s\".",
                            this->config.c_str(), this->dir.c_str(), this->command.c_str(),
                            join(this->files, ", ").c_str(), join(this->required_envs, ", ").c_str())
                  << std::endl;

        if (this->config.length() &&
            (this->dir.length() || this->commands.size() || this->files.size() > 1 || this->required_envs.size())) {
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
