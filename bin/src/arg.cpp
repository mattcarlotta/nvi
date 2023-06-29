#include "arg.h"
#include <iostream>
#include <optional>

namespace nvi {
arg_parser::arg_parser(int &argc, char *argv[]) {
    if (argc % 2 == 0) {
        if (std::string(argv[1]) == "--help") {
            std::clog
                << "┌─────────────────────────────────────────────────────────────────────────────────────────────"
                   "───────────────────────────────────┐"
                << std::endl;
            std::clog << "│ NVI CLI Documentation                                                                      "
                         "                                    │"
                      << std::endl;
            std::clog
                << "├──────────┬──────────────────────────────────────────────────────────────────────────────────"
                   "───────────────────────────────────┤"
                << std::endl;
            std::clog
                << "│ flag     │ description                                                                       "
                   "                                  │"
                << std::endl;
            std::clog
                << "├──────────┼──────────────────────────────────────────────────────────────────────────────────"
                   "───────────────────────────────────┤"
                << std::endl;
            std::clog << "│ --config │ Specifies which environment configuration you'd like to load from the "
                         "env.config.json file. (ex: --config dev)      │"
                      << std::endl;
            std::clog
                << "│ --debug  │ Specifies whether or not to log file parsing details. (ex: --debug on|off)         "
                   "                                 │"
                << std::endl;
            std::clog
                << "│ --dir    │ Specifies which directory the env file is located within. (ex: --dir path/to/env)  "
                   "                                 │"
                << std::endl;
            std::clog
                << "│ --file   │ Specifies which .env file to load. (ex: --file test.env)                           "
                   "                                 │"
                << std::endl;
            std::clog
                << "└──────────┴──────────────────────────────────────────────────────────────────────────────────"
                   "───────────────────────────────────┘"
                << std::endl;
            exit(0);
        } else {
            std::cerr
                << "[nvi] You must pass valid arguments with '--flag arg'. Use flag '--help' for more information."
                << std::endl;
            exit(1);
        }
    }

    for (int i = 1; i < argc; i += 2) {
        this->args.insert(std::make_pair(argv[i], argv[i + 1]));
    }
};

const std::optional<std::string> arg_parser::get(const std::string &flag) noexcept {
    try {
        return this->args.at(flag);
    } catch (const std::out_of_range &) {
        return std::nullopt;
    }
};
} // namespace nvi
