#include "config.h"
#include "constants.h"
#include "format.h"
#include "json.cpp"
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace nvi {
    config::config(const std::string &environment, const std::string env_dir) : env_(environment) {
        file_path_ = std::string(std::filesystem::current_path() / env_dir / "env.config.json");
        if (!std::filesystem::exists(file_path_)) {
            log(constants::CONFIG_FILE_ERROR);
            std::exit(1);
        }

        std::ifstream env_config_file(file_path_, std::ios_base::in);
        parsed_config_ = nlohmann::json::parse(env_config_file);
        if (!parsed_config_.count(env_)) {
            log(constants::CONFIG_FILE_PARSE_ERROR);
            std::exit(1);
        }

        nlohmann::json::object_t env_config_ = parsed_config_.at(env_);

        if (env_config_.count("files")) {
            options_.files = env_config_.at("files");
        } else {
            log(constants::CONFIG_MISSING_FILES_ARG_ERROR);
            std::exit(1);
        }

        if (env_config_.count("debug")) {
            options_.debug = env_config_.at("debug");
        }

        if (env_config_.count("dir")) {
            options_.dir = env_config_.at("dir");
        }

        if (env_config_.count("required")) {
            options_.required_envs = env_config_.at("required");
        }

        if (env_config_.count("execute")) {
            command_ = env_config_.at("execute");
            std::stringstream command_iss(command_);
            std::string arg;

            while (command_iss >> arg) {
                char *arg_cstr = new char[arg.length() + 1];
                std::strcpy(arg_cstr, arg.c_str());
                options_.commands.push_back(arg_cstr);
            }

            options_.commands.push_back(nullptr);
        }

        if (options_.debug) {
            log(constants::CONFIG_DEBUG);
        }
    };

    const options &config::get_options() const noexcept { return options_; }

    void config::log(uint8_t code) const noexcept {
        switch (code) {
        case constants::CONFIG_FILE_ERROR: {
            std::cerr
                << fmt::format(
                       "[nvi] (config::FILE_ERROR) Unable to locate \"%s\". The configuration file doesn't appear "
                       "to exist!",
                       file_path_.c_str())
                << std::endl;
            break;
        }
        case constants::CONFIG_FILE_PARSE_ERROR: {
            std::cerr
                << fmt::format(
                       "[nvi] (config::FILE_PARSE_ERROR) Unable to load a \"%s\" environment from the "
                       "env.config.json configuration file (%s). The specified environment doesn't appear to exist!",
                       env_.c_str(), file_path_.c_str())
                << std::endl;
            break;
        }
        case constants::CONFIG_DEBUG: {
            const std::string last_key = std::prev(parsed_config_.end()).key();
            std::string keys;
            for (auto &el : parsed_config_.items()) {
                const std::string comma = el.key() != last_key ? ", " : "";
                keys += el.key() + comma;
            }

            std::clog << fmt::format(
                             "[nvi] (config::DEBUG) Parsed the following keys from the env.config.json configuration "
                             "file: \"%s\" and selected the \"%s\" configuration.",
                             keys.c_str(), env_.c_str())
                      << std::endl;

            std::clog << fmt::format("[nvi] (config::DEBUG) The following flags were set: "
                                     "debug=\"true\", dir=\"%s\", execute=\"%s\", files=\"%s\", required=\"%s\".\n",
                                     options_.dir.c_str(), command_.c_str(), fmt::join(options_.files, ", ").c_str(),
                                     fmt::join(options_.required_envs, ", ").c_str())
                      << std::endl;
            break;
        }
        case constants::CONFIG_MISSING_FILES_ARG_ERROR: {
            std::cerr << fmt::format(
                             "[nvi] (config::MISSING_FILES_ARG_ERROR) Unable to locate a \"files\" property within the "
                             "\"%s\" environment configuration (%s). You must specify at least 1 .env file to load!",
                             env_.c_str(), file_path_.c_str())
                      << std::endl;
            break;
        }
        default:
            break;
        }
    }
} // namespace nvi
