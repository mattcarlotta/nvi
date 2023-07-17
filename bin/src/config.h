#ifndef NVI_ENV_CONFIG_H
#define NVI_ENV_CONFIG_H

#include "json.cpp"
#include <fstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
/** @class
 * Reads an env.config.json configuration file from the project root directory or a specified directory.
 *
 * @param `env` contains a pointer to the name of the environment to load from the configuration file.
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
 * nvi::config config(&env, dir);
 *
 * ```
 */
class config {
    public:
    bool debug = false;
    string dir;
    vector<string> files;
    vector<string> required_envs = vector<string>();
    string command;
    vector<char *> commands;
    config(const string *environment, const string env_dir = "");

    private:
    /**
     * Logs error/warning/debug config details.
     */
    void log(unsigned int code) const;

    private:
    const string *env;
    string file_path;
    nlohmann::json parsed_config;
    nlohmann::json::object_t env_config;
};
}; // namespace nvi

#endif
