#include "parser.h"
#include "format.h"
#include "lexer.h"
#include "log.h"
#include "options.h"
#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>

namespace nvi {
    inline constexpr char NULL_CHAR = '\0'; // 0x00

    Parser::Parser(const tokens_t &&tokens, options_t &options)
        : _tokens(std::move(tokens)), _options(options),
          logger(LOGGER::PARSER, _options, _key, _token, _value_token, _interp_key, _value) {}

    const env_map_t &Parser::get_env_map() const noexcept { return _env_map; }

    Parser *Parser::parse_tokens() noexcept {
        if (!_tokens.size()) {
            logger.fatal(EMPTY_ENVS_ERROR);
        }

        for (const Token &t : _tokens) {
            _token = t;
            _key.clear();
            _interp_key.clear();
            _value.clear();

            if (_token.values.size()) {
                for (const ValueToken &vt : _token.values) {
                    _value_token = vt;
                    _key = _token.key.value();
                    if (_value_token.type == ValueType::interpolated) {
                        _interp_key = _value_token.value.has_value() ? _value_token.value.value() : std::string{};
                        const char *val_from_env = std::getenv(_interp_key.c_str());

                        if (val_from_env != nullptr && *val_from_env != NULL_CHAR) {
                            _value += val_from_env;
                        } else if (_env_map.count(_interp_key)) {
                            _value += _env_map.at(_interp_key);
                        } else if (_options.debug) {
                            logger.debug(INTERPOLATION_WARNING);
                        }
                    } else {
                        _value += _value_token.value.has_value() ? _value_token.value.value() : std::string{};
                    }
                }

                _env_map[_key] = _value;

                // remove keys from required envs list
                if (_options.required_envs.size() && _key.length()) {
                    auto key = std::find(_options.required_envs.begin(), _options.required_envs.end(), _key);

                    if (key != _options.required_envs.end()) {
                        _options.required_envs.erase(key);
                    }
                }

                if (_options.debug) {
                    logger.debug(DEBUG_PARSER);
                }
            }
        }

        if (not _env_map.size()) {
            logger.fatal(EMPTY_ENVS_ERROR);
        } else if (_options.required_envs.size()) {
            logger.fatal(REQUIRED_ENV_ERROR);
        } else if (_options.debug && _options.api) {
            logger.debug(DEBUG_RESPONSE_PROCESSED);
        } else if (_options.debug) {
            logger.debug(DEBUG_FILE_PROCESSED);
        }

        return this;
    }
} // namespace nvi
