#include "config.h"
#include "format.h"
#include "json.cpp"
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace nvi {

    enum MESSAGES {
        FILE_ERROR = 0,
        FILE_PARSE_ERROR = 1,
        DEBUG = 2,
        MISSING_FILES_ARG_ERROR = 3,
    };

    Config::Config(const std::string &environment, const std::string _envdir) : _env(environment) {
        _file_path = std::string(std::filesystem::current_path() / _envdir / "nvi.json");
        if (not std::filesystem::exists(_file_path)) {
            log(MESSAGES::FILE_ERROR);
            std::exit(1);
        }

        std::ifstream env_configfile(_file_path, std::ios_base::in);
        _parsed_config = nlohmann::json::parse(env_configfile);
        if (not _parsed_config.count(_env)) {
            log(MESSAGES::FILE_PARSE_ERROR);
            std::exit(1);
        }

        nlohmann::json::object_t env_config = _parsed_config.at(_env);

        if (env_config.count("files")) {
            _options.files = env_config.at("files");
        } else {
            log(MESSAGES::MISSING_FILES_ARG_ERROR);
            std::exit(1);
        }

        if (env_config.count("debug")) {
            _options.debug = env_config.at("debug");
        }

        if (env_config.count("dir")) {
            _options.dir = env_config.at("dir");
        }

        if (env_config.count("required")) {
            _options.required_envs = env_config.at("required");
        }

        if (env_config.count("execute")) {
            _command = env_config.at("execute");
            std::stringstream _commandiss(_command);
            std::string arg;

            while (_commandiss >> arg) {
                char *arg_cstr = new char[arg.length() + 1];
                std::strcpy(arg_cstr, arg.c_str());
                _options.commands.push_back(arg_cstr);
            }

            _options.commands.push_back(nullptr);
        }

        if (_options.debug) {
            log(MESSAGES::DEBUG);
        }
    };

    const Options &Config::get_options() const noexcept { return _options; }

    void Config::log(const uint_least8_t &code) const noexcept {
        switch (code) {
        case MESSAGES::FILE_ERROR: {
            std::cerr
                << fmt::format(
                       "[nvi] (config::FILE_ERROR) Unable to locate \"%s\". The configuration file doesn't appear "
                       "to exist!",
                       _file_path.c_str())
                << std::endl;
            break;
        }
        case MESSAGES::FILE_PARSE_ERROR: {
            std::cerr << fmt::format(
                             "[nvi] (config::FILE_PARSE_ERROR) Unable to load a \"%s\" environment from the "
                             "nvi.json configuration file (%s). The specified environment doesn't appear to exist!",
                             _env.c_str(), _file_path.c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::DEBUG: {
            const std::string last_key = std::prev(_parsed_config.end()).key();
            std::string keys;
            for (auto &el : _parsed_config.items()) {
                const std::string comma = el.key() != last_key ? ", " : "";
                keys += el.key() + comma;
            }

            std::clog << fmt::format("[nvi] (config::DEBUG) Parsed the following keys from the nvi.json configuration "
                                     "file: \"%s\" and selected the \"%s\" configuration.",
                                     keys.c_str(), _env.c_str())
                      << std::endl;

            std::clog << fmt::format("[nvi] (config::DEBUG) The following flags were set: "
                                     "debug=\"true\", dir=\"%s\", execute=\"%s\", files=\"%s\", required=\"%s\".\n",
                                     _options.dir.c_str(), _command.c_str(), fmt::join(_options.files, ", ").c_str(),
                                     fmt::join(_options.required_envs, ", ").c_str())
                      << std::endl;
            break;
        }
        case MESSAGES::MISSING_FILES_ARG_ERROR: {
            std::cerr << fmt::format(
                             "[nvi] (config::MISSING_FILES_ARG_ERROR) Unable to locate a \"files\" property within the "
                             "\"%s\" environment configuration (%s). You must specify at least 1 .env file to load!",
                             _env.c_str(), _file_path.c_str())
                      << std::endl;
            break;
        }
        default:
            break;
        }
    }
} // namespace nvi
