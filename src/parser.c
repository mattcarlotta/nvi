#include "parser.h"
#include "dynarr.h"
#include "errors.h"
#include "log.h"
#include "macros.h"
#include "tokenizer.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} value_buf_t;

env_t *get_env_from_map(env_map_t *env_map, const char *entry) {
    size_t i = hashmap_get(&env_map->index, entry, strlen(entry));
    if (i == HASHMAP_NOT_FOUND) {
        return NULL;
    }

    return &env_map->items[i];
}

static inline const char *resolve_env(env_map_t *env_map, const char *key) {
    const char *val = getenv(key);
    if (val != NULL) {
        return val;
    }

    const env_t *entry = get_env_from_map(env_map, key);
    return entry != NULL ? entry->value : NULL;
}

result_t run_parser(const args_t *args, const token_list_t *tokens, env_map_t *env_map) {
    result_t result = {.ok = true, .code = 0};

    if (args->dry_run) {
        log_info("[INFO]");
        log_f(" Attempting to parse %zu token%s...\n\n", tokens->count, TO_PLURAL(tokens->count));
    }

    value_buf_t value = {0};

    for (size_t ti = 0; ti < tokens->count; ++ti) {
        const token_t token = tokens->items[ti];
        const char *token_key = token.key;

        value.count = 0;

        for (size_t vt = 0; vt < token.values.count; ++vt) {
            const value_token_t value_token = token.values.items[vt];

            switch (value_token.kind) {
                case INTERPOLATED: {
                    const char *interpolated_key = value_token.value;
                    const char *env = resolve_env(env_map, interpolated_key);

                    if (env == NULL) {
                        log_warning(
                            "[WARNING] The '%s' key contains an interpolated key variable %s (%s:%zu:%zu) that is "
                            "not defined; skipping.\n",
                            token_key, interpolated_key, token.file, value_token.line, value_token.byte);
                        break;
                    }

                    DYN_ARR_APPEND_MANY(&value, env, strlen(env));
                    break;
                }
                case COMMENTED: {
                    if (args->dry_run) {
                        log_info("[INFO]");
                        log_f(" Skipping a parsed comment in Token #%zu...\n    \u2022 ", ti + 1);
                        log_comment("%s\n\n", value_token.value);
                    }
                    break;
                }
                default: {
                    DYN_ARR_APPEND_MANY(&value, value_token.value, value_token.value_len);
                    break;
                }
            }
        }

        // skip storing comments
        if (token_key == NULL) {
            continue;
        }

        char *owned_value = malloc(value.count + 1);
        if (owned_value == NULL) {
            free(value.items);
            return operation_error("Unable to copy token value (not enough system memory?); aborting.\n");
        }

        if (value.count > 0) {
            memcpy(owned_value, value.items, value.count);
        }
        owned_value[value.count] = '\0';

        env_t *existing = get_env_from_map(env_map, token_key);
        if (existing != NULL) {
            if (args->dry_run) {
                log_info("[INFO]");
                log_f(" Token #%zu updated ", ti + 1);
                log_bold_info("%s", token_key);
                log_f(" key's value from ");
                log_info("%s", existing->value);
                log_f(" to ");
                log_info("%s", owned_value);
                log_f("...\n\n");
            }
            free(existing->value);
            existing->value = owned_value;
        } else {
            env_t new_env = {.key = token_key, .value = owned_value};
            DYN_ARR_APPEND(env_map, new_env);
            hashmap_put(&env_map->index, token_key, strlen(token_key), env_map->count - 1);
        }

        if (args->dry_run) {
            log_info("[INFO]");
            log_f(" Successfully parsed Token #%zu...\n    \u2022 ", ti + 1);
            log_bold_info("%s ", token_key);
            log_info("\u219E %s", owned_value);
            log_f("\n\n");
        }
    }

    free(value.items);
    value.items = NULL;

    if (env_map->count == 0) {
        return operation_error("After parsing .env tokens, there aren't any ENVs to emit; aborting.\n\n");
    }

    list_t missing_envs = {0};
    for (size_t i = 0; i < args->required.count; ++i) {
        const char *required_key = args->required.items[i];
        if (get_env_from_map(env_map, required_key) == NULL) {
            DYN_ARR_APPEND(&missing_envs, required_key);
        }
    }

    if (args->dry_run) {
        log_info("[INFO]");
        log_f(" The following %zu ENV%s were parsed and will be emitted to stdout... \n", env_map->count,
              TO_PLURAL(env_map->count));
        for (size_t i = 0; i < env_map->count; ++i) {
            const env_t env = env_map->items[i];
            log_f("    \u2022 ");
            log_bold_info("%s=%s\n", env.key, env.value);
        }
        log_f("\n");
        goto done;
    }

    if (args->command.count > 0 && missing_envs.count > 0) {
        log_error("[ERROR] The following ENV keys were marked as required, but weren't set after parsing:");
        for (size_t i = 0; i < missing_envs.count; ++i) {
            log_error("\n   \u2022 %s", missing_envs.items[i]);
        }
        log_error("\n");
        result.ok = false;
        result.code = 1;
    }

    goto done;

done:
    free_list(&missing_envs);
    return result;
}

void free_envs(env_map_t *env_map) {
    for (size_t i = 0; i < env_map->count; i++) {
        free(env_map->items[i].value);
    }
    free(env_map->items);
    free_hashmap(&env_map->index);
}
