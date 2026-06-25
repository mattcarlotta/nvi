#include "parser.h"
#include "dynarr.h"
#include "errors.h"
#include "log.h"
#include "macros.h"
#include "tokenizer.h"
#include <stdlib.h>
#include <string.h>

env_t *get_env_from_map(env_map_t *env_map, const char *entry) {
    for (size_t i = 0; i < env_map->count; ++i) {
        const char *key = env_map->items[i].key;
        if (strcmp(entry, key) == 0) {
            return &env_map->items[i];
        }
    }

    return NULL;
}

static const char *resolve_env(env_map_t *env_map, const char *key) {
    const char *val = getenv(key);
    if (val != NULL) {
        return val;
    }

    const env_t *entry = get_env_from_map(env_map, key);
    return entry != NULL ? entry->value : NULL;
}

result_t run_parser(args_t *args, token_list_t *tokens, env_map_t *env_map) {
    result_t result = {.ok = true, .errcode = 0};

    for (size_t ti = 0; ti < tokens->count; ++ti) {
        const token_t token = tokens->items[ti];
        const char *token_key = token.key;

        size_t value_len = 0;
        for (size_t vl = 0; vl < token.values.count; ++vl) {
            if (token.values.items[vl].kind != COMMENTED) {
                value_len += strlen(token.values.items[vl].value);
            }
        }

        char *value = malloc(value_len + 1);
        if (value == NULL) {
            return operation_error("Unable to allocate space for a value (maybe system out of memory?)");
        }

        size_t vi = 0;
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

                    size_t len = strlen(env);
                    memcpy(value + vi, env, len);
                    vi += len;
                    break;
                }
                case COMMENTED: {
                    if (args->dry_run) {
                        log_info("[INFO]");
                        log_f(" Parsed comment in token #%zu (skipping):\n    \u2022 ", ti + 1);
                        log_comment("%s (%s:%zu:%zu)\n\n", value_token.value, token.file, value_token.line,
                                    value_token.byte);
                    }
                    break;
                }
                default: {
                    size_t len = strlen(value_token.value);
                    memcpy(value + vi, value_token.value, len);
                    vi += len;
                    break;
                }
            }
        }

        if (token_key != NULL && value != NULL) {
            value[vi] = '\0';

            char *owned_value = strdup(value);
            if (owned_value == NULL) {
                free(value);
                return operation_error("Unable to copy token key (not enough system memory?); aborting.\n");
            }

            env_t *existing = get_env_from_map(env_map, token_key);
            if (existing != NULL) {
                free(existing->value);
                existing->value = owned_value;
            } else {
                env_t new_env = {.key = token_key, .value = owned_value};
                DYN_ARR_APPEND(env_map, new_env);
            }

            if (args->dry_run) {
                log_info("[INFO]");
                log_f(" Parsed ENV from token #%zu...\n    \u2022 ", ti + 1);
                log_bold_info("%s=%s\n\n", token_key, owned_value);
            }
        }

        free(value);
    }

    if (env_map->count == 0) {
        return operation_error("After parsing .env tokens, there aren't any ENVs to emit; aborting.\n");
    }

    list_t missing_envs = {0};
    for (size_t i = 0; i < args->required.count; ++i) {
        const char *required_key = args->required.items[i];
        if (get_env_from_map(env_map, required_key) == NULL) {
            DYN_ARR_APPEND(&missing_envs, required_key);
        }
    }

    if (missing_envs.count > 0) {
        log_error("[ERROR] The following ENV keys were marked as required, but weren't set after parsing:");
        for (size_t i = 0; i < missing_envs.count; ++i) {
            log_error("\n   \u2022 %s", missing_envs.items[i]);
        }
        log_error("\n");
        result.ok = false;
        result.errcode = 1;
        goto done;
    }

    if (args->dry_run) {
        log_info("[INFO]");
        log_f(" The following %zu ENV%s were parsed and will be emitted to stdout... \n", env_map->count,
              TO_PLURAL(env_map->count));
        for (size_t i = 0; i < env_map->count; ++i) {
            const env_t env = env_map->items[i];
            log_f("    \u2022 %s=%s\n", env.key, env.value);
        }
        log_f("\n");
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
}
