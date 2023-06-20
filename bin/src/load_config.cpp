#include "load_config.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
config::config(const string &environment, const string dir) {
    env = environment;
    file_path = std::filesystem::current_path() / dir / "env.config.json";
    std::ifstream env_config_file(file_path.string());
    if (!env_config_file.good()) {
        std::cerr << "[nvi] ERROR: Unable to locate '" << file_path.string() << "'. The file doesn't appear to exist!"
                  << std::endl;
        exit(1);
    }

    try {
        nlohmann::json data = nlohmann::json::parse(env_config_file);
        env_config = data.at(env);
    } catch (const std::out_of_range &) {
        std::cerr << "[nvi] ERROR: Unable to locate a '" << env << "' configuration within the env.config.json!"
                  << std::endl;
        exit(1);
    }
};

const string config::get_dir() {
    if (env_config.contains("dir")) {
        return env_config.at("dir");
    } else {
        return "";
    }
};

const bool config::get_debug() {
    if (env_config.contains("debug")) {
        return env_config.at("debug");
    } else {
        return false;
    }
};

const vector<string> config::get_files() {
    if (env_config.contains("files")) {
        return env_config.at("files");
    } else {
        std::cerr << "[nvi] ERROR: Unable to locate a 'files' property within the env.config.json. You must specify at "
                     "least 1 env "
                     "file to load!"
                  << std::endl;
        exit(1);
    }
};

const vector<string> config::get_required_envs() {
    if (env_config.contains("required")) {
        return env_config.at("required");
    } else {
        return {};
    }
};
} // namespace nvi
