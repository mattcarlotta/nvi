#ifndef NVI_ENV_LEXER_H
#define NVI_ENV_LEXER_H

#include "log.h"
#include "options.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace nvi {
    enum class ValueType { normal, interpolated, multiline, unknown };

    struct ValueToken {
        ValueType type;
        std::optional<std::string> value{};
        size_t byte = 0;
        size_t line = 0;
    };

    struct Token {
        std::optional<std::string> key = "";
        std::vector<ValueToken> values{};
        std::string file;
    };

    typedef std::vector<Token> tokens_t;

    /**
     * @detail Lexes one or many .env files into tokens.
     * @param `options` initialize parser with the following required option: `files`, followed by optional options:
     * `api`, `debug`, `dir`, `environment`, `project` and `required_envs`.
     * @example Initializing a lexer and lexing .env files
     *
     * nvi::options_t options;
     * options.debug = false;
     * options.dir = "custom/path/to/envs";
     * options.files = {".env", "base.env", ...etc};
     * optons.required_envs = {"KEY1", "KEY2", ...etc};
     * nvi::Lexer lexer(&options);
     * lexer.parse_files();
     */
    class Lexer {
        public:
        Lexer(const options_t &options);
        Lexer *parse_api_response(std::string &&envs) noexcept;
        Lexer *parse_files() noexcept;
        tokens_t get_tokens() const noexcept;

        private:
        void parse_file() noexcept;
        std::optional<char> peek(int offset = 0) const noexcept;
        char commit() noexcept;
        void skip(int offset = 1) noexcept;
        void log(const messages_t &code) const noexcept;
        std::string get_value_type_string(const ValueType &vt) const noexcept;
        void reset_byte_count() noexcept;
        void increase_byte_count(size_t offset = 1) noexcept;
        void increase_line_count() noexcept;

        const options_t &_options;
        size_t _index;
        size_t _byte;
        size_t _line;
        std::ifstream _env_file;
        std::string _file;
        std::string _file_name;
        std::filesystem::path _file_path;
        std::vector<Token> _tokens;
        std::string _token_key;
    };
}; // namespace nvi
#endif
