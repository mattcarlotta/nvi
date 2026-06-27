#include "matcher.h"
#include "accessors.h"
#include "chars.h"
#include "dynarr.h"
#include "file.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

static inline const accessor_t *get_accessor(const file_details_t *file, const file_ext_t *file_ext_match, size_t i) {
    for (size_t a = 0; a < file_ext_match->accessor_count; ++a) {
        const accessor_t *acc = &file_ext_match->accessors[a];
        size_t prefix_len = strlen(acc->prefix);

        if (i + prefix_len > file->len || strncmp(file->contents + i, acc->prefix, prefix_len) != 0) {
            continue;
        }

        // reject mid-identifier matches
        if (i > 0 && is_ident_char(file->contents[i - 1]) && is_ident_char(acc->prefix[0])) {
            continue;
        }

        return acc;
    }

    return NULL;
}

static env_key_t extract_env_by_pattern(const file_details_t *file, pattern_t kind, size_t start) {
    switch (kind) {
        case ident: {
            size_t end = start;
            while (end < file->len && is_ident_char(file->contents[end])) {
                ++end;
            }

            if (end == start) {
                return (env_key_t){0};
            }

            return (env_key_t){.key = file->contents + start, .key_len = end - start, .start = start, .end = end};
        }
        case quoted: {
            size_t cursor = start;
            while (cursor < file->len && (file->contents[cursor] == SPACE || file->contents[cursor] == TAB)) {
                ++cursor;
            }

            if (cursor >= file->len) {
                return (env_key_t){0};
            }

            char quote = file->contents[cursor];
            if (quote != DOUBLE_QUOTE && quote != SINGLE_QUOTE) {
                return (env_key_t){0};
            }

            size_t key_start = cursor + 1;
            if (key_start >= file->len || !is_ident_start(file->contents[key_start])) {
                return (env_key_t){0};
            }

            size_t key_end = index_of(file, key_start, quote);
            if (key_end == file->len || !is_same_line(file, key_start, key_end) || key_end == key_start) {
                return (env_key_t){0};
            }

            return (env_key_t){.key = file->contents + key_start,
                               .key_len = key_end - key_start,
                               .start = key_start,
                               .end = key_end + 1};
        }
        case braced: {
            size_t brace = index_of(file, start, CLOSE_BRACE);
            if (brace == file->len || !is_same_line(file, start, brace)) {
                return (env_key_t){0};
            }

            size_t key_start = start;
            size_t key_end = brace;

            // strip a matching pair of surrounding quotes: $ENV{'FOO'} -> FOO
            if (key_end > key_start &&
                (file->contents[key_start] == SINGLE_QUOTE || file->contents[key_start] == DOUBLE_QUOTE) &&
                file->contents[key_end - 1] == file->contents[key_start]) {
                ++key_start;
                --key_end;
            }

            if (key_end <= key_start) {
                return (env_key_t){0};
            }

            return (env_key_t){.key = file->contents + key_start,
                               .key_len = key_end - key_start,
                               .start = key_start,
                               .end = brace + 1};
        }
        case parened: {
            size_t paren = index_of(file, start, CLOSE_PAREN);
            if (paren == file->len || !is_same_line(file, start, paren) || paren == start) {
                return (env_key_t){0};
            }

            return (env_key_t){
                .key = file->contents + start, .key_len = paren - start, .start = start, .end = paren + 1};
        }
        default:
            return (env_key_t){0};
    }
}

void scan_file_content(const file_details_t *file, const file_ext_t *file_ext_match,
                       env_key_matches_t *env_key_matches) {
    size_t i = 0;
    size_t line = 1;
    size_t line_start = 0;

    while (i < file->len) {
        // skip to next line
        if (file->contents[i] == LINE_DELIMITER) {
            ++line;
            line_start = i + 1;
            ++i;
            continue;
        }

        const accessor_t *acc = get_accessor(file, file_ext_match, i);
        if (acc == NULL) {
            ++i;
            continue;
        }

        size_t prefix_len = strlen(acc->prefix);
        env_key_t env = extract_env_by_pattern(file, acc->pattern, i + prefix_len);

        bool valid_key = is_valid_key(env.key, env.key_len);
        if (!valid_key) {
            i += prefix_len;
            continue;
        }

        env_key_match_t found_env_key_match = {
            .key = env.key,
            .key_len = env.key_len,
            .line = line,
            .byte = env.start - line_start + 1,
        };

        DYN_ARR_APPEND(env_key_matches, found_env_key_match);

        i = env.end;
    }
}
