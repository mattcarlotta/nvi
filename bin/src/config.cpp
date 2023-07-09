#include "config.h"
#include "constants.h"
#include "format.h"
#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using std::string;

namespace nvi {
config::config(const string *environment, const string env_dir) : env(environment) {
    this->file_path = string(std::filesystem::current_path() / env_dir / "env.config.json");
    std::ifstream env_config_file(this->file_path);
    if (!env_config_file.good()) {
        this->log(constants::CONFIG_FILE_ERROR);
        std::exit(1);
    }

    this->parsed_config = nlohmann::json::parse(env_config_file);
    if (!this->parsed_config.count(*this->env)) {
        this->log(constants::CONFIG_FILE_PARSE_ERROR);
        std::exit(1);
    }

    this->env_config = this->parsed_config.at(*this->env);

    if (this->env_config.count("files")) {
        this->files = this->env_config.at("files");
    } else {
        this->log(constants::CONFIG_MISSING_FILES_ARG_ERROR);
        std::exit(1);
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
        std::cerr << format("[nvi] (config::FILE_ERROR) Unable to locate \"%s\". The configuration file doesn't appear "
                            "to exist!",
                            this->file_path.c_str())
                  << std::endl;
        break;
    }
    case constants::CONFIG_FILE_PARSE_ERROR: {
        std::cerr << format(
                         "[nvi] (config::FILE_PARSE_ERROR) Unable to load a \"%s\" environment from the "
                         "env.config.json configuration file (%s). The specified environment doesn't appear to exist!",
                         &*this->env->c_str(), this->file_path.c_str())
                  << std::endl;
        break;
    }
    case constants::CONFIG_DEBUG: {
        string keys;
        for (auto &el : this->parsed_config.items()) {
            keys += el.key() + ",";
        }

        std::clog
            << format("[nvi] (config::DEBUG) Parsed the following keys \"%s\" from the env.config.json configuration "
                      "file and selected the \"%s\" configuration.",
                      keys.c_str(), &*this->env->c_str())
            << std::endl;

        std::stringstream files;
        std::copy(this->files.begin(), this->files.end(), std::ostream_iterator<string>(files, ","));

        std::stringstream envs;
        std::copy(this->required_envs.begin(), this->required_envs.end(), std::ostream_iterator<string>(envs, ","));

        std::clog << format("[nvi] (config::DEBUG) The following arguments were set: "
                            "debug=\"true\", dir=\"%s\", files=\"%s\", required=\"%s\".\n",
                            this->dir.c_str(), files.str().c_str(), envs.str().c_str())
                  << std::endl;
        break;
    }
    case constants::CONFIG_MISSING_FILES_ARG_ERROR: {
        std::cerr << format("[nvi] (config::MISSING_FILES_ARG_ERROR) Unable to locate a \"files\" property within the "
                            "\"%s\" environment configuration (%s). You must specify at least 1 .env file to load!",
                            &*this->env->c_str(), this->file_path.c_str())
                  << std::endl;
        break;
    }
    default:
        break;
    }
}
} // namespace nvi
