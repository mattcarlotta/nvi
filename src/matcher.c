#include "matcher.h"
#include "accessors.h"
#include "arena.h"
#include "chars.h"
#include "dynarr.h"
#include "file.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

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
        case expansion: {
            size_t end = start;
            while (end < file->len && is_ident_char(file->contents[end])) {
                ++end;
            }

            if (end == start) {
                return (env_key_t){0};
            }

            size_t brace = index_of(file, end, CLOSE_BRACE);
            if (brace == file->len || !is_same_line(file, start, brace)) {
                return (env_key_t){0};
            }

            return (env_key_t){.key = file->contents + start, .key_len = end - start, .start = start, .end = end};
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

void scan_file_content(arena_t *scratch, const file_details_t *file, const file_ext_t *file_ext_match,
                       env_key_matches_t *env_key_matches) {
    for (size_t acc_idx = 0; acc_idx < file_ext_match->accessor_count; ++acc_idx) {
        const accessor_t *acc = &file_ext_match->accessors[acc_idx];
        const bool prefix_starts_with_ident = is_ident_char(acc->prefix[0]);
        const bool is_expansion = (acc->pattern == expansion);

        size_t line = 1;
        size_t line_start = 0;
        size_t line_cursor = 0;
        size_t search_start = 0;

        while (search_start + acc->prefix_len <= file->len) {
            const char *match = memchr(file->contents + search_start, acc->prefix[0], file->len - search_start);
            if (match == NULL) {
                break;
            }

            size_t match_pos = (size_t)(match - file->contents);
            if (match_pos + acc->prefix_len > file->len) {
                break;
            }

            search_start = match_pos + 1;

            // reject incomplete prefix match
            if (memcmp(file->contents + match_pos, acc->prefix, acc->prefix_len) != 0) {
                continue;
            }

            // reject matches that begin partway through an identifier (e.g. avoid matching "VAR" inside "MYVAR").
            if (match_pos > 0 && prefix_starts_with_ident && is_ident_char(file->contents[match_pos - 1])) {
                continue;
            }

            // reject $$+{...} (accepts only ${})
            if (is_expansion && match_pos > 0 && file->contents[match_pos - 1] == DOLLAR_SIGN) {
                continue;
            }

            env_key_t env = extract_env_by_pattern(file, acc->pattern, match_pos + acc->prefix_len);
            if (!is_valid_key(env.key, env.key_len)) {
                continue;
            }

            // advance line from wherever it last stopped up to the start of the env
            // NOTE: may be worth removing as this is purely for dry-run output; this would
            // reduce overhead for an env_key_match by just storing the key and its length
            while (line_cursor < env.start) {
                const char *nl = memchr(file->contents + line_cursor, LINE_DELIMITER, env.start - line_cursor);
                if (nl == NULL) {
                    line_cursor = env.start;
                    break;
                }
                size_t nl_pos = (size_t)(nl - file->contents);
                ++line;
                line_start = nl_pos + 1;
                line_cursor = nl_pos + 1;
            }

            env_key_match_t new_env_key_match = {
                .key = env.key,
                .key_len = env.key_len,
                .line = line,
                .byte = env.start - line_start + 1,
            };
            DYN_ARR_APPEND(scratch, env_key_matches, new_env_key_match);

            // resume past the extracted span; every accessor prefix contains at least one
            // non-identifier byte, so no prefix can begin inside the matched key
            search_start = env.end;
        }
    }
}
