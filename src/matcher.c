#include "matcher.h"
#include "accessors.h"
#include "chars.h"
#include "dynarr.h"
#include "file.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A candidate is a position where an accessor's prefix matched and the
// position-local guards passed. Candidates are collected in one pass per
// accessor (memchr-driven, so the search skips in SIMD-width chunks), then
// merged in file order to reproduce the single-pass scanner's semantics.
typedef struct {
    size_t pos;
    size_t acc;
} candidate_t;

typedef struct {
    candidate_t *items;
    size_t count;
    size_t capacity;
} candidate_list_t;

static void collect_candidates(const file_details_t *file, const file_ext_t *file_ext_match, candidate_list_t *out) {
    for (size_t a = 0; a < file_ext_match->accessor_count; ++a) {
        const accessor_t *acc = &file_ext_match->accessors[a];
        const bool prefix_is_ident = is_ident_char(acc->prefix[0]);

        size_t cursor = 0;
        while (cursor + acc->prefix_len <= file->len) {
            const char *p = memchr(file->contents + cursor, acc->prefix[0], file->len - cursor);
            if (p == NULL) {
                break;
            }

            size_t i = (size_t)(p - file->contents);
            if (i + acc->prefix_len > file->len) {
                break;
            }

            cursor = i + 1;

            if (memcmp(file->contents + i, acc->prefix, acc->prefix_len) != 0) {
                continue;
            }

            // reject mid-identifier matches
            if (i > 0 && prefix_is_ident && is_ident_char(file->contents[i - 1])) {
                continue;
            }

            // only support ${}, reject $${}
            if (acc->pattern == expansion && i > 0 && file->contents[i - 1] == DOLLAR_SIGN) {
                continue;
            }

            candidate_t cand = {.pos = i, .acc = a};
            DYN_ARR_APPEND(out, cand);
        }
    }
}

// order candidates by position; ties resolve by accessor array order, which
// mirrors get_accessor's first-match-wins iteration in the old single pass
static int cmp_candidates(const void *a, const void *b) {
    const candidate_t *ca = a;
    const candidate_t *cb = b;

    if (ca->pos != cb->pos) {
        return ca->pos < cb->pos ? -1 : 1;
    }

    if (ca->acc != cb->acc) {
        return ca->acc < cb->acc ? -1 : 1;
    }

    return 0;
}

// advance line accounting from wherever it last stopped up to (but not
// including) 'target', memchr-hopping between newlines
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
    candidate_list_t candidates = {0};
    collect_candidates(file, file_ext_match, &candidates);

    if (candidates.count == 0) {
        free(candidates.items);
        return;
    }

    qsort(candidates.items, candidates.count, sizeof(*candidates.items), cmp_candidates);

    // 'cursor' is the position the old single-pass scanner would resume from:
    // candidates before it fall inside a consumed region (or a failed match's
    // prefix span) and are discarded, exactly as the byte-walk never visited
    // them
    size_t cursor = 0;
    size_t line = 1;
    size_t line_start = 0;
    size_t line_cursor = 0;

    for (size_t c = 0; c < candidates.count; ++c) {
        const candidate_t *cand = &candidates.items[c];

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

        env_key_match_t found_env_key_match = {
            .key = env.key,
            .key_len = env.key_len,
            .line = line,
            .byte = env.start - line_start + 1,
        };

        DYN_ARR_APPEND(env_key_matches, found_env_key_match);

        cursor = env.end;
    }

    free(candidates.items);
}
