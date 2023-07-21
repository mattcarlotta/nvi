#ifndef NVI_PARSER_H
#define NVI_PARSER_H

#include "json.cpp"
#include "options.h"
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace nvi {
    /** @class
     * Parses and interpolates an .env file to a single object of `"key": "value"` pairs.
     *
     * @param `options` initialize parser with the following required option: `files`, folled by optional options:
     * `dir`, `required_envs`, and `debug`.
     *
     * @example Initializing a parser, parsing .env files, checking ENVs, and setting ENVs
     * ```
     * nvi::options options;
     *
     * options.debug = false;
     *
     * options.dir = "custom/path/to/envs";
     *
     * options.files = {".env", "base.env", ...etc};
     *
     * optons.required_envs = {"KEY1", "KEY2", ...etc};
     *
     * nvi::parser parser(options);
     *
     * parser.parse_envs()->check_envs()->set_envs();
     *
     * ```
     */
    class parser {
        public:
            parser(const options &options);
            parser *check_envs();
            parser *parse_envs() noexcept;
            void set_envs();
            const std::map<std::string, std::string> &get_env_map() const;

        private:
            void log(uint8_t code) const noexcept;
            parser *read(const std::string &env_file_name);
            parser *parse();

            const options options_;
            std::map<std::string, std::string> env_map_;
            std::ifstream env_file_;
            std::string loaded_file_;
            std::string file_name_;
            std::string file_path_;
            size_t file_length_;
            size_t byte_count_;
            size_t line_count_;
            size_t val_byte_count_;
            int assignment_index_;
            std::string key_;
            std::string key_prop_;
            std::string value_;
            std::vector<std::string> undefined_keys_;
    };
}; // namespace nvi

#endif
