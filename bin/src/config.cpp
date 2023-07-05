#include "config.h"
#include "constants.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using std::string;

namespace nvi {
config::config(const string &environment, const string dir) {
    this->env = environment;
    this->file_path = string(std::filesystem::current_path() / dir / "env.config.json");
    std::ifstream env_config_file(this->file_path);
    if (!env_config_file.good()) {
        this->log(constants::CONFIG_FILE_ERROR);
        exit(1);
    }

    this->parsed_config = nlohmann::json::parse(env_config_file);
    if (!this->parsed_config.count(this->env)) {
        this->log(constants::CONFIG_FILE_PARSE_ERROR);
        exit(1);
    }

    this->env_config = this->parsed_config.at(this->env);

    if (this->env_config.count("files")) {
        this->files = this->env_config.at("files");
    } else {
        this->log(constants::CONFIG_MISSING_FILES_ARG_ERROR);
        exit(1);
    }

    if (this->env_config.count("debug")) {
        this->debug = this->env_config.at("debug");
    }

    if (this->env_config.count("dir")) {
        this->dir = this->env_config.at("dir");
    }

    if (this->env_config.count("required")) {
        this->required_envs = this->env_config.at("required");
    }

    if (this->debug) {
        this->log(constants::CONFIG_DEBUG);
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
    case constants::CONFIG_DEBUG: {
        std::clog << "[nvi] (config::DEBUG) Parsed the following keys from the env.config.json configuration file: '";
        for (auto &el : this->parsed_config.items()) {
            std::clog << el.key() << ",";
        }
        std::clog << "' and selected the '" << this->env << "' configuration." << std::endl;
        std::clog << "[nvi] (config::DEBUG) The following '" << this->env << "' configuration settings were set: ";
        std::clog << "debug='true', ";
        std::clog << "dir='" << this->dir << "', ";
        std::stringstream files;
        std::copy(this->files.begin(), this->files.end(), std::ostream_iterator<string>(files, ","));
        std::clog << "files='" << files.str() << "', ";
        std::stringstream envs;
        std::copy(this->required_envs.begin(), this->required_envs.end(), std::ostream_iterator<string>(envs, ","));
        std::clog << "required='" << envs.str() << "'.\n" << std::endl;
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
}
} // namespace nvi
