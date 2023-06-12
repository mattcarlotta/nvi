#include "arg.h"
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace nvi {
arg_parser::arg_parser(int &argc, char *argv[]) {
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
