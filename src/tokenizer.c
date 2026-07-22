#include "tokenizer.h"
#include "arena.h"
#include "arg.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "file.h"
#include "log.h"
#include "macros.h"
#include "result.h"
#include "tty.h"
#include "utils.h"
#include <stdarg.h>
#include <string.h>

static void report_tokenizing_file(const args_t *args, const char *path) {
    if (!args->dry_run) {
        return;
    }

    log_info(SINK_STDERR, "[INFO]");
    log_f(SINK_STDERR, " Tokenizing ");
    log_fi(SINK_STDERR, "%s", path);
    log_f(SINK_STDERR, " file...\n\n");
}

static void report_missing_files_warning(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_warning(SINK_STDERR, "[WARNING]");
    log_f(SINK_STDERR,
          " The '--files' flag was not set during a dry-run, therefore no .env files will be tokenized. If "
          "just testing '--scan' results, please omit the '--dry-run' flag.\n\n");
}

static void report_tokenizer_start(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_info(SINK_STDERR, "[INFO]");
    log_f(SINK_STDERR, " Attempting to tokenize %zu ", args->files.count);
    log_fi(SINK_STDERR, ".env");
    log_f(SINK_STDERR, " file%s... \n\n", TO_PLURAL(args->files.count));
}

static void report_tokenizer_summary(const args_t *args, const tokenizer_t *tokenizer) {
    if (!args->dry_run) {
        return;
    }

    log_info(SINK_STDERR, "[INFO]");
    log_f(SINK_STDERR, " The following %zu token%s were generated from the %zu ", tokenizer->tokens.count,
          TO_PLURAL(tokenizer->tokens.count), args->files.count);
    log_fi(SINK_STDERR, ".env");
    log_f(SINK_STDERR, " file%s...\n", TO_PLURAL(args->files.count));

    for (size_t ti = 0; ti < tokenizer->tokens.count; ++ti) {
        const token_t *token = &tokenizer->tokens.items[ti];

        log_info(SINK_STDERR, "\n[INFO]");
        log_f(SINK_STDERR, " Token #%zu\n", ti + 1);
        log_f(SINK_STDERR, "    %s file: ", BULLET);
        log_fi(SINK_STDERR, "%s", token->file);
        log_f(SINK_STDERR, "\n");
        log_f(SINK_STDERR, "    %s key: ", BULLET);
        log_bold_info(SINK_STDERR, "%s \n", token->key ? token->key : "(none)");
        log_f(SINK_STDERR, "    %s value%s:", BULLET, TO_PLURAL(token->values.count));

        for (size_t vi = 0; vi < token->values.count; ++vi) {
            const value_token_t *v = &token->values.items[vi];
            bool is_last_value_token = vi == token->values.count - 1;
            const char *sub_stem_sym = is_last_value_token ? TREE_END : TREE_BRANCH;

            log_f(SINK_STDERR, "\n      %s%s ", sub_stem_sym, TREE_RUNG);
            log_info(SINK_STDERR, "%s: ", get_value_kind_name(v->kind));
            if (args->reveal) {
                log_f(SINK_STDERR, "%.*s", (int)v->value_len, v->value);
            } else {
                log_f(SINK_STDERR, "*****");
            }
            log_comment(SINK_STDERR, " [%zu:%zu]", v->line, v->byte);
        }

        if (ti != tokenizer->tokens.count - 1) {
            log_f(SINK_STDERR, "\n");
        }
    }

    log_f(SINK_STDERR, "\n\n");
}

static void report_value(const char *s, size_t len, bool reveal) {
    if (reveal) {
        fwrite(s, 1, len, stderr);
        return;
    }

    fput_repeat(stderr, '*', len);
}

static size_t report_token_line(const token_t *token, bool reveal) {
    const char *key = token->key ? token->key : "(none)";
    log_f(SINK_STDERR, "   %s=", key);

    size_t prefix_len = strlen(key) + 1;
    for (size_t k = 0; k < token->values.count; ++k) {
        const value_token_t *v = &token->values.items[k];
        if (v->kind == LITERAL_VALUE) {
            report_value(v->value, v->value_len, reveal);
            prefix_len += v->value_len;
        }
    }

    return prefix_len;
}

static void report_token_error(const tokenizer_t *tokenizer) {
    log_error(SINK_STDERR, "[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name, tokenizer->line,
              tokenizer->byte);
}

static void report_token_error_at(size_t pad, size_t tildes, const char *hint_fmt, ...) {
    log_f(SINK_STDERR, "   ");
    fput_repeat(stderr, ' ', pad);
    fputc('^', stderr);
    fput_repeat(stderr, '~', tildes);
    fputc(' ', stderr);

    va_list args;
    va_start(args, hint_fmt);
    vfprintf(stderr, hint_fmt, args);
    va_end(args);

    fputc('\n', stderr);
}

static result_t report_quote_error(const tokenizer_t *tokenizer, const token_t *token, const buf_t *value, char quote) {
    report_token_error(tokenizer);
    log_error(SINK_STDERR, "The %s key has an unterminated quoted value.\n", token->key ? token->key : "(none)");

    size_t prefix_len = report_token_line(token, tokenizer->reveal);
    fputc(quote, stderr);
    report_value(value->items, value->count, tokenizer->reveal);
    fputc('\n', stderr);
    report_token_error_at(prefix_len, value->count, "(missing a closing quote %c)", quote);

    return OPERATION_FAILURE;
}

static result_t report_empty_value_error(const tokenizer_t *tokenizer, const char *key) {
    report_token_error(tokenizer);
    log_f(SINK_STDERR, "The '%s' key has an empty value assignment.\n", key);
    log_f(SINK_STDERR, "   %s=\n", key);
    report_token_error_at(strlen(key) + 1, 0, "(missing value)");

    return OPERATION_FAILURE;
}

static result_t report_missing_key_error(const tokenizer_t *tokenizer) {
    report_token_error(tokenizer);
    log_error(SINK_STDERR, "A value assignment ('=') was found without a key name.\n");

    size_t line_end = index_of_scalar(tokenizer->file, tokenizer->file_len, tokenizer->i, LINE_DELIMITER);
    const char *rest = tokenizer->file + tokenizer->i;
    size_t rest_len = line_end - tokenizer->i;

    log_f(SINK_STDERR, "   ");
    if (rest_len > 0) {
        fputc(rest[0], stderr);
        report_value(rest + 1, rest_len - 1, tokenizer->reveal);
    }
    fputc('\n', stderr);
    report_token_error_at(0, rest_len > 1 ? rest_len - 1 : 0, "(missing key)");

    return OPERATION_FAILURE;
}

static result_t report_invalid_key_error(const tokenizer_t *tokenizer, const char *key, size_t key_len) {
    report_token_error(tokenizer);
    log_error(SINK_STDERR, "The key '%.*s' is not a valid ENV name.\n", (int)key_len, key);

    log_f(SINK_STDERR, "   %.*s=\n", (int)key_len, key);
    report_token_error_at(0, key_len - 1, "(keys must match [A-Za-z_][A-Za-z0-9_]*)");

    return OPERATION_FAILURE;
}

static result_t report_unterminated_interpolation_error(const tokenizer_t *tokenizer, const token_t *token,
                                                        const buf_t *value) {
    report_token_error(tokenizer);
    log_error(SINK_STDERR, "The %s key has an unterminated value interpolation.\n", token->key ? token->key : "(none)");

    size_t prefix_len = report_token_line(token, tokenizer->reveal);
    log_f(SINK_STDERR, "${%.*s\n", (int)value->count, value->items);
    report_token_error_at(prefix_len + 1, value->count, "(missing a closing brace '}')");

    return OPERATION_FAILURE;
}

static result_t report_empty_interpolation_error(const tokenizer_t *tokenizer, const token_t *token) {
    report_token_error(tokenizer);
    log_error(SINK_STDERR, "The %s key has an undefined key interpolation.\n", token->key ? token->key : "(none)");

    size_t prefix_len = report_token_line(token, tokenizer->reveal);
    log_f(SINK_STDERR, "${}\n");
    report_token_error_at(prefix_len + 1, 1, "(unresolvable interpolation key)");

    return OPERATION_FAILURE;
}

static result_t report_trailing_chars_error(const tokenizer_t *tokenizer, const token_t *token) {
    report_token_error(tokenizer);
    log_error(SINK_STDERR, "The %s key has unexpected characters after a closing quote.\n",
              token->key ? token->key : "(none)");

    size_t line_start = tokenizer->i - (tokenizer->byte - 1);

    size_t line_end = index_of_scalar(tokenizer->file, tokenizer->file_len, tokenizer->i, LINE_DELIMITER);
    if (line_end > line_start && tokenizer->file[line_end - 1] == CARRIAGE_RETURN) {
        --line_end;
    }

    size_t caret_col = tokenizer->i - line_start;
    size_t rest_len = line_end - tokenizer->i;

    const char *line = tokenizer->file + line_start;
    size_t line_len = line_end - line_start;
    size_t assign_op = index_of_scalar(line, line_len, 0, ASSIGN_OP);
    size_t visible_len = assign_op < line_len ? assign_op + 1 : 0;

    log_f(SINK_STDERR, "   %.*s", (int)visible_len, line);
    report_value(line + visible_len, line_len - visible_len, tokenizer->reveal);
    fputc('\n', stderr);
    report_token_error_at(caret_col, rest_len > 1 ? rest_len - 1 : 0, "(only whitespace may follow a closing quote)");

    return OPERATION_FAILURE;
}

static const unsigned char LITERAL_STOPS[] = {
    NULL_CHAR, LINE_DELIMITER, CARRIAGE_RETURN, ASSIGN_OP, HASH, DOLLAR_SIGN, BACK_SLASH,
};
static const size_t LITERAL_STOPS_LEN = ARR_LEN(LITERAL_STOPS);

static const unsigned char DQ_STOPS[] = {
    NULL_CHAR, LINE_DELIMITER, CARRIAGE_RETURN, DOUBLE_QUOTE, DOLLAR_SIGN, BACK_SLASH,
};
static const size_t DQ_STOPS_LEN = ARR_LEN(DQ_STOPS);

static const unsigned char SQ_STOPS[] = {
    NULL_CHAR,
    LINE_DELIMITER,
    CARRIAGE_RETURN,
    SINGLE_QUOTE,
};
static const size_t SQ_STOPS_LEN = ARR_LEN(SQ_STOPS);

static const unsigned char STOP_NL[] = {LINE_DELIMITER, CARRIAGE_RETURN};
static const size_t STOP_NL_LEN = ARR_LEN(STOP_NL);

static const unsigned char STOP_BRACE_NL[] = {NULL_CHAR, CLOSE_BRACE, LINE_DELIMITER, CARRIAGE_RETURN};
static const size_t STOP_BRACE_NL_LEN = ARR_LEN(STOP_BRACE_NL);

static void skip_byte(tokenizer_t *tokenizer, size_t offset) {
    tokenizer->byte += offset;
    tokenizer->i += offset;
}

static int peek(const tokenizer_t *tokenizer, size_t offset) {
    size_t index = tokenizer->i + offset;

    if (index >= tokenizer->file_len) {
        return -1;
    }

    return (unsigned char)tokenizer->file[index];
}

static void commit_byte(arena_t *scratch, tokenizer_t *tokenizer, buf_t *value) {
    DYN_ARR_APPEND(scratch, value, tokenizer->file[tokenizer->i]);
    skip_byte(tokenizer, 1);
}

static void scan_until(arena_t *scratch, tokenizer_t *tokenizer, buf_t *value, const unsigned char *set,
                       size_t set_len) {
    size_t end = tokenizer->i;
    while (end < tokenizer->file_len && memchr(set, (unsigned char)tokenizer->file[end], set_len) == NULL) {
        ++end;
    }

    DYN_ARR_APPEND_MANY(scratch, value, tokenizer->file + tokenizer->i, end - tokenizer->i);

    tokenizer->byte += end - tokenizer->i;
    tokenizer->i = end;
}

static void commit_token(arena_t *arena, value_kind_t kind, tokenizer_t *tokenizer, token_t *token,
                         const buf_t *value) {
    char *copy = arena_alloc(arena, value->count + 1);

    if (value->count > 0) {
        memcpy(copy, value->items, value->count);
    }
    copy[value->count] = '\0';

    value_token_t vt = {
        .value = copy,
        .value_len = value->count,
        .kind = kind,
        .line = tokenizer->line,
        .byte = tokenizer->byte,
    };

    DYN_ARR_APPEND(arena, &token->values, vt);
}

static void append_token(arena_t *arena, tokenizer_t *tokenizer, token_t *token) {
    DYN_ARR_APPEND(arena, &tokenizer->tokens, *token);
    *token = (token_t){.file = tokenizer->file_name};
}

static result_t validate_and_append_token(arena_t *arena, tokenizer_t *tokenizer, token_t *token, buf_t *value,
                                          bool allow_empty) {
    if (value->count > 0 || token->values.count == 0) {
        commit_token(arena, LITERAL_VALUE, tokenizer, token, value);
    }

    value->count = 0;

    if (!allow_empty &&
        (token->values.count == 0 || (token->values.count == 1 && token->values.items[0].value_len == 0))) {
        return report_empty_value_error(tokenizer, token->key);
    }

    append_token(arena, tokenizer, token);

    return RESULT_OK;
}

result_t generate_tokens(arena_t *main_arena, arena_t *scratch, const args_t *args, const file_details_t *file,
                         tokenizer_t *tokenizer) {
    tokenizer->file_name = file->path;
    tokenizer->file = file->contents;
    tokenizer->file_len = file->len;
    tokenizer->i = 0;
    tokenizer->byte = 1;
    tokenizer->line = 1;
    tokenizer->reveal = args->reveal;

    // skip a UTF-8 BOM so it doesn't become part of the first key
    if (tokenizer->file_len >= 3 && memcmp(tokenizer->file, "\xEF\xBB\xBF", 3) == 0) {
        tokenizer->i = 3;
    }

    report_tokenizing_file(args, file->path);

    size_t prev_token_count = tokenizer->tokens.count;
    token_t token = {.file = tokenizer->file_name};
    buf_t value = {.arena = scratch};
    result_t result = RESULT_OK;

    // 'quote' holds the active quote character (0 when unquoted) and only opens
    // when a quote immediately follows '='.
    char quote = 0;
    // 'quoted' persists until the token is committed so an explicitly quoted
    // empty value ("" or '') passes the empty-value check.
    bool quoted = false;

    int current;
    while ((current = peek(tokenizer, 0)) != -1) {
        switch (current) {
            case NULL_CHAR: {
                skip_byte(tokenizer, 1);
                break;
            }
            case CARRIAGE_RETURN: {
                // '\r\n' (Windows line ending): drop the '\r' and let the next
                // iteration handle '\n'; a lone '\r' remains a literal byte
                if (peek(tokenizer, 1) == LINE_DELIMITER) {
                    skip_byte(tokenizer, 1);
                } else {
                    commit_byte(scratch, tokenizer, &value);
                }
                break;
            }
            case LINE_DELIMITER: {
                if (quote != 0) {
                    result = report_quote_error(tokenizer, &token, &value, quote);
                    goto done;
                }

                if (token.key != NULL) {
                    result = validate_and_append_token(main_arena, tokenizer, &token, &value, quoted);
                    if (!result.ok) {
                        goto done;
                    }
                }

                // discard any keyless bytes accumulated on this line
                quoted = false;
                value.count = 0;
                ++tokenizer->line;
                skip_byte(tokenizer, 1);
                tokenizer->byte = 1;
                break;
            }
            case ASSIGN_OP: {
                // '=' inside a value is literal
                if (token.key != NULL) {
                    commit_byte(scratch, tokenizer, &value);
                    continue;
                }

                size_t start = 0;
                size_t end = value.count;
                while (start < end && (value.items[start] == SPACE || value.items[start] == TAB)) {
                    ++start;
                }
                while (end > start && (value.items[end - 1] == SPACE || value.items[end - 1] == TAB)) {
                    --end;
                }

                // strip an optional shell-style "export " (7) prefix so files meant
                // for `source` parse the same way (the trailing check keeps a
                // literal key named "export" intact)
                if (end - start > 7 && memcmp(value.items + start, "export", 6) == 0 &&
                    (value.items[start + 6] == SPACE || value.items[start + 6] == TAB)) {
                    start += 7;
                    while (start < end && (value.items[start] == SPACE || value.items[start] == TAB)) {
                        ++start;
                    }
                }

                if (end - start == 0) {
                    result = report_missing_key_error(tokenizer);
                    goto done;
                }

                if (!is_valid_key(value.items + start, end - start)) {
                    result = report_invalid_key_error(tokenizer, value.items + start, end - start);
                    goto done;
                }

                token.key = arena_strndup(main_arena, value.items + start, end - start);

                value.count = 0;
                // skip '='
                skip_byte(tokenizer, 1);

                // a quote directly after '=' opens a quoted value; the quote
                // characters themselves are not part of the value
                {
                    int q = peek(tokenizer, 0);
                    if (q == DOUBLE_QUOTE || q == SINGLE_QUOTE) {
                        quote = (char)q;
                        quoted = true;
                        skip_byte(tokenizer, 1);
                    }
                }
                break;
            }
            case HASH:
                // commit literal hash
                if (token.key != NULL) {
                    commit_byte(scratch, tokenizer, &value);
                    continue;
                }

                scan_until(scratch, tokenizer, &value, STOP_NL, STOP_NL_LEN);

                commit_token(main_arena, COMMENTED_LINE, tokenizer, &token, &value);
                append_token(main_arena, tokenizer, &token);
                value.count = 0;
                break;
            case DOLLAR_SIGN: {
                // inside single quotes '$' is always literal (no interpolation),
                // and a dollar sign not followed by '{' is literal
                if (quote == SINGLE_QUOTE || peek(tokenizer, 1) != OPEN_BRACE) {
                    commit_byte(scratch, tokenizer, &value);
                    continue;
                }

                // commit anything accumulated before the "${"
                if (value.count != 0) {
                    commit_token(main_arena, LITERAL_VALUE, tokenizer, &token, &value);
                    value.count = 0;
                }

                // skip "${"
                skip_byte(tokenizer, 2);

                scan_until(scratch, tokenizer, &value, STOP_BRACE_NL, STOP_BRACE_NL_LEN);

                if (peek(tokenizer, 0) != CLOSE_BRACE) {
                    result = report_unterminated_interpolation_error(tokenizer, &token, &value);
                    goto done;
                }

                // skip '}'
                skip_byte(tokenizer, 1);

                if (value.count == 0) {
                    result = report_empty_interpolation_error(tokenizer, &token);
                    goto done;
                }

                commit_token(main_arena, INTERPOLATED_KEY, tokenizer, &token, &value);
                value.count = 0;
                break;
            }
            case BACK_SLASH: {
                if (quote == SINGLE_QUOTE) {
                    commit_byte(scratch, tokenizer, &value);
                    continue;
                }

                int n = peek(tokenizer, 1);
                bool crlf = n == CARRIAGE_RETURN && peek(tokenizer, 2) == LINE_DELIMITER;
                if (n != -1 && n != LINE_DELIMITER && !crlf) {
                    commit_byte(scratch, tokenizer, &value);
                    continue;
                }

                // a line continuation: commit the current segment and keep tokenizing
                // the same token, so '$', '#', and '=' on continuation lines are
                // handled normally by the main loop
                if (value.count != 0) {
                    commit_token(main_arena, LITERAL_VALUE, tokenizer, &token, &value);
                }

                value.count = 0;
                // skip "\\\n" (or "\\\r\n")
                skip_byte(tokenizer, crlf ? 3 : 2);
                ++tokenizer->line;
                tokenizer->byte = 1;
                break;
            }
            case DOUBLE_QUOTE:
            case SINGLE_QUOTE: {
                if (quote == 0 || current != quote) {
                    commit_byte(scratch, tokenizer, &value);
                    continue;
                }

                skip_byte(tokenizer, 1);
                quote = 0;

                while (peek(tokenizer, 0) == SPACE || peek(tokenizer, 0) == TAB) {
                    skip_byte(tokenizer, 1);
                }

                int next_char = peek(tokenizer, 0);
                bool trailing_crlf = next_char == CARRIAGE_RETURN && peek(tokenizer, 1) == LINE_DELIMITER;
                bool is_end_of_line = next_char == LINE_DELIMITER;
                if (next_char != -1 && !is_end_of_line && !trailing_crlf) {
                    result = report_trailing_chars_error(tokenizer, &token);
                    goto done;
                }
                break;
            }
            default: {
                const unsigned char *stops = LITERAL_STOPS;
                size_t stops_len = LITERAL_STOPS_LEN;

                if (quote == DOUBLE_QUOTE) {
                    stops = DQ_STOPS;
                    stops_len = DQ_STOPS_LEN;
                } else if (quote == SINGLE_QUOTE) {
                    stops = SQ_STOPS;
                    stops_len = SQ_STOPS_LEN;
                }

                scan_until(scratch, tokenizer, &value, stops, stops_len);
                break;
            }
        }
    }

    // a quote may still be open at end-of-file
    if (quote != 0) {
        result = report_quote_error(tokenizer, &token, &value, quote);
        goto done;
    }

    // flush a pending token if the file doesn't end with a newline
    if (token.key != NULL) {
        result = validate_and_append_token(main_arena, tokenizer, &token, &value, quoted);
    }

    if (tokenizer->tokens.count == prev_token_count) {
        log_error(SINK_STDERR,
                  "[ERROR] Unable to generate tokens for %s. Ensure the .env file is valid by following the KEY=VALUE "
                  "spec; aborting.",
                  tokenizer->file_name);

        result = OPERATION_FAILURE;
        goto done;
    }

done:
    return result;
}

result_t run_tokenizer(arena_t *main_arena, const args_t *args, tokenizer_t *tokenizer) {
    result_t result = RESULT_OK;

    if (args->files.count == 0) {
        report_missing_files_warning(args);
        return result;
    }

    report_tokenizer_start(args);

    arena_t scratch = {0};

    for (size_t fi = 0; fi < args->files.count; ++fi) {
        const char *path = args->files.items[fi];

        file_details_t file = open_file(&scratch, path);
        if (file.contents == NULL) {
            arena_free(&scratch);
            return OPERATION_FAILURE;
        }

        if (file.len == 0) {
            arena_free(&scratch);
            return operation_error("The '%s' file is empty; expected at least one KEY=VALUE assignment.\n", path);
        }

        result = generate_tokens(main_arena, &scratch, args, &file, tokenizer);
        tokenizer->file = NULL;
        arena_reset(&scratch);

        if (!result.ok) {
            break;
        }
    }

    arena_free(&scratch);

    if (!result.ok) {
        return result;
    }

    report_tokenizer_summary(args, tokenizer);

    return result;
}
