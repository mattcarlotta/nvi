#include "arg.h"
#include <cassert>
#include <iostream>

int main(int argc, char *argv[]) {
    nvi::arg_parser args(argc, argv);
    std::string file = args.get("-f");
    assert(file == ".env");

    return 0;
}
