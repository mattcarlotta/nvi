#ifndef NVI_PARSER_H
#define NVI_PARSER_H

#include "lexer.h"
#include "log.h"
#include "options.h"
#include <cstddef>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace nvi {
    typedef std::map<std::string, std::string> env_map_t;

    class Parser {
        public:
            Parser(const tokens_t &tokens, const options_t &options);
            Parser *check_envs() noexcept;
            const env_map_t &get_env_map() const noexcept;
            Parser *parse_tokens() noexcept;

        private:
            void log(const messages_t &code) const noexcept;

            tokens_t _tokens;
            const options_t _options;
            Token _token;
            ValueToken _value_token;
            std::string _key;
            std::string _interp_key;
            std::string _value;
            std::vector<std::string> _undefined_keys;
            env_map_t _env_map;
    };
}; // namespace nvi
#endif
