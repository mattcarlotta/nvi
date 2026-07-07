#include "matcher.h"
#include "accessors.h"
#include "chars.h"
#include "dynarr.h"
#include "file.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A env_candidate is a position where an accessor's prefix matched and the
// position-local guards passed. Candidates are collected in one pass per
// accessor,  then merged in file order to reproduce a single-pass scan.
typedef struct {
    size_t pos;
    size_t acc;
} env_candidate_t;

typedef struct {
    env_candidate_t *items;
    size_t count;
    size_t capacity;
} env_candidate_list_t;

// Scan a file's contents for every place an accessor prefix appears, recording
// each valid match as a env_candidate. Jump between occurrences of
// the prefix's first byte rather than checking every offset.
static void find_env_candidates(const file_details_t *file, const file_ext_t *file_ext_match,
                                env_candidate_list_t *env_candidate_list) {
    for (size_t acc_idx = 0; acc_idx < file_ext_match->accessor_count; ++acc_idx) {
        const accessor_t *acc = &file_ext_match->accessors[acc_idx];
        const bool prefix_starts_with_ident = is_ident_char(acc->prefix[0]);
        const bool is_expansion = (acc->pattern == expansion);

        size_t search_start = 0;
        while (search_start + acc->prefix_len <= file->len) {
            const char *match = memchr(file->contents + search_start, acc->prefix[0], file->len - search_start);
            if (match == NULL) {
                break;
            }

            size_t match_pos = (size_t)(match - file->contents);
            // Reject if past file size
            if (match_pos + acc->prefix_len > file->len) {
                break;
            }

            search_start = match_pos + 1;

            // Reject incomplete prefix match
            if (memcmp(file->contents + match_pos, acc->prefix, acc->prefix_len) != 0) {
                continue;
            }

            // Reject matches that begin partway through an identifier (e.g. avoid matching "VAR" inside "MYVAR").
            if (match_pos > 0 && prefix_starts_with_ident && is_ident_char(file->contents[match_pos - 1])) {
                continue;
            }

            // For shell-style expansion, only accept ${...}; treat $${...} as an escaped literal and skip it.
            if (is_expansion && match_pos > 0 && file->contents[match_pos - 1] == DOLLAR_SIGN) {
                continue;
            }

            DYN_ARR_APPEND(env_candidate_list, ((env_candidate_t){.pos = match_pos, .acc = acc_idx}));
        }
    }
}

// order env_candidates by position; ties resolve by accessor array order, which
// mirrors get_accessor's first-match-wins iteration in the old single pass
static int cmp_env_candidates(const void *a, const void *b) {
    const env_candidate_t *ca = a;
    const env_candidate_t *cb = b;

    if (ca->pos != cb->pos) {
        return ca->pos < cb->pos ? -1 : 1;
    }

    if (ca->acc != cb->acc) {
        return ca->acc < cb->acc ? -1 : 1;
    }

    return 0;
}

// advance line accounting from wherever it last stopped up to (but not
// including) 'target', hopping between newlines
static void advance_line(const file_details_t *file, size_t *line_cursor, size_t target, size_t *line,
                         size_t *line_start) {
    while (*line_cursor < target) {
        const char *nl = memchr(file->contents + *line_cursor, LINE_DELIMITER, target - *line_cursor);
        if (nl == NULL) {
            *line_cursor = target;
            break;
        }

        size_t nl_pos = (size_t)(nl - file->contents);
        ++*line;
        *line_start = nl_pos + 1;
        *line_cursor = nl_pos + 1;
    }
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

void scan_file_content(const file_details_t *file, const file_ext_t *file_ext_match,
                       env_key_matches_t *env_key_matches) {
    env_candidate_list_t env_candidates = {0};
    find_env_candidates(file, file_ext_match, &env_candidates);

    if (env_candidates.count == 0) {
        free(env_candidates.items);
        return;
    }

    qsort(env_candidates.items, env_candidates.count, sizeof(*env_candidates.items), cmp_env_candidates);

    // 'cursor' is the position the old single-pass scanner would resume from:
    // env_candidates before it fall inside a consumed region (or a failed match's
    // prefix span) and are discarded, exactly as the byte-walk never visited them
    size_t cursor = 0;
    size_t line = 1;
    size_t line_start = 0;
    size_t line_cursor = 0;

    for (size_t c = 0; c < env_candidates.count; ++c) {
        const env_candidate_t *cand = &env_candidates.items[c];

        if (cand->pos < cursor) {
            continue;
        }

        const accessor_t *acc = &file_ext_match->accessors[cand->acc];

        env_key_t env = extract_env_by_pattern(file, acc->pattern, cand->pos + acc->prefix_len);

        if (!is_valid_key(env.key, env.key_len)) {
            cursor = cand->pos + acc->prefix_len;
            continue;
        }

        advance_line(file, &line_cursor, env.start, &line, &line_start);

        env_key_match_t new_env_key_match = {
            .key = env.key,
            .key_len = env.key_len,
            .line = line,
            .byte = env.start - line_start + 1,
        };

        DYN_ARR_APPEND(env_key_matches, new_env_key_match);

        cursor = env.end;
    }

    free(env_candidates.items);
}
