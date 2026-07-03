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

// appends the current byte to 'value' as a literal and advances past it
static void take_byte(tokenizer_t *tokenizer, byte_list_t *value) {
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

static void log_token_error(const tokenizer_t *tokenizer) {
    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name, tokenizer->line,
              tokenizer->byte);
}

static void log_err_at(size_t pad, size_t tildes, const char *hint_fmt, ...) {
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

static size_t log_token_line(const token_t *token) {
    const char *key = token->key ? token->key : "(none)";
    log_f("   %s=", key);

    size_t prefix_len = strlen(key) + 1;
    for (size_t k = 0; k < token->values.count; ++k) {
        const value_token_t *v = &token->values.items[k];
        if (v->kind == LITERAL) {
            fwrite(v->value, 1, v->value_len, stderr);
            prefix_len += v->value_len;
        }
    }

    return prefix_len;
}

static result_t err_unterminated_quote(const tokenizer_t *tokenizer, const token_t *token, const byte_list_t *value,
                                       char quote) {
    log_token_error(tokenizer);
    log_error("The %s key has an unterminated quoted value.\n", token->key ? token->key : "(none)");

    size_t prefix_len = log_token_line(token);
    log_f("%c%.*s\n", quote, (int)value->count, value->items);
    log_err_at(prefix_len, value->count, "(missing a closing quote %c)", quote);

    return OPERATION_FAILURE;
}

static result_t validate_and_append_token(tokenizer_t *tokenizer, token_t *token, byte_list_t *value,
                                          bool allow_empty) {
    if (value->count > 0 || token->values.count == 0) {
        commit_token(LITERAL, tokenizer, token, value);
    }

    value->count = 0;

    if (!allow_empty &&
        (token->values.count == 0 || (token->values.count == 1 && token->values.items[0].value_len == 0))) {
        log_token_error(tokenizer);
        log_f("The '%s' key has an empty value assignment.\n", token->key);
        log_f("   %s=\n", token->key);
        log_err_at(strlen(token->key) + 1, 0, "(missing value)");
        return OPERATION_FAILURE;
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

    if (args->dry_run) {
        log_info("[INFO]");
        log_f(" Tokenizing ");
        log_fi("%s", file->path);
        log_f(" file...\n\n");
    }

    size_t tokens_before = tokenizer->tokens.count;
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
                    take_byte(tokenizer, &value);
                }
                break;
            }
            case LINE_DELIMITER: {
                if (quote != 0) {
                    result = err_unterminated_quote(tokenizer, &token, &value, quote);
                    goto done;
                }

                if (token.key != NULL) {
                    result = validate_and_append_token(tokenizer, &token, &value, quoted);
                    if (!result.ok) {
                        goto done;
                    }
                }

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
                    take_byte(tokenizer, &value);
                    continue;
                }

                // trim surrounding spaces/tabs off the pending key
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
                    log_token_error(tokenizer);
                    log_error("A value assignment ('=') was found without a key name.\n");

                    size_t line_end =
                        index_of_scalar(tokenizer->file, tokenizer->file_len, tokenizer->i, LINE_DELIMITER);
                    const char *rest = tokenizer->file + tokenizer->i;
                    size_t rest_len = line_end - tokenizer->i;

                    log_f("   %.*s\n", (int)rest_len, rest);
                    log_err_at(0, rest_len > 1 ? rest_len - 1 : 0, "(missing key)");

                    result = OPERATION_FAILURE;
                    goto done;
                }

                if (!is_valid_key(value.items + start, end - start)) {
                    log_token_error(tokenizer);
                    log_error("The key '%.*s' is not a valid ENV name.\n", (int)(end - start), value.items + start);

                    log_f("   %.*s=\n", (int)(end - start), value.items + start);
                    log_err_at(0, end - start - 1, "(keys must match [A-Za-z_][A-Za-z0-9_]*)");

                    result = OPERATION_FAILURE;
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
                // hash inside a literal
                if (token.key != NULL) {
                    take_byte(tokenizer, &value);
                    continue;
                }

                scan_until(tokenizer, &value, STOP_NL, STOP_NL_LEN);

                commit_token(COMMENTED, tokenizer, &token, &value);
                append_token(tokenizer, &token);
                value.count = 0;
                break;
            case DOLLAR_SIGN: {
                // inside single quotes '$' is always literal (no interpolation),
                // and a dollar sign not followed by '{' is literal
                if (quote == SINGLE_QUOTE || peek(tokenizer, 1) != OPEN_BRACE) {
                    take_byte(tokenizer, &value);
                    continue;
                }

                // commit anything accumulated before the "${"
                if (value.count != 0) {
                    commit_token(LITERAL, tokenizer, &token, &value);
                    value.count = 0;
                }

                // skip "${"
                skip_byte(tokenizer, 2);

                scan_until(tokenizer, &value, STOP_BRACE_NL, STOP_BRACE_NL_LEN);

                if (peek(tokenizer, 0) != CLOSE_BRACE) {
                    log_token_error(tokenizer);
                    log_error("The %s key has an unterminated value interpolation.\n",
                              token.key ? token.key : "(none)");

                    size_t prefix_len = log_token_line(&token);
                    log_f("${%.*s\n", (int)value.count, value.items);
                    log_err_at(prefix_len + 1, value.count, "(missing a closing brace '}')");

                    result = OPERATION_FAILURE;
                    goto done;
                }

                // skip '}'
                skip_byte(tokenizer, 1);

                if (value.count == 0) {
                    log_token_error(tokenizer);
                    log_error("The %s key has an undefined key interpolation.\n", token.key ? token.key : "(none)");

                    size_t prefix_len = log_token_line(&token);
                    log_f("${}\n");
                    log_err_at(prefix_len + 1, 1, "(unresolvable interpolation key)");

                    result = OPERATION_FAILURE;
                    goto done;
                }

                commit_token(INTERPOLATED, tokenizer, &token, &value);
                value.count = 0;
                break;
            }
            case BACK_SLASH: {
                // inside single quotes '\' is always literal (no continuation)
                if (quote == SINGLE_QUOTE) {
                    take_byte(tokenizer, &value);
                    continue;
                }

                int n = peek(tokenizer, 1);
                bool crlf = n == CARRIAGE_RETURN && peek(tokenizer, 2) == LINE_DELIMITER;
                if (n != -1 && n != LINE_DELIMITER && !crlf) {
                    take_byte(tokenizer, &value);
                    continue;
                }

                // a line continuation: commit the current segment and keep tokenizing
                // the same token, so '$', '#', and '=' on continuation lines are
                // handled normally by the main loop
                if (value.count != 0) {
                    commit_token(LITERAL, tokenizer, &token, &value);
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
                    // a quote that didn't open the value is a literal byte
                    take_byte(tokenizer, &value);
                    continue;
                }

                // closing quote: consume it, tolerate trailing spaces/tabs,
                // then require end-of-line or end-of-file
                skip_byte(tokenizer, 1);
                quote = 0;

                while (peek(tokenizer, 0) == SPACE || peek(tokenizer, 0) == TAB) {
                    skip_byte(tokenizer, 1);
                }

                int next_char = peek(tokenizer, 0);
                bool trailing_crlf = next_char == CARRIAGE_RETURN && peek(tokenizer, 1) == LINE_DELIMITER;
                if (next_char != -1 && next_char != LINE_DELIMITER && !trailing_crlf) {
                    log_token_error(tokenizer);
                    log_error("The %s key has unexpected characters after a closing quote.\n",
                              token.key ? token.key : "(none)");

                    size_t line_start = tokenizer->i;
                    while (line_start > 0 && tokenizer->file[line_start - 1] != LINE_DELIMITER) {
                        --line_start;
                    }

                    size_t line_end =
                        index_of_scalar(tokenizer->file, tokenizer->file_len, tokenizer->i, LINE_DELIMITER);
                    if (line_end > line_start && tokenizer->file[line_end - 1] == CARRIAGE_RETURN) {
                        --line_end;
                    }

                    size_t caret_col = tokenizer->i - line_start;
                    size_t rest_len = line_end - tokenizer->i;

                    log_f("   %.*s\n", (int)(line_end - line_start), tokenizer->file + line_start);
                    log_err_at(caret_col, rest_len > 1 ? rest_len - 1 : 0,
                               "(only whitespace may follow a closing quote)");

                    result = OPERATION_FAILURE;
                    goto done;
                }
                break;
            }
            default: {
                const unsigned char *stops = quote == DOUBLE_QUOTE   ? DQ_STOPS
                                             : quote == SINGLE_QUOTE ? SQ_STOPS
                                                                     : LITERAL_STOPS;
                size_t stops_len = quote == DOUBLE_QUOTE   ? DQ_STOPS_LEN
                                   : quote == SINGLE_QUOTE ? SQ_STOPS_LEN
                                                           : LITERAL_STOPS_LEN;

                scan_until(tokenizer, &value, stops, stops_len);
                break;
            }
        }
    }

    // a quote may still be open at end-of-file
    if (quote != 0) {
        result = err_unterminated_quote(tokenizer, &token, &value, quote);
        goto done;
    }

    // flush a pending token if the file doesn't end with a newline
    if (token.key != NULL) {
        result = validate_and_append_token(tokenizer, &token, &value, quoted);
    }

    if (tokenizer->tokens.count == tokens_before) {
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

    if (args->dry_run && args->files.count == 0) {
        log_warning("[WARNING]");
        log_f(" The '--files' flag was not set during a dry-run, therefore no .env files will be tokenized. If "
              "just testing '--scan' results, please omit the '--dry-run' flag.\n\n");
        goto done;
    }

    if (args->dry_run) {
        log_info("[INFO]");
        log_f(" Attempting to tokenize %zu ", args->files.count);
        log_fi(".env");
        log_f(" file%s... \n\n", TO_PLURAL(args->files.count));
    }

    for (size_t fi = 0; fi < args->files.count; ++fi) {
        const char *path = args->files.items[fi];

        file_details_t file = open_file(path);
        if (file.contents == NULL) {
            result = OPERATION_FAILURE;
            goto done;
        }

        if (file.len == 0) {
            free(file.contents);
            result = operation_error("The '%s' file is empty; expected at least one KEY=VALUE assignment.\n", path);
            goto done;
        }

        result = generate_tokens(args, &file, tokenizer);
        free(file.contents);
        tokenizer->file = NULL;

        if (!result.ok) {
            goto done;
        }
    }

    if (args->dry_run) {
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
                char *sub_stem_sym = is_last_value_token ? TREE_END : TREE_BRANCH;

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

done:
    return result;
}

void free_tokenizer(tokenizer_t *tokenizer) {
    for (size_t i = 0; i < tokenizer->tokens.count; ++i) {
        free_token(&tokenizer->tokens.items[i]);
    }
    free(tokenizer->tokens.items);
}
