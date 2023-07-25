#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "options.h"
#include <string>
#include <string_view>
#include <vector>

namespace nvi {
    /**
     * @detail Reads an `.nvi` configuration file from the project root directory or a specified directory and
     * converts the selected environment into `options`.
     *
     * @param `env` contains the name of the environment to load from the configuration file.
     *
     * @param `dir` is an optional argument to specify where the .nvi file resides according to current directory
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
     * nvi::options_t options = config.get_options();
     *
     * ```
     */
    class Config {
        public:
            Config(const std::string &environment, const std::string env_dir = "");
            const options_t &get_options() const noexcept;

        private:
            const std::string_view trim_surrounding_spaces(const std::string_view &val) noexcept;
            const std::vector<std::string> parse_vector_arg(const unsigned int &code) const;
            const std::string_view parse_string_arg(const unsigned int &code) const;
            void log(const unsigned int &code) const noexcept;

            options_t _options;
            std::string _file;
            std::string_view _file_view;
            std::string _command;
            const std::string _env;
            std::string _file_path;
            std::string_view _key;
            std::string_view _value;
    };
}; // namespace nvi

#endif
