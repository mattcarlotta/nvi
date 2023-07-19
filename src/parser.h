#ifndef NVI_PARSER_H
#define NVI_PARSER_H

#include "json.cpp"
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace nvi {
/** @class
 * Parses and interpolates an .env file to a single object of `"key": "value"` pairs.
 *
 * @param `options` initialize parser with the following required option: `files`, folled by optional options: `dir`,
 * `required_envs`, and `debug`.
 *
 * @example Initializing a parser
 * ```
 *
 * std::vector<std::string> = {".env", "base.env", ...etc};
 *
 * std::string dir = "custom/path/to/envs";
 *
 * std::vector<std::string> required_envs = {"KEY1", "KEY2", ...etc};
 *
 * bool debug = false;
 *
 * nvi::parser parser(&files, dir, &required_envs, debug);
 *
 * ```
 */
class parser {
    public:
    std::map<string, string> env_map;
    parser(const vector<string> *files, const std::optional<string> &dir, const vector<string> *required_envs = nullptr,
           const bool &debug = false);
    /**
     * Uses the `required_envs` optional option to check for specified ENVs to be defined after all `files` are parsed.
     */
    void check_envs();
    /**
     * Iterates over `files` and hands them over to the `read` and `parser` methods for processing.
     */
    parser *parse_envs() noexcept;
    /**
     * Sets parsed ENVs to the parent process, which will be shared with any spawned child processes.
     */
    void set_envs();
    /**
     * Prints stringied JSON of ENVs to `std::cout`.
     */
    void print_envs();

    private:
    /**
     * Logs error/warning/debug parser details.
     */
    void log(unsigned int code) const noexcept;
    /**
     * Reads an .env file and/or resets the loaded file.
     *
     * @param `env_file_name` the name of the .env file to read using any initialized options.
     *
     * @return an instance of an initialized parser.
     */
    parser *read(const string &env_file_name);
    /**
     * Parses and interpolates one .env file at a time and saves results to a private `env_map` field for
     * cross-referencing with other .env files.
     *
     * @return an instance of an initialized parser.
     */
    parser *parse();

    private:
    const vector<string> *files;
    string dir;
    bool debug;
    const vector<string> *required_envs;
    std::ifstream env_file;
    string loaded_file;
    string file_name;
    string file_path;
    size_t file_length;
    size_t byte_count;
    size_t line_count;
    size_t val_byte_count;
    int assignment_index;
    string key;
    string key_prop;
    string value;
    vector<string> undefined_keys;
};
}; // namespace nvi

#endif
