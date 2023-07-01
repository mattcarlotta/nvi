#include "config.h"
#include "constants.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

using std::string;
using std::vector;

namespace nvi {
config::config(const string &environment, const string dir) {
    this->env = environment;
    this->file_path = string(std::filesystem::current_path() / dir / "env.config.json");
    std::ifstream env_config_file(this->file_path);
    if (!env_config_file.good()) {
        this->log(constants::CONFIG_FILE_ERROR);
    }

    nlohmann::json::object_t config = nlohmann::json::parse(env_config_file);
    if (!config.count(this->env)) {
        this->log(constants::CONFIG_FILE_PARSE_ERROR);
    }

    this->env_config = config.at(this->env);
};

const std::optional<bool> config::get_debug() noexcept {
    if (this->env_config.count("debug")) {
        return this->env_config.at("debug");
    } else {
        return std::nullopt;
    }
};

const std::optional<string> config::get_dir() noexcept {
    if (this->env_config.count("dir")) {
        return this->env_config.at("dir");
    } else {
        return std::nullopt;
    }
};

const std::optional<vector<string>> config::get_files() {
    if (this->env_config.count("files")) {
        return this->env_config.at("files");
    } else {
        this->log(constants::CONFIG_MISSING_FILES_ARG_ERROR);
        //@@@ this is not needed because the above exits the process,
        // it's just here to ignore a non-void return warning
        return std::nullopt;
    }
};

const vector<string> config::get_required_envs() noexcept {
    if (this->env_config.count("required")) {
        return this->env_config.at("required");
    } else {
        return vector<string>();
    }
};

void config::log(unsigned int code) const {
    switch (code) {
    case constants::CONFIG_FILE_ERROR: {
        std::cerr << "[nvi] (config::FILE_ERROR) Unable to locate '" << this->file_path
                  << "'. The configuration file doesn't appear to exist!" << std::endl;
        break;
    }
    case constants::CONFIG_FILE_PARSE_ERROR: {
        std::cerr << "[nvi] (config::FILE_PARSE_ERROR) Unable to load a '" << this->env
                  << "' environment from the env.config.json configuration file (" << this->file_path
                  << "). The specified environment doesn't appear to exist!" << std::endl;
        break;
    }
    case constants::CONFIG_MISSING_FILES_ARG_ERROR: {
        std::cerr << "[nvi] (config::MISSING_FILES_ARG_ERROR) Unable to locate a 'files' property within the '"
                  << this->env << "' environment configuration (" << this->file_path
                  << "). You must specify at least 1 .env file to load!" << std::endl;
        break;
    }
    default:
        break;
    }

    exit(1);
}
} // namespace nvi
