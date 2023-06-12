#include "../src/arg.h"
#include <cassert>
// #include <iostream>
// #include <string>

int main() {
    int argc = 2;
    char *argv[] = {(char *)"file", (char *)".env"};

    // nvi::arg_parser args(argc, argv);
    // std::string file = args.get("file");
    // std::cout << file << std::endl;
    // assert(file == ".env");

    return 0;
}
