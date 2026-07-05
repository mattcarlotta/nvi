#include "tokenizer.h"
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
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include "shims.h"
#endif

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} byte_list_t;

static void report_tokenizing_file(const args_t *args, const char *path) {
    if (!args->dry_run) {
        return;
    }

    log_info("[INFO]");
    log_f(" Tokenizing ");
    log_fi("%s", path);
    log_f(" file...\n\n");
}

static void report_missing_files_warning(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_warning("[WARNING]");
    log_f(" The '--files' flag was not set during a dry-run, therefore no .env files will be tokenized. If "
          "just testing '--scan' results, please omit the '--dry-run' flag.\n\n");
}

static void report_tokenizer_start(const args_t *args) {
    if (!args->dry_run) {
        return;
    }

    log_info("[INFO]");
    log_f(" Attempting to tokenize %zu ", args->files.count);
    log_fi(".env");
    log_f(" file%s... \n\n", TO_PLURAL(args->files.count));
}

static void report_tokenizer_summary(const args_t *args, const tokenizer_t *tokenizer) {
    if (!args->dry_run) {
        return;
    }

    log_info("[INFO]");
    log_f(" The following %zu token%s were generated from the %zu ", tokenizer->tokens.count,
          TO_PLURAL(tokenizer->tokens.count), args->files.count);
    log_fi(".env");
    log_f(" files...\n");

    for (size_t ti = 0; ti < tokenizer->tokens.count; ++ti) {
        const token_t *token = &tokenizer->tokens.items[ti];

        log_info("\n[INFO]");
        log_f(" Token #%zu\n", ti + 1);
        log_f("    \u2022 file: ");
        log_fi("%s", token->file);
        log_f("\n");
        log_f("    \u2022 key: ");
        log_bold_info("%s \n", token->key ? token->key : "(none)");
        log_f("    \u2022 value%s:", TO_PLURAL(token->values.count));

        for (size_t vi = 0; vi < token->values.count; ++vi) {
            const value_token_t *v = &token->values.items[vi];
            bool is_last_value_token = vi == token->values.count - 1;
            const char *sub_stem_sym = is_last_value_token ? TREE_END : TREE_BRANCH;

            log_f("\n      %s\u2500 ", sub_stem_sym);
            log_info("%s \u21A0 ", get_value_kind_name(v->kind));
            log_f("%.*s", (int)v->value_len, v->value);
            log_comment(" [%zu:%zu]", v->line, v->byte);
        }

        if (ti != tokenizer->tokens.count - 1) {
            log_f("\n");
        }
    }

    log_f("\n\n");
}

static size_t report_token_line(const token_t *token) {
    const char *key = token->key ? token->key : "(none)";
    log_f("   %s=", key);

    size_t prefix_len = strlen(key) + 1;
    for (size_t k = 0; k < token->values.count; ++k) {
        const value_token_t *v = &token->values.items[k];
        if (v->kind == LITERAL_VALUE) {
            fwrite(v->value, 1, v->value_len, stderr);
            prefix_len += v->value_len;
        }
    }

    return prefix_len;
}

static void report_token_error(const tokenizer_t *tokenizer) {
    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name, tokenizer->line,
              tokenizer->byte);
}

static void report_token_error_at(size_t pad, size_t tildes, const char *hint_fmt, ...) {
    log_f("   ");
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

static result_t report_quote_error(const tokenizer_t *tokenizer, const token_t *token, const byte_list_t *value,
                                   char quote) {
    report_token_error(tokenizer);
    log_error("The %s key has an unterminated quoted value.\n", token->key ? token->key : "(none)");

    size_t prefix_len = report_token_line(token);
    log_f("%c%.*s\n", quote, (int)value->count, value->items);
    report_token_error_at(prefix_len, value->count, "(missing a closing quote %c)", quote);

    return OPERATION_FAILURE;
}

static result_t report_empty_value_error(const tokenizer_t *tokenizer, const char *key) {
    report_token_error(tokenizer);
    log_f("The '%s' key has an empty value assignment.\n", key);
    log_f("   %s=\n", key);
    report_token_error_at(strlen(key) + 1, 0, "(missing value)");

    return OPERATION_FAILURE;
}

static result_t report_missing_key_error(const tokenizer_t *tokenizer) {
    report_token_error(tokenizer);
    log_error("A value assignment ('=') was found without a key name.\n");

    size_t line_end = index_of_scalar(tokenizer->file, tokenizer->file_len, tokenizer->i, LINE_DELIMITER);
    const char *rest = tokenizer->file + tokenizer->i;
    size_t rest_len = line_end - tokenizer->i;

    log_f("   %.*s\n", (int)rest_len, rest);
    report_token_error_at(0, rest_len > 1 ? rest_len - 1 : 0, "(missing key)");

    return OPERATION_FAILURE;
}

static result_t report_invalid_key_error(const tokenizer_t *tokenizer, const char *key, size_t key_len) {
    report_token_error(tokenizer);
    log_error("The key '%.*s' is not a valid ENV name.\n", (int)key_len, key);

    log_f("   %.*s=\n", (int)key_len, key);
    report_token_error_at(0, key_len - 1, "(keys must match [A-Za-z_][A-Za-z0-9_]*)");

    return OPERATION_FAILURE;
}

static result_t report_unterminated_interpolation_error(const tokenizer_t *tokenizer, const token_t *token,
                                                        const byte_list_t *value) {
    report_token_error(tokenizer);
    log_error("The %s key has an unterminated value interpolation.\n", token->key ? token->key : "(none)");

    size_t prefix_len = report_token_line(token);
    log_f("${%.*s\n", (int)value->count, value->items);
    report_token_error_at(prefix_len + 1, value->count, "(missing a closing brace '}')");

    return OPERATION_FAILURE;
}

static result_t report_empty_interpolation_error(const tokenizer_t *tokenizer, const token_t *token) {
    report_token_error(tokenizer);
    log_error("The %s key has an undefined key interpolation.\n", token->key ? token->key : "(none)");

    size_t prefix_len = report_token_line(token);
    log_f("${}\n");
    report_token_error_at(prefix_len + 1, 1, "(unresolvable interpolation key)");

    return OPERATION_FAILURE;
}

static result_t report_trailing_chars_error(const tokenizer_t *tokenizer, const token_t *token) {
    report_token_error(tokenizer);
    log_error("The %s key has unexpected characters after a closing quote.\n", token->key ? token->key : "(none)");

    size_t line_start = tokenizer->i;
    while (line_start > 0 && tokenizer->file[line_start - 1] != LINE_DELIMITER) {
        --line_start;
    }

    size_t line_end = index_of_scalar(tokenizer->file, tokenizer->file_len, tokenizer->i, LINE_DELIMITER);
    if (line_end > line_start && tokenizer->file[line_end - 1] == CARRIAGE_RETURN) {
        --line_end;
    }

    size_t caret_col = tokenizer->i - line_start;
    size_t rest_len = line_end - tokenizer->i;

    log_f("   %.*s\n", (int)(line_end - line_start), tokenizer->file + line_start);
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

static void commit_byte(tokenizer_t *tokenizer, byte_list_t *value) {
    DYN_ARR_APPEND(value, tokenizer->file[tokenizer->i]);
    skip_byte(tokenizer, 1);
}

static void scan_until(tokenizer_t *tokenizer, byte_list_t *value, const unsigned char *set, size_t set_len) {
    size_t end = tokenizer->i;
    while (end < tokenizer->file_len && memchr(set, (unsigned char)tokenizer->file[end], set_len) == NULL) {
        ++end;
    }

    DYN_ARR_APPEND_MANY(value, tokenizer->file + tokenizer->i, end - tokenizer->i);

    tokenizer->byte += end - tokenizer->i;
    tokenizer->i = end;
}

static void commit_token(value_kind_t kind, tokenizer_t *tokenizer, token_t *token, const byte_list_t *value) {
    char *copy = malloc(value->count + 1);
    if (copy == NULL) {
        log_error("[ERROR] Unable to copy token value (not enough system memory?); aborting.");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

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

    DYN_ARR_APPEND(&token->values, vt);
}

static void append_token(tokenizer_t *tokenizer, token_t *token) {
    DYN_ARR_APPEND(&tokenizer->tokens, *token);
    *token = (token_t){.file = tokenizer->file_name};
}

static void free_token(token_t *token) {
    free(token->key);

    for (size_t k = 0; k < token->values.count; ++k) {
        free(token->values.items[k].value);
    }

    free(token->values.items);
    token->key = NULL;
    token->values = (value_token_list_t){0};
}

static result_t validate_and_append_token(tokenizer_t *tokenizer, token_t *token, byte_list_t *value,
                                          bool allow_empty) {
    if (value->count > 0 || token->values.count == 0) {
        commit_token(LITERAL_VALUE, tokenizer, token, value);
    }

    value->count = 0;

    if (!allow_empty &&
        (token->values.count == 0 || (token->values.count == 1 && token->values.items[0].value_len == 0))) {
        return report_empty_value_error(tokenizer, token->key);
    }

    append_token(tokenizer, token);

    return RESULT_OK;
}

result_t generate_tokens(const args_t *args, const file_details_t *file, tokenizer_t *tokenizer) {
    tokenizer->file_name = file->path;
    tokenizer->file = file->contents;
    tokenizer->file_len = file->len;
    tokenizer->i = 0;
    tokenizer->byte = 1;
    tokenizer->line = 1;

    // skip a UTF-8 BOM so it doesn't become part of the first key
    if (tokenizer->file_len >= 3 && memcmp(tokenizer->file, "\xEF\xBB\xBF", 3) == 0) {
        tokenizer->i = 3;
    }

    report_tokenizing_file(args, file->path);

    size_t prev_token_count = tokenizer->tokens.count;
    token_t token = {.file = tokenizer->file_name};
    byte_list_t value = {0};
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
                    commit_byte(tokenizer, &value);
                }
                break;
            }
            case LINE_DELIMITER: {
                if (quote != 0) {
                    result = report_quote_error(tokenizer, &token, &value, quote);
                    goto done;
                }

                if (token.key != NULL) {
                    result = validate_and_append_token(tokenizer, &token, &value, quoted);
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
                    commit_byte(tokenizer, &value);
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

                token.key = strndup(value.items + start, end - start);
                if (token.key == NULL) {
                    result = OPERATION_FAILURE;
                    goto done;
                }

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
                    commit_byte(tokenizer, &value);
                    continue;
                }

                scan_until(tokenizer, &value, STOP_NL, STOP_NL_LEN);

                commit_token(COMMENTED_LINE, tokenizer, &token, &value);
                append_token(tokenizer, &token);
                value.count = 0;
                break;
            case DOLLAR_SIGN: {
                // inside single quotes '$' is always literal (no interpolation),
                // and a dollar sign not followed by '{' is literal
                if (quote == SINGLE_QUOTE || peek(tokenizer, 1) != OPEN_BRACE) {
                    commit_byte(tokenizer, &value);
                    continue;
                }

                // commit anything accumulated before the "${"
                if (value.count != 0) {
                    commit_token(LITERAL_VALUE, tokenizer, &token, &value);
                    value.count = 0;
                }

                // skip "${"
                skip_byte(tokenizer, 2);

                scan_until(tokenizer, &value, STOP_BRACE_NL, STOP_BRACE_NL_LEN);

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

                commit_token(INTERPOLATED_KEY, tokenizer, &token, &value);
                value.count = 0;
                break;
            }
            case BACK_SLASH: {
                if (quote == SINGLE_QUOTE) {
                    commit_byte(tokenizer, &value);
                    continue;
                }

                int n = peek(tokenizer, 1);
                bool crlf = n == CARRIAGE_RETURN && peek(tokenizer, 2) == LINE_DELIMITER;
                if (n != -1 && n != LINE_DELIMITER && !crlf) {
                    commit_byte(tokenizer, &value);
                    continue;
                }

                // a line continuation: commit the current segment and keep tokenizing
                // the same token, so '$', '#', and '=' on continuation lines are
                // handled normally by the main loop
                if (value.count != 0) {
                    commit_token(LITERAL_VALUE, tokenizer, &token, &value);
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
                    commit_byte(tokenizer, &value);
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

                scan_until(tokenizer, &value, stops, stops_len);
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
        result = validate_and_append_token(tokenizer, &token, &value, quoted);
    }

    if (tokenizer->tokens.count == prev_token_count) {
        log_error("[ERROR] Unable to generate tokens for %s. Ensure the .env file is valid by following the KEY=VALUE "
                  "spec; aborting.",
                  tokenizer->file_name);

        result = OPERATION_FAILURE;
        goto done;
    }

done:
    free_token(&token);
    free(value.items);
    return result;
}

result_t run_tokenizer(const args_t *args, tokenizer_t *tokenizer) {
    result_t result = RESULT_OK;

    if (args->files.count == 0) {
        report_missing_files_warning(args);
        return result;
    }

    report_tokenizer_start(args);

    for (size_t fi = 0; fi < args->files.count; ++fi) {
        const char *path = args->files.items[fi];

        file_details_t file = open_file(path);
        if (file.contents == NULL) {
            return OPERATION_FAILURE;
        }

        if (file.len == 0) {
            free(file.contents);
            return operation_error("The '%s' file is empty; expected at least one KEY=VALUE assignment.\n", path);
        }

        result = generate_tokens(args, &file, tokenizer);
        free(file.contents);
        tokenizer->file = NULL;

        if (!result.ok) {
            return result;
        }
    }

    report_tokenizer_summary(args, tokenizer);

    return result;
}

void free_tokenizer(tokenizer_t *tokenizer) {
    for (size_t i = 0; i < tokenizer->tokens.count; ++i) {
        free_token(&tokenizer->tokens.items[i]);
    }
    free(tokenizer->tokens.items);
}
