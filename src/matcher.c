#include "matcher.h"
#include "accessors.h"
#include "chars.h"
#include "dynarr.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

static const accessor_t *match_prefix(const char *contents, size_t len, size_t i, const accessor_t *accessors,
                                      size_t accessor_count) {
    for (size_t a = 0; a < accessor_count; ++a) {
        const accessor_t *acc = &accessors[a];
        size_t plen = strlen(acc->prefix);

        if (i + plen > len || strncmp(contents + i, acc->prefix, plen) != 0) {
            continue;
        }

        // reject mid-identifier matches
        if (i > 0 && is_ident_char(contents[i - 1]) && is_ident_char(acc->prefix[0])) {
            continue;
        }

        return acc;
    }
    return NULL;
}

static env_t extract_key(const char *contents, size_t len, size_t start, pattern_t kind) {
    switch (kind) {
        case ident: {
            size_t end = start;

            while (end < len && is_ident_char(contents[end])) {
                ++end;
            }

            if (end == start) {
                return (env_t){0};
            }

            return (env_t){.key = contents + start, .key_len = end - start, .start = start, .end = end};
        }
        case quoted: {
            size_t cursor = start;
            while (cursor < len && (contents[cursor] == SPACE || contents[cursor] == TAB)) {
                ++cursor;
            }

            if (cursor >= len) {
                return (env_t){0};
            }

            char quote = contents[cursor];
            if (quote != DOUBLE_QUOTE && quote != SINGLE_QUOTE) {
                return (env_t){0};
            }

            size_t key_start = cursor + 1;
            if (key_start >= len || !is_ident_start(contents[key_start])) {
                return (env_t){0};
            }

            size_t key_end = index_of(contents, len, key_start, quote);
            if (key_end == len || !is_same_line(contents, len, key_start, key_end) || key_end == key_start) {
                return (env_t){0};
            }

            return (env_t){
                .key = contents + key_start, .key_len = key_end - key_start, .start = key_start, .end = key_end + 1};
        }
        case braced: {
            size_t brace = index_of(contents, len, start, CLOSE_BRACE);
            if (brace == len || !is_same_line(contents, len, start, brace)) {
                return (env_t){0};
            }

            size_t key_start = start;
            size_t key_end = brace;

            // strip a matching pair of surrounding quotes: $ENV{'FOO'} -> FOO
            if (key_end > key_start && (contents[key_start] == SINGLE_QUOTE || contents[key_start] == DOUBLE_QUOTE) &&
                contents[key_end - 1] == contents[key_start]) {
                ++key_start;
                --key_end;
            }

            if (key_end <= key_start) {
                return (env_t){0};
            }

            return (env_t){
                .key = contents + key_start, .key_len = key_end - key_start, .start = key_start, .end = brace + 1};
        }
        case parened: {
            size_t paren = index_of(contents, len, start, CLOSE_PAREN);
            if (paren == len || !is_same_line(contents, len, start, paren) || paren == start) {
                return (env_t){0};
            }

            return (env_t){.key = contents + start, .key_len = paren - start, .start = start, .end = paren + 1};
        }
    }
}

void scan_file_content(const char *contents, size_t len, const accessor_t *accessors, size_t accessor_count,
                       env_matches_t *matches) {
    size_t i = 0;
    size_t line = 1;
    size_t line_start = 0;

    while (i < len) {
        if (contents[i] == LINE_DELIMITER) {
            ++line;
            line_start = i + 1;
            ++i;
            continue;
        }

        const accessor_t *acc = match_prefix(contents, len, i, accessors, accessor_count);
        if (acc == NULL) {
            ++i;
            continue;
        }

        size_t prefix_len = strlen(acc->prefix);
        env_t env = extract_key(contents, len, i + prefix_len, acc->pattern);

        bool valid_key = is_valid_key(env.key, env.key_len);
        if (!valid_key) {
            i += prefix_len;
            continue;
        }

        env_match_t m = {
            .key = env.key,
            .key_len = env.key_len,
            .line = line,
            .byte = env.start - line_start + 1,
        };

        DYN_ARR_APPEND(matches, m);

        i = env.end;
    }
}
