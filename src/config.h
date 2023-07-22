#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "json.cpp"
#include "options.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace nvi {
    /**
     * @detail Reads an `nvi.json` configuration file from the project root directory or a specified directory and
     * converts the selected environment into `options`.
     *
     * @param `env` contains the name of the environment to load from the configuration file.
     *
     * @param `dir` is an optional argument to specify where the nvi.json resides according to current directory.
     *
     * @example Initializing a config
     * ```
     *
     * const string::string env = "development";
     *
     * const std::string dir = "custom/path/to/envs";
     *
     * nvi::Config config(env, dir);
     *
     * nvi::Options options = config.get_options();
     *
     * ```
     */
    class Config {
        public:
            Config(const std::string &environment, const std::string env_dir = "");
            const Options &get_options() const noexcept;

        private:
            void log(const uint_least8_t &code) const noexcept;

            Options _options;
            std::string _command;
            const std::string _env;
            std::string _file_path;
            nlohmann::json _parsed_config;
    };
}; // namespace nvi

#endif
