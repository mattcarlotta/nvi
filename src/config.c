#include "config.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "file.h"
#include "utils.h"
#include <stdbool.h>
#include <string.h>

result_t tokenize_config_file(arena_t *arena, const file_details_t *file, config_tokens_t *tokens) {
    size_t i = 0;
    size_t line = 1;

    while (i < file->len) {
        const char c = file->contents[i];

        if (is_token_sep(c)) {
            if (c == LINE_DELIMITER) {
                ++line;
            }
            ++i;
            continue;
        }

        if (c == HASH) {
            while (i < file->len && file->contents[i] != LINE_DELIMITER) {
                ++i;
            }
            continue;
        }

        const size_t start = i;
        while (i < file->len && !is_token_sep(file->contents[i])) {
            ++i;
        }

        const char *token = arena_strndup(arena, file->contents + start, i - start);

        if (strcmp(token, "--") == 0) {
            return usage_error("The '%s' config file may not contain a '--' command delimiter (%s:%zu)", file->path,
                               file->path, line);
        }

        if (token[0] == AT_SIGN) {
            return usage_error("The '%s' config file may not reference another config file '%s' (%s:%zu)", file->path,
                               token, file->path, line);
        }

        if (tokens->count == CONFIG_MAX_TOKENS) {
            return usage_error("The '%s' config file exceeds %d tokens", file->path, CONFIG_MAX_TOKENS);
        }

        DYN_ARR_APPEND(arena, tokens, token);
    }

    return RESULT_OK;
}

result_t load_config_file(arena_t *arena, int argc, const char **argv, config_t *config) {
    config->argc = argc;
    config->argv = argv;

    int config_index = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--") == 0) {
            break;
        }

        if (argv[i][0] != AT_SIGN) {
            continue;
        }

        if (config_index != 0) {
            return usage_error("Only one '@<path>' config file may be provided (found '%s' and '%s')",
                               argv[config_index], argv[i]);
        }

        config_index = i;
    }

    if (config_index == 0) {
        return RESULT_OK;
    }

    config->path = argv[config_index] + 1;

    if (config->path[0] == NULL_CHAR) {
        return usage_error("The '@' config file argument requires a path (eg. '@.nvi')");
    }

    // .nvi, .nvi.staging, project.nvi
    if (!has_dotfile_ext(path_basename(config->path), ".nvi")) {
        return operation_error("The config file '%s' is an invalid .nvi file (missing '.nvi' extension)\n",
                               config->path);
    }

    if (is_absolute_path(config->path)) {
        return operation_error("The config file '%s' must be relative to the current directory\n", config->path);
    }

    if (path_escapes_cwd(config->path)) {
        return operation_error("The config file '%s' may not escape the current directory\n", config->path);
    }

    const file_details_t file = open_file(arena, config->path);
    if (file.contents == NULL) {
        return OPERATION_FAILURE;
    }

    config_tokens_t tokens = {0};
    const result_t result = tokenize_config_file(arena, &file, &tokens);
    if (!result.ok) {
        return result;
    }

    // splice the tokens into argv where the '@<path>' argument sat; bounded by
    // argc + CONFIG_MAX_TOKENS, so no overflow concerns
    const size_t merged_count = (size_t)argc - 1 + tokens.count;
    const char **merged = arena_alloc(arena, merged_count * sizeof(const char *));

    size_t n = 0;
    for (int i = 0; i < config_index; ++i) {
        merged[n++] = argv[i];
    }
    for (size_t t = 0; t < tokens.count; ++t) {
        merged[n++] = tokens.items[t];
    }
    for (int i = config_index + 1; i < argc; ++i) {
        merged[n++] = argv[i];
    }

    config->argc = (int)merged_count;
    config->argv = merged;

    return RESULT_OK;
}
