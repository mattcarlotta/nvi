#ifndef NVI_PARSER_H
#define NVI_PARSER_H

#include "json.cpp"
#include "options.h"
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
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
     * nvi::Options options;
     *
     * options.debug = false;
     *
     * options.dir = "custom/path/to/envs";
     *
     * options.files = {".env", "base.env", ...etc};
     *
     * optons.required_envs = {"KEY1", "KEY2", ...etc};
     *
     * nvi::Parser parser(options);
     *
     * parser.parse_envs()->check_envs()->set_envs();
     *
     * ```
     */
    class Parser {
        public:
            Parser(const Options &options);
            Parser *check_envs();
            Parser *parse_envs() noexcept;
            void set_envs();
            const std::map<std::string, std::string> &get_env_map() const;

        private:
            void log(uint8_t code) const noexcept;
            Parser *read(const std::string &env_file_name);
            Parser *parse();

            const Options _options;
            std::map<std::string, std::string> _env_map;
            std::ifstream _env_file;
            std::string _file;
            std::string_view _file_view;
            std::string _file_name;
            std::string _file_path;
            size_t _byte_count;
            size_t _line_count;
            size_t _val_byte_count;
            int _assignment_index;
            std::string _key;
            std::string _key_prop;
            std::string _value;
            std::vector<std::string> _undefined_keys;
    };
}; // namespace nvi

#endif
