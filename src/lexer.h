#ifndef NVI_ENV_LEXER_H
#define NVI_ENV_LEXER_H

#include "options.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace nvi {
    //
    enum class ValueType { normal, interpolated, multiline };

    struct ValueToken {
            ValueType type;
            std::optional<std::string> value{};
            size_t byte;
            size_t line;
    };

    struct Token {
            std::optional<std::string> key{};
            std::vector<ValueToken> values{};
            std::string file;
    };

    typedef std::vector<Token> tokens_t;

    class Lexer {
        public:
            Lexer(const options_t &options);

            Lexer *read_files() noexcept;
            tokens_t get_tokens() const noexcept;

        private:
            void lex();
            std::optional<char> peek(int offset = 0) const;
            char consume();
            void skip(int offset = 1);

            options_t _options;
            size_t _byte;
            size_t _line;
            std::ifstream _env_file;
            std::string _file;
            std::string _file_name;
            std::filesystem::path _file_path;
            std::vector<Token> _tokens;
    };
}; // namespace nvi
#endif
