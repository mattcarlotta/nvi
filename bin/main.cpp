#include "arg.h"
#include "env.h"
#include "json/single_include/nlohmann/json.hpp"
#include <filesystem>

int main(int argc, char *argv[]) {
    nvi::arg_parser args(argc, argv);

    std::string env_file_name = args.get("-f");
    if (!env_file_name.length()) {
        env_file_name = ".env";
    }

    nlohmann::json env_map;

    nvi::file env_file(args.get("-d"), env_file_name);

    env_map = env_file.parse(env_map);

    std::cout << env_map << std::endl;

    return 0;
}
