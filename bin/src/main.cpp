#include "arg.h"
#include "env.h"
#include "json.cpp"
#include "load_config.h"
#include <filesystem>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

int main(int argc, char *argv[]) {
    nlohmann::json env_map;
    nvi::arg_parser args(argc, argv);

    string env = args.get("--config");
    if (env.length()) {
        nvi::config config(env);
        const bool debug = config.get_debug();
        const string dir = config.get_dir();
        const vector<string> env_files = config.get_files();
        const vector<string> required_envs = config.get_required_envs();

        for (string env : env_files) {
            nvi::file env_file(dir, env, debug);
            env_map = env_file.parse(env_map);
        }

        if (required_envs.size()) {
            vector<string> undefined_keys;
            for (string key : required_envs) {
                if (!env_map.contains(key)) {
                    undefined_keys.push_back(key);
                }
            }

            if (undefined_keys.size()) {
                std::stringstream envs;
                std::copy(undefined_keys.begin(), undefined_keys.end(), std::ostream_iterator<string>(envs, ", "));
                std::cerr << "[nvi] ERROR: The following Envs are marked as required: " << envs.str()
                          << "but they are undefined after all of the .env files were parsed." << std::endl;
                exit(1);
            }
        }
    } else {
        string env_file_name = args.get("--file");
        if (!env_file_name.length()) {
            env_file_name = ".env";
        }

        nvi::file env_file(args.get("--dir"), env_file_name, args.get("--debug") == "on");

        env_map = env_file.parse(env_map);
    }

    std::cout << std::setw(4) << env_map << std::endl;

    return 0;
}
