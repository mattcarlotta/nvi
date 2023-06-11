#include "arg.hpp"
#include "env.hpp"
#include "json/single_include/nlohmann/json.hpp"
#include <filesystem>

int main(int argc, char *argv[]) {
    argparser args(argc, argv);

    const std::string env_file_name = args.get("-f");
    if (!env_file_name.length()) {
        std::cerr << "You must assign an .env file to read!" << std::endl;
        exit(1);
    }

    nlohmann::json env_map;

    envfile env(std::filesystem::current_path() / args.get("-d") / env_file_name);

    env_map = env.parse(env_map);

    std::cout << env_map << std::endl;

    return 0;
}
