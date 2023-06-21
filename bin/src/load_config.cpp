#include "load_config.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>
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

    nlohmann::json config = nlohmann::json::parse(env_config_file);
    if (!config.contains(env)) {
        std::cerr << "[nvi] ERROR: Unable to load the '" << env
                  << "' environment within the env.config.json configuration file (" << file_path.string()
                  << "). The specified environment doesn't appear to exist!" << std::endl;
        exit(1);
    }

    env_config = config.at(env);
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
        std::cerr << "[nvi] ERROR: Unable to locate a 'files' property within the '" << env
                  << "' environment configuration (" << file_path.string()
                  << "). You must specify at least 1 .env file to load!" << std::endl;
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
