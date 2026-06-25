#include "tokenizer.h"
#include "arg.h"
#include "chars.h"
#include "dynarr.h"
#include "errors.h"
#include "file.h"
#include "log.h"
#include "result.h"
#include "tty.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} byte_list_t;

static const char *value_kind_name(value_kind_t kind) {
    switch (kind) {
        case LITERAL:
            return "literal";
        case COMMENTED:
            return "commented";
        case INTERPOLATED:
            return "interpolated";
    }

    return "unknown";
}

static void next_line(tokenizer_t *tokenizer) { tokenizer->line += 1; }

static void reset_byte(tokenizer_t *tokenizer) { tokenizer->byte = 1; }

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
        end++;
    }

    for (size_t k = tokenizer->i; k < end; ++k) {
        DYN_ARR_APPEND(value, tokenizer->file[k]);
    }

    tokenizer->byte += end - tokenizer->i;
    tokenizer->i = end;
}

static result_t commit_token(tokenizer_t *tokenizer, token_t *token, value_kind_t kind, const byte_list_t *value) {
    const char *src = value->count ? value->items : "";
    char *copy = strndup(src, value->count);
    if (copy == NULL) {
        return operation_error("Unable to copy token value (not enough system memory?); aborting.", NULL);
    }

    value_token_t vt = {
        .value = copy,
        .value_len = value->count,
        .kind = kind,
        .line = tokenizer->line,
        .byte = tokenizer->byte,
    };

    DYN_ARR_APPEND(&token->values, vt);

    return (result_t){.ok = true};
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

static void fput_repeat(FILE *f, char c, size_t n) {
    while (n--) {
        fputc(c, f);
    }
}

static void print_error_header(tokenizer_t *tokenizer) {
    log_error("[ERROR] A tokenizing error occurred in %s:%zu:%zu. ", tokenizer->file_name, tokenizer->line,
              tokenizer->byte);
}

static size_t print_token_line(const token_t *token) {
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

static void print_tokens(tokenizer_t *tokenizer) {
    log_info("[INFO]");
    log_f(" The following %zu token(s) have been generated from .env files...\n", tokenizer->tokens.count);

    for (size_t ti = 0; ti < tokenizer->tokens.count; ++ti) {
        const token_t *token = &tokenizer->tokens.items[ti];

        log_info("\n[INFO]");
        log_f(" Token #%zu\n", ti + 1);
        log_f("    \u2022 file: %s\n", token->file);
        log_f("    \u2022 key: ");
        log_bold_info("%s \n", token->key ? token->key : "(none)");
        log_f("    \u2022 value:", token->values.count);

        for (size_t vi = 0; vi < token->values.count; ++vi) {
            const value_token_t *v = &token->values.items[vi];
            bool is_last_value_token = vi == token->values.count - 1;
            char *sub_stem_sym = is_last_value_token ? TREE_END : TREE_BRANCH;

            log_f("\n      %s\u2500 ", sub_stem_sym);
            log_info("%s value \u21A0 ", value_kind_name(v->kind));
            log_f("%.*s (%zu:%zu)", (int)v->value_len, v->value, v->line, v->byte);
        }

        if (ti != tokenizer->tokens.count - 1) {
            log_f("\n");
        }
    }

    log_f("\n\n");
}

result_t generate_tokens(args_t *args, tokenizer_t *tokenizer, file_details_t *file) {
    tokenizer->file_name = file->path;
    tokenizer->file = file->contents;
    tokenizer->file_len = file->len;
    tokenizer->i = 0;
    tokenizer->byte = 1;
    tokenizer->line = 1;

    if (args->dry_run) {
        log_info("[INFO]");
        log_f(" Tokenizing %s...\n\n", file->path);
    }

    token_t token = {.file = tokenizer->file_name};
    byte_list_t value = {0};
    result_t result = {.ok = true, .errcode = 0};

    int current;
    while ((current = peek(tokenizer, 0)) != -1) {
        switch (current) {
            case NULL_CHAR:
                skip_byte(tokenizer, 1);
                break;

            case LINE_DELIMITER:
                if (token.key != NULL) {
                    result = commit_token(tokenizer, &token, LITERAL, &value);
                    if (!result.ok) {
                        goto done;
                    }
                    append_token(tokenizer, &token);
                }
                value.count = 0;
                next_line(tokenizer);
                skip_byte(tokenizer, 1);
                reset_byte(tokenizer);
                break;

            case ASSIGN_OP: {
                // '=' inside a value is literal
                if (token.key != NULL) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
                    continue;
                }

                // trim surrounding spaces/tabs off the pending key
                size_t lo = 0;
                size_t hi = value.count;
                while (lo < hi && (value.items[lo] == ' ' || value.items[lo] == '\t')) {
                    ++lo;
                }
                while (hi > lo && (value.items[hi - 1] == ' ' || value.items[hi - 1] == '\t')) {
                    --hi;
                }

                if (hi - lo == 0) {
                    print_error_header(tokenizer);
                    log_f("A value assignment ('=') was found without a key name.\n");

                    size_t end = index_of_scalar(tokenizer->file, tokenizer->file_len, tokenizer->i, LINE_DELIMITER);
                    const char *rest = tokenizer->file + tokenizer->i;
                    size_t rest_len = end - tokenizer->i;

                    log_f("   %.*s\n", (int)rest_len, rest);
                    fputs("   ^", stderr);
                    if (rest_len > 1) {
                        fput_repeat(stderr, '~', rest_len - 1);
                    }
                    log_f(" (missing key)\n");

                    result.ok = false;
                    result.errcode = 1;
                    goto done;
                }

                token.key = strndup(value.items + lo, hi - lo);
                if (token.key == NULL) {
                    result.ok = false;
                    result.errcode = 1;
                    goto done;
                }
                value.count = 0;
                skip_byte(tokenizer, 1); // '='
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

                result = commit_token(tokenizer, &token, COMMENTED, &value);
                if (!result.ok) {
                    goto done;
                }
                append_token(tokenizer, &token);
                value.count = 0;
                break;

            case DOLLAR_SIGN: {
                // dollar sign not followed by '{' is literal
                if (peek(tokenizer, 1) != OPEN_BRACE) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
                    continue;
                }

                // commit anything accumulated before the "${"
                if (value.count != 0) {
                    result = commit_token(tokenizer, &token, LITERAL, &value);
                    if (!result.ok) {
                        goto done;
                    }
                    value.count = 0;
                }

                skip_byte(tokenizer, 2); // "${"
                scan_until(tokenizer, &value, STOP_BRACE_NL, STOP_BRACE_NL_LEN);

                if (peek(tokenizer, 0) != CLOSE_BRACE) {
                    print_error_header(tokenizer);
                    log_f("The %s key has an unterminated value interpolation.\n", token.key ? token.key : "(none)");
                    size_t prefix_len = print_token_line(&token);
                    log_f("${%.*s\n", (int)value.count, value.items);

                    log_f("   ");
                    fput_repeat(stderr, ' ', prefix_len + 1);
                    fputc('^', stderr);
                    fput_repeat(stderr, '~', value.count);
                    log_f(" (missing a closing brace '}')\n");

                    result.ok = false;
                    result.errcode = 1;
                    goto done;
                }

                skip_byte(tokenizer, 1); // '}'

                if (value.count == 0) {
                    print_error_header(tokenizer);
                    log_f("The %s key has an undefined key interpolation.\n", token.key ? token.key : "(none)");

                    size_t prefix_len = print_token_line(&token);
                    log_f("${}\n");

                    log_f("   ");
                    fput_repeat(stderr, ' ', prefix_len + 1);
                    fputs("^~ (unresolvable interpolation key)\n", stderr);

                    result.ok = false;
                    result.errcode = 1;
                    goto done;
                }

                result = commit_token(tokenizer, &token, INTERPOLATED, &value);
                if (!result.ok) {
                    goto done;
                }

                if (peek(tokenizer, 0) == LINE_DELIMITER) {
                    append_token(tokenizer, &token);
                    next_line(tokenizer);
                    skip_byte(tokenizer, 1); // '\n'
                    reset_byte(tokenizer);
                }
                value.count = 0;
                break;
            }

            case BACK_SLASH: {
                int n = peek(tokenizer, 1);
                if (n != -1 && n != LINE_DELIMITER) {
                    DYN_ARR_APPEND(&value, (char)current);
                    skip_byte(tokenizer, 1);
                    continue;
                }

                // line continuation: commit the current segment and keep tokenizing
                // the same token, so '$', '#', and '=' on continuation lines are
                // handled normally by the main loop
                if (value.count != 0) {
                    result = commit_token(tokenizer, &token, LITERAL, &value);
                    if (!result.ok) {
                        goto done;
                    }
                }
                value.count = 0;
                skip_byte(tokenizer, 2); // "\\\n"
                next_line(tokenizer);
                reset_byte(tokenizer);
                break;
            }

            default:
                scan_until(tokenizer, &value, LITERAL_STOPS, LITERAL_STOPS_LEN);
                break;
        }
    }

    // flush a pending token if the file doesn't end with a newline
    if (token.key != NULL) {
        if (value.count > 0 || token.values.count == 0) {
            result = commit_token(tokenizer, &token, LITERAL, &value);
            if (!result.ok) {
                goto done;
            }
        }

        append_token(tokenizer, &token);
    }

    if (tokenizer->tokens.count == 0) {
        log_error("[ERROR] Unable to generate tokens for %s. Ensure the .env file is valid by following the KEY=VALUE "
                  "spec; aborting.",
                  tokenizer->file_name);

        result.ok = false;
        result.errcode = 1;
        goto done;
    }

done:
    free_token(&token);
    free(value.items);
    return result;
}

result_t run_tokenizer(args_t *args, tokenizer_t *tokenizer) {
    result_t result = {.ok = true, .errcode = 0};

    for (size_t fi = 0; fi < args->files.count; ++fi) {
        const char *path = args->files.items[fi];

        file_details_t file = open_file(path, args->dry_run);
        if (file.contents == NULL) {
            result.ok = false;
            result.errcode = 1;
            return result;
        }

        result = generate_tokens(args, tokenizer, &file);
        free(file.contents);
        tokenizer->file = NULL;

        if (result.ok) {
            continue;
        }

        return result;
    }

    if (args->dry_run) {
        print_tokens(tokenizer);
    }

    return result;
}

void free_tokenizer(tokenizer_t *tokenizer) {
    for (size_t i = 0; i < tokenizer->tokens.count; ++i) {
        free_token(&tokenizer->tokens.items[i]);
    }
    free(tokenizer->tokens.items);
    tokenizer->tokens = (token_list_t){0};
}
