#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "json.cpp"
#include "options.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace nvi {
    /** @class
     * Reads an `env.config.json` configuration file from the project root directory or a specified directory and
     * converts the selected environment into `options`.
     *
     * @param `env` contains the name of the environment to load from the configuration file.
     *
     * @param `dir` is an optional argument to specify where the env.config.json resides according to current directory.
     *
     * @example Initializing a config
     * ```
     *
     * const string::string env = "development";
     *
     * const std::string dir = "custom/path/to/envs";
     *
     * nvi::config config(env, dir);
     *
     * nvi::options options = config.get_options();
     *
     * ```
     */
    class config {
        public:
            config(const std::string &environment, const std::string env_dir = "");
            const options &get_options() const noexcept;

        private:
            void log(uint8_t code) const noexcept;

            options options_;
            std::string command_;
            const std::string env_;
            std::string file_path_;
            nlohmann::json parsed_config_;
    };
}; // namespace nvi

#endif
