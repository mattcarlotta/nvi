#include "parser.h"
#include "arena.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "log.h"
#include "macros.h"
#include "result.h"
#include "tokenizer.h"
#include <stdlib.h>
#include <string.h>

env_t *get_env_from_map(env_map_t *env_map, const char *entry) {
    size_t i = hashmap_get(&env_map->index, entry, strlen(entry));
    if (i == HASHMAP_NOT_FOUND) {
        return NULL;
    }

    return &env_map->items[i];
}

static const char *resolve_env(env_map_t *env_map, const char *key) {
    const char *val = getenv(key);
    if (val != NULL) {
        return val;
    }

    const env_t *entry = get_env_from_map(env_map, key);
    return entry != NULL ? entry->value : NULL;
}

result_t run_parser(arena_t *arena, const args_t *args, const token_list_t *tokens, parser_t *parser) {
    result_t result = RESULT_OK;

    if (args->dry_run) {
        log_info(SINK_STDERR, "[INFO]");
        log_f(SINK_STDERR, " Attempting to parse %zu token%s...\n\n", tokens->count, TO_PLURAL(tokens->count));
    }

    buf_t value = {.arena = arena};

    for (size_t ti = 0; ti < tokens->count; ++ti) {
        const token_t *token = &tokens->items[ti];
        const char *token_key = token->key;

        value.count = 0;

        for (size_t vt = 0; vt < token->values.count; ++vt) {
            const value_token_t *value_token = &token->values.items[vt];

            switch (value_token->kind) {
                case INTERPOLATED_KEY: {
                    const char *raw_value = value_token->value;
                    size_t raw_value_len = value_token->value_len;

                    // POSIX-style ${KEY:-default}: substitute the default when KEY is unset or empty
                    size_t key_len = raw_value_len;
                    const char *fallback = NULL;
                    size_t fallback_len = 0;
                    for (size_t k = 0; k + 1 < raw_value_len; ++k) {
                        if (raw_value[k] == COLON && raw_value[k + 1] == DASH) {
                            key_len = k;
                            fallback = raw_value + k + 2;
                            fallback_len = raw_value_len - k - 2;
                            break;
                        }
                    }

                    const char *lookup_key = raw_value;
                    if (key_len != raw_value_len) {
                        lookup_key = arena_strndup(arena, raw_value, key_len);
                    }

                    const char *env = resolve_env(&parser->env_map, lookup_key);

                    if (env == NULL && fallback == NULL) {
                        result = operation_error(
                            "The '%s' key contains an interpolated key variable %.*s (%s:%zu:%zu) that is not "
                            "defined.\n",
                            token_key ? token_key : "(none)", (int)key_len, raw_value, token->file, value_token->line,
                            value_token->byte);
                        goto done;
                    }

                    if (env != NULL && env[0] != '\0') {
                        DYN_ARR_APPEND_MANY(arena, &value, env, strlen(env));
                    } else if (fallback != NULL) {
                        DYN_ARR_APPEND_MANY(arena, &value, fallback, fallback_len);
                    }
                    break;
                }
                case COMMENTED_LINE: {
                    if (args->dry_run) {
                        log_info(SINK_STDERR, "[INFO]");
                        log_f(SINK_STDERR, " Skipping a parsed comment in Token #%zu...\n    \u2022 ", ti + 1);
                        log_comment(SINK_STDERR, "%s\n\n", value_token->value);
                    }
                    break;
                }
                default: {
                    DYN_ARR_APPEND_MANY(arena, &value, value_token->value, value_token->value_len);
                    break;
                }
            }
        }

        // skip storing comments
        if (token_key == NULL) {
            continue;
        }

        char *env_value = arena_alloc(arena, value.count + 1);

        if (value.count > 0) {
            memcpy(env_value, value.items, value.count);
        }
        env_value[value.count] = '\0';

        env_t *existing = get_env_from_map(&parser->env_map, token_key);
        if (existing != NULL) {
            if (args->dry_run) {
                log_info(SINK_STDERR, "[INFO]");
                log_f(SINK_STDERR, " Token #%zu updated ", ti + 1);
                log_bold_info(SINK_STDERR, "%s", token_key);
                log_f(SINK_STDERR, " key's value from ");
                log_info(SINK_STDERR, "%s", existing->value);
                log_f(SINK_STDERR, " to ");
                log_info(SINK_STDERR, "%s", env_value);
                log_f(SINK_STDERR, "...\n\n");
            }
            // the previous value is abandoned in the main arena
            existing->value = env_value;
        } else {
            env_t new_env = {.key = token_key, .value = env_value};
            DYN_ARR_APPEND(arena, &parser->env_map, new_env);
            hashmap_append(arena, &parser->env_map.index, token_key, strlen(token_key), parser->env_map.count - 1);
        }

        if (args->dry_run) {
            log_info(SINK_STDERR, "[INFO]");
            log_f(SINK_STDERR, " Successfully parsed Token #%zu...\n", ti + 1);
            log_f(SINK_STDERR, "    \u2022 key: ");
            log_bold_info(SINK_STDERR, "%s \n", token_key);
            log_f(SINK_STDERR, "    \u2022 value: ");
            log_info(SINK_STDERR, "%s", env_value);
            log_f(SINK_STDERR, "\n\n");
        }
    }

    if (parser->env_map.count == 0) {
        result = operation_error("After parsing .env tokens, there aren't any ENVs to emit; aborting.\n\n");
        goto done;
    }

    for (size_t i = 0; i < args->required.count; ++i) {
        const char *required_key = args->required.items[i];
        const env_t *entry = get_env_from_map(&parser->env_map, required_key);
        if (entry == NULL || entry->value[0] == '\0') {
            DYN_ARR_APPEND(arena, &parser->missing_envs, required_key);
        }
    }

    if (args->dry_run) {
        log_info(SINK_STDERR, "[INFO]");
        log_f(SINK_STDERR, " The following %zu ENV%s were parsed and will be emitted to stdout... \n",
              parser->env_map.count, TO_PLURAL(parser->env_map.count));
        for (size_t i = 0; i < parser->env_map.count; ++i) {
            const env_t env = parser->env_map.items[i];
            log_f(SINK_STDERR, "    \u2022 ");
            log_bold_info(SINK_STDERR, "%s=%s\n", env.key, env.value);
        }
        log_f(SINK_STDERR, "\n");
        goto done;
    }

    if (args->command.count > 0 && parser->missing_envs.count > 0) {
        log_error(SINK_STDERR,
                  "[ERROR] The following ENV keys were marked as required, but are undefined or empty after parsing:");
        for (size_t i = 0; i < parser->missing_envs.count; ++i) {
            log_error(SINK_STDERR, "\n   \u2022 %s", parser->missing_envs.items[i]);
        }
        log_error(SINK_STDERR, "\n");
        result = OPERATION_FAILURE;
    }

done:
    return result;
}
