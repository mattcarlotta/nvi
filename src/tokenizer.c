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
        abort();
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
    commit_token(LITERAL, tokenizer, token, value);
    value->count = 0;

    if (!allow_empty &&
        (token->values.count == 0 || (token->values.count == 1 && token->values.items[0].value_len == 0))) {
        log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name, tokenizer->line,
                  tokenizer->byte);
        log_f("The '%s' key has an empty value assignment.\n", token->key);
        log_f("   %s=\n", token->key);
        log_f("   ");
        fput_repeat(stderr, ' ', strlen(token->key) + 1);
        log_f("^ (missing value)\n");
        return (result_t){.ok = false, .code = 1};
    }

    append_token(tokenizer, token);

    return (result_t){.ok = true};
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
    result_t result = {.ok = true, .code = 0};

    // 'quote' holds the active quote character (0 when unquoted) and only opens
    // when a quote immediately follows '='. 'quote_seen' persists until the
    // token is committed so an explicitly quoted empty value ("" or '') passes
    // the empty-value check.
    char quote = 0;
    bool quote_seen = false;

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
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
                }
                break;
            }
            case LINE_DELIMITER: {
                if (quote != 0) {
                    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name,
                              tokenizer->line, tokenizer->byte);
                    log_error("The %s key has an unterminated quoted value.\n", token.key ? token.key : "(none)");

                    size_t prefix_len = log_token_line(&token);
                    log_f("%c%.*s\n", quote, (int)value.count, value.items);

                    log_f("   ");
                    fput_repeat(stderr, ' ', prefix_len);
                    fputc('^', stderr);
                    fput_repeat(stderr, '~', value.count);
                    log_f(" (missing a closing quote %c)\n", quote);

                    result.ok = false;
                    result.code = 1;
                    goto done;
                }

                if (token.key != NULL) {
                    result = validate_and_append_token(tokenizer, &token, &value, quote_seen);
                    if (!result.ok) {
                        goto done;
                    }
                }

                quote_seen = false;
                value.count = 0;
                ++tokenizer->line;
                skip_byte(tokenizer, 1);
                tokenizer->byte = 1;
                break;
            }
            case ASSIGN_OP: {
                // '=' inside a value is literal
                if (token.key != NULL) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
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

                // strip an optional shell-style "export " prefix so files meant
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
                    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name,
                              tokenizer->line, tokenizer->byte);
                    log_error("A value assignment ('=') was found without a key name.\n");

                    size_t line_end =
                        index_of_scalar(tokenizer->file, tokenizer->file_len, tokenizer->i, LINE_DELIMITER);
                    const char *rest = tokenizer->file + tokenizer->i;
                    size_t rest_len = line_end - tokenizer->i;

                    log_f("   %.*s\n", (int)rest_len, rest);
                    fputs("   ^", stderr);
                    if (rest_len > 1) {
                        fput_repeat(stderr, '~', rest_len - 1);
                    }
                    log_f(" (missing key)\n");

                    result.ok = false;
                    result.code = 1;
                    goto done;
                }

                if (!is_valid_key(value.items + start, end - start)) {
                    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name,
                              tokenizer->line, tokenizer->byte);
                    log_error("The key '%.*s' is not a valid ENV name.\n", (int)(end - start), value.items + start);

                    log_f("   %.*s=\n", (int)(end - start), value.items + start);
                    log_f("   ^");
                    if (end - start > 1) {
                        fput_repeat(stderr, '~', end - start - 1);
                    }
                    log_f(" (keys must match [A-Za-z_][A-Za-z0-9_]*)\n");

                    result.ok = false;
                    result.code = 1;
                    goto done;
                }

                token.key = strndup(value.items + start, end - start);
                if (token.key == NULL) {
                    result.ok = false;
                    result.code = 1;
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
                        quote_seen = true;
                        skip_byte(tokenizer, 1);
                    }
                }
                break;
            }
            case HASH:
                // hash inside a literal
                if (token.key != NULL) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
                    continue;
                }

                scan_until(tokenizer, &value, STOP_NL, STOP_NL_LEN);

                commit_token(COMMENTED, tokenizer, &token, &value);
                append_token(tokenizer, &token);
                value.count = 0;
                break;
            case DOLLAR_SIGN: {
                // inside single quotes '$' is always literal (no interpolation)
                if (quote == SINGLE_QUOTE) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
                    continue;
                }

                // dollar sign not followed by '{' is literal
                if (peek(tokenizer, 1) != OPEN_BRACE) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
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
                    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name,
                              tokenizer->line, tokenizer->byte);
                    log_error("The %s key has an unterminated value interpolation.\n",
                              token.key ? token.key : "(none)");

                    size_t prefix_len = log_token_line(&token);
                    log_f("${%.*s\n", (int)value.count, value.items);

                    log_f("   ");
                    fput_repeat(stderr, ' ', prefix_len + 1);
                    fputc('^', stderr);
                    fput_repeat(stderr, '~', value.count);
                    log_f(" (missing a closing brace '}')\n");

                    result.ok = false;
                    result.code = 1;
                    goto done;
                }

                // skip '}'
                skip_byte(tokenizer, 1);

                if (value.count == 0) {
                    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name,
                              tokenizer->line, tokenizer->byte);
                    log_error("The %s key has an undefined key interpolation.\n", token.key ? token.key : "(none)");

                    size_t prefix_len = log_token_line(&token);
                    log_f("${}\n");

                    log_f("   ");
                    fput_repeat(stderr, ' ', prefix_len + 1);
                    fputs("^~ (unresolvable interpolation key)\n", stderr);

                    result.ok = false;
                    result.code = 1;
                    goto done;
                }

                commit_token(INTERPOLATED, tokenizer, &token, &value);

                if (quote == 0) {
                    // fold a trailing '\r\n'
                    if (peek(tokenizer, 0) == CARRIAGE_RETURN && peek(tokenizer, 1) == LINE_DELIMITER) {
                        skip_byte(tokenizer, 1);
                    }

                    if (peek(tokenizer, 0) == LINE_DELIMITER) {
                        append_token(tokenizer, &token);
                        quote_seen = false;
                        ++tokenizer->line;
                        // skip '\n'
                        skip_byte(tokenizer, 1);
                        tokenizer->byte = 1;
                    }
                }

                value.count = 0;
                break;
            }
            case BACK_SLASH: {
                // inside single quotes '\' is always literal (no continuation)
                if (quote == SINGLE_QUOTE) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
                    continue;
                }

                int n = peek(tokenizer, 1);
                bool crlf = n == CARRIAGE_RETURN && peek(tokenizer, 2) == LINE_DELIMITER;
                if (n != -1 && n != LINE_DELIMITER && !crlf) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
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
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
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
                    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name,
                              tokenizer->line, tokenizer->byte);
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

                    log_f("   ");
                    log_f("%.*s\n", (int)(line_end - line_start), tokenizer->file + line_start);
                    fput_repeat(stderr, ' ', caret_col + 3);
                    fputc('^', stderr);
                    if (rest_len > 1) {
                        fput_repeat(stderr, '~', rest_len - 1);
                    }
                    log_f(" (only whitespace may follow a closing quote)\n");

                    result.ok = false;
                    result.code = 1;
                    goto done;
                }
                break;
            }
            default: {
                const unsigned char *char_stops = LITERAL_STOPS;
                size_t char_stops_len = LITERAL_STOPS_LEN;

                if (quote == DOUBLE_QUOTE) {
                    char_stops = DQ_STOPS;
                    char_stops_len = DQ_STOPS_LEN;
                } else if (quote == SINGLE_QUOTE) {
                    char_stops = SQ_STOPS;
                    char_stops_len = SQ_STOPS_LEN;
                }

                scan_until(tokenizer, &value, char_stops, char_stops_len);
                break;
            }
        }
    }

    // a quote may still be open at end-of-file
    if (quote != 0) {
        log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name, tokenizer->line,
                  tokenizer->byte);
        log_error("The %s key has an unterminated quoted value.\n", token.key ? token.key : "(none)");

        size_t prefix_len = log_token_line(&token);
        log_f("%c%.*s\n", quote, (int)value.count, value.items);

        log_f("   ");
        fput_repeat(stderr, ' ', prefix_len);
        fputc('^', stderr);
        fput_repeat(stderr, '~', value.count);
        log_f(" (missing a closing quote %c)\n", quote);

        result.ok = false;
        result.code = 1;
        goto done;
    }

    // flush a pending token if the file doesn't end with a newline
    if (token.key != NULL) {
        result = validate_and_append_token(tokenizer, &token, &value, quote_seen);
    }

    if (tokenizer->tokens.count == tokens_before) {
        log_error("[ERROR] Unable to generate tokens for %s. Ensure the .env file is valid by following the KEY=VALUE "
                  "spec; aborting.",
                  tokenizer->file_name);

        result.ok = false;
        result.code = 1;
        goto done;
    }

done:
    free_token(&token);
    free(value.items);
    return result;
}

result_t run_tokenizer(const args_t *args, tokenizer_t *tokenizer) {
    result_t result = {.ok = true, .code = 0};

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
            // open_file already logged the cause (unopenable, unreadable, or oversized)
            result.ok = false;
            result.code = 1;
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
