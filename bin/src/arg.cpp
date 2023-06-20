#include "arg.h"
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace nvi {
arg_parser::arg_parser(int &argc, char *argv[]) {
    if (argc % 2 == 0) {
        if (std::string(argv[1]) == "-help") {
            std::clog
                << " --------------------------------------------------------------------------------------------- "
                << std::endl;
            std::clog
                << "| NVI Help & Documentation                                                                    |"
                << std::endl;
            std::clog
                << "|---------------------------------------------------------------------------------------------|"
                << std::endl;
            std::clog
                << "| flag   | description                                                                        |"
                << std::endl;
            std::clog
                << "|--------|------------------------------------------------------------------------------------|"
                << std::endl;
            std::clog
                << "| -debug | Specifies whether or not to show a debug log. (ex: -debug on|off)                  |"
                << std::endl;
            std::clog
                << "| -dir   | Specifies which directory the env file is located within. (ex: -dir path/to/env)   |"
                << std::endl;
            std::clog
                << "| -file  | Specifies which env file you'd like to load. (ex: -file test.env)                  |"
                << std::endl;
            std::clog
                << " ---------------------------------------------------------------------------------------------"
                << std::endl;
            exit(0);
        } else {
            std::cerr << "[nvi] You must pass valid arguments with '-flag arg'. Use flag '-help' for more information."
                      << std::endl;
            exit(1);
        }
    }

    for (int i = 1; i < argc; i += 2) {
        args.insert(std::make_pair(argv[i], argv[i + 1]));
    }
};

const std::string arg_parser::get(const std::string &flag) {
    try {
        return args.at(flag);
    } catch (const std::out_of_range &) {
        return "";
    }
};
} // namespace nvi
