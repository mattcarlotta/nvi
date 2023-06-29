#include "load_config.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>

using std::string;
using std::vector;

namespace nvi {
config::config(const string &environment, const string dir) {
    this->env = environment;
    this->file_path = string(std::filesystem::current_path() / dir / "env.config.json");
    std::ifstream env_config_file(this->file_path);
    if (!env_config_file.good()) {
        std::cerr << "[nvi] ERROR: Unable to locate '" << this->file_path << "'. The file doesn't appear to exist!"
                  << std::endl;
        exit(1);
    }

    nlohmann::json config = nlohmann::json::parse(env_config_file);
    if (!config.contains(this->env)) {
        std::cerr << "[nvi] ERROR: Unable to load the '" << this->env
                  << "' environment within the env.config.json configuration file (" << this->file_path
                  << "). The specified environment doesn't appear to exist!" << std::endl;
        exit(1);
    }

    this->env_config = config.at(this->env);
};

const string config::get_dir() {
    if (this->env_config.contains("dir")) {
        return this->env_config.at("dir");
    } else {
        return "";
    }
};

const bool config::get_debug() {
    if (this->env_config.contains("debug")) {
        return this->env_config.at("debug");
    } else {
        return false;
    }
};

const vector<string> config::get_files() {
    if (this->env_config.contains("files")) {
        return this->env_config.at("files");
    } else {
        std::cerr << "[nvi] ERROR: Unable to locate a 'files' property within the '" << this->env
                  << "' environment configuration (" << this->file_path
                  << "). You must specify at least 1 .env file to load!" << std::endl;
        exit(1);
    }
};

const vector<string> config::get_required_envs() {
    if (this->env_config.contains("required")) {
        return this->env_config.at("required");
    } else {
        return {};
    }
};
} // namespace nvi
