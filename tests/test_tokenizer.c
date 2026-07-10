#include "arena.h"
#include "arg.h"
#include "dynarr.h"
#include "file.h"
#include "test_capture.h"
#include "tokenizer.h"
#include "unity.h"
#include <string.h>

static arena_t test_arena;

void setUp(void) { test_arena = (arena_t){0}; }
void tearDown(void) { arena_free(&test_arena); }

static result_t tokenize(const char *src, tokenizer_t *out) {
    // one arena serves as both the persistent token store and the scratch value buffer;
    // tearDown releases everything, so per-test cleanup is gone
    args_t args = {0};
    file_details_t file = {.contents = (char *)src, .path = "test.env", .len = strlen(src)};
    *out = (tokenizer_t){0};
    return generate_tokens(&test_arena, &test_arena, &args, &file, out);
}

static const value_token_t *val(const tokenizer_t *t, size_t tok, size_t v) {
    return &t->tokens.items[tok].values.items[v];
}

typedef struct {
    const char *src;
    tokenizer_t *out;
    result_t result;
} tokenize_ctx_t;

static void call_tokenize(void *ctx) {
    tokenize_ctx_t *c = ctx;
    c->result = tokenize(c->src, c->out);
}

static result_t tokenize_capture_fd(const char *src, tokenizer_t *out) {
    tokenize_ctx_t ctx = {.src = src, .out = out};
    char sink[1];
    capture_fd(stderr, sink, sizeof(sink), call_tokenize, &ctx);
    return ctx.result;
}

static void test_simple_key_value(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=value\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_INT(LITERAL_VALUE, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("value", val(&t, 0, 0)->value);
}

static void test_key_value_without_trailing_newline(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=value", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_STRING("value", val(&t, 0, 0)->value);
}

static void test_multiline_continuation(void) {
    tokenizer_t t;
    result_t r = tokenize("A=123\\\n456\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_STRING("A", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_size_t(2, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_STRING("123", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("456", val(&t, 0, 1)->value);
    TEST_ASSERT_EQUAL_size_t(1, val(&t, 0, 0)->line);
    TEST_ASSERT_EQUAL_size_t(2, val(&t, 0, 1)->line);
}

static void test_interpolation_inside_multiline(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=abc\\\n${OTHER}def\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_size_t(3, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_INT(LITERAL_VALUE, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("abc", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_INT(INTERPOLATED_KEY, val(&t, 0, 1)->kind);
    TEST_ASSERT_EQUAL_STRING("OTHER", val(&t, 0, 1)->value);
    TEST_ASSERT_EQUAL_INT(LITERAL_VALUE, val(&t, 0, 2)->kind);
    TEST_ASSERT_EQUAL_STRING("def", val(&t, 0, 2)->value);
}

static void test_multiline_ssh_key_with_interp_and_literals(void) {
    tokenizer_t t;
    result_t r = tokenize("MULTI=ssh-rsa ABC\\\ng3HI$\\\n+jk${MESSAGE}/4\\\nLm5Mn== test@example.com\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_STRING("MULTI", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_size_t(6, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_STRING("ssh-rsa ABC", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("g3HI$", val(&t, 0, 1)->value);
    TEST_ASSERT_EQUAL_STRING("+jk", val(&t, 0, 2)->value);
    TEST_ASSERT_EQUAL_INT(INTERPOLATED_KEY, val(&t, 0, 3)->kind);
    TEST_ASSERT_EQUAL_STRING("MESSAGE", val(&t, 0, 3)->value);
    TEST_ASSERT_EQUAL_STRING("/4", val(&t, 0, 4)->value);
    TEST_ASSERT_EQUAL_STRING("Lm5Mn== test@example.com", val(&t, 0, 5)->value);
}

static void test_equals_is_literal_after_key(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=a==b\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_STRING("a==b", val(&t, 0, 0)->value);
}

static void test_parses_a_comment(void) {
    tokenizer_t t;
    result_t r = tokenize("# a comment\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_NULL(t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_INT(COMMENTED_LINE, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("# a comment", val(&t, 0, 0)->value);
}

static void test_parses_interpolated_value(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=${OTHER}\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_INT(INTERPOLATED_KEY, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("OTHER", val(&t, 0, 0)->value);
}

// --- error paths (these intentionally log diagnostics to stderr) ---

static void test_errors_on_unterminated_interpolation(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=${OTHER\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_unterminated_interpolation_eof(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=${OTHER", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_when_no_tokens_generated(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("novalue\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_empty_key(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("=abc123\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_whitespace_only_key(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("   =value\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_empty_interpolation_key(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=abc${}123\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

// --- Windows-style input (CRLF, BOM) ---

static void test_crlf_line_endings(void) {
    tokenizer_t t;
    result_t r = tokenize("A=1\r\nB=2\r\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, t.tokens.count);
    TEST_ASSERT_EQUAL_STRING("1", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("2", val(&t, 1, 0)->value);
    TEST_ASSERT_EQUAL_size_t(2, val(&t, 1, 0)->line);
}

static void test_lone_carriage_return_is_literal(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=a\rb\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("a\rb", val(&t, 0, 0)->value);
}

static void test_crlf_multiline_continuation(void) {
    tokenizer_t t;
    result_t r = tokenize("A=123\\\r\n456\r\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_size_t(2, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_STRING("123", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("456", val(&t, 0, 1)->value);
}

static void test_crlf_after_interpolation(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=${OTHER}\r\nB=2\r\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, t.tokens.count);
    TEST_ASSERT_EQUAL_INT(INTERPOLATED_KEY, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("OTHER", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("2", val(&t, 1, 0)->value);
}

static void test_crlf_comment(void) {
    tokenizer_t t;
    result_t r = tokenize("# a comment\r\nA=1\r\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, t.tokens.count);
    TEST_ASSERT_EQUAL_INT(COMMENTED_LINE, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("# a comment", val(&t, 0, 0)->value);
}

static void test_errors_on_unterminated_interpolation_crlf(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=${OTHER\r\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_utf8_bom_is_stripped(void) {
    tokenizer_t t;
    result_t r = tokenize("\xEF\xBB\xBFKEY=value\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_STRING("value", val(&t, 0, 0)->value);
}

typedef struct {
    const args_t *args;
    tokenizer_t *t;
    result_t result;
} run_ctx_t;

static void call_run_tokenizer(void *ctx) {
    run_ctx_t *c = ctx;
    c->result = run_tokenizer(&test_arena, c->args, c->t);
}

static void test_run_tokenizer_errors_on_empty_file(void) {
    const char *path = "tokenizer_test_empty.env";
    FILE *f = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(f);
    fclose(f);

    args_t args = {0};
    DYN_ARR_APPEND(&test_arena, &args.files, path);

    tokenizer_t t = {0};
    run_ctx_t ctx = {.args = &args, .t = &t};
    char sink[1];
    capture_fd(stderr, sink, sizeof(sink), call_run_tokenizer, &ctx);

    TEST_ASSERT_FALSE(ctx.result.ok);
    TEST_ASSERT_EQUAL_INT(1, ctx.result.code);

    remove(path);
}

// --- quoted values ---

static void test_double_quotes_are_stripped(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=\"hello world\"\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_STRING("hello world", val(&t, 0, 0)->value);
}

static void test_double_quotes_preserve_inner_whitespace(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=\"  padded  \"\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("  padded  ", val(&t, 0, 0)->value);
}

static void test_single_quotes_suppress_interpolation(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY='${OTHER}'\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_INT(LITERAL_VALUE, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("${OTHER}", val(&t, 0, 0)->value);
}

static void test_single_quotes_keep_backslashes_literal(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY='a\\b'\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("a\\b", val(&t, 0, 0)->value);
}

static void test_interpolation_inside_double_quotes(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=\"${A}b\"\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_INT(INTERPOLATED_KEY, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("A", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("b", val(&t, 0, 1)->value);
}

static void test_continuation_inside_double_quotes(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=\"a\\\nb\"\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_STRING("a", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("b", val(&t, 0, 1)->value);
}

static void test_quoted_empty_values_are_allowed(void) {
    tokenizer_t t;
    result_t r = tokenize("A=\"\"\nB=''\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, t.tokens.count);
    TEST_ASSERT_EQUAL_STRING("", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("", val(&t, 1, 0)->value);
}

static void test_mid_value_quotes_stay_literal(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=it's\nQ=sad\"wow\"bak\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("it's", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_STRING("sad\"wow\"bak", val(&t, 1, 0)->value);
}

static void test_trailing_whitespace_after_closing_quote_ok(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=\"a\"  \n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("a", val(&t, 0, 0)->value);
}

static void test_errors_on_unterminated_quote(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=\"abc\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_unterminated_quote_eof(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY='abc", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_content_after_closing_quote(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=\"a\"b\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

// --- key handling ---

static void test_export_prefix_is_stripped(void) {
    tokenizer_t t;
    result_t r = tokenize("export KEY=value\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_STRING("value", val(&t, 0, 0)->value);
}

static void test_export_alone_is_a_key(void) {
    tokenizer_t t;
    result_t r = tokenize("export=value\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("export", t.tokens.items[0].key);
}

static void test_errors_on_key_with_space(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("MY KEY=1\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_key_starting_with_digit(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("1KEY=x\n", &t);
    TEST_ASSERT_FALSE(r.ok);
}

typedef struct {
    const char *src;
    size_t len;
    tokenizer_t *out;
    result_t result;
} tokenize_n_ctx_t;

static void call_tokenize_n(void *ctx) {
    tokenize_n_ctx_t *c = ctx;
    args_t args = {0};
    file_details_t file = {.contents = (char *)c->src, .path = "test.env", .len = c->len};
    *c->out = (tokenizer_t){0};
    c->result = generate_tokens(&test_arena, &test_arena, &args, &file, c->out);
}

static void test_errors_on_nul_inside_interpolation(void) {
    static const char src[] = "KEY=${A\0B}\n";
    tokenizer_t t;
    tokenize_n_ctx_t ctx = {.src = src, .len = sizeof(src) - 1, .out = &t};
    char sink[1];
    capture_fd(stderr, sink, sizeof(sink), call_tokenize_n, &ctx);
    TEST_ASSERT_FALSE(ctx.result.ok);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_simple_key_value);
    RUN_TEST(test_key_value_without_trailing_newline);
    RUN_TEST(test_multiline_continuation);
    RUN_TEST(test_interpolation_inside_multiline);
    RUN_TEST(test_multiline_ssh_key_with_interp_and_literals);
    RUN_TEST(test_equals_is_literal_after_key);
    RUN_TEST(test_parses_a_comment);
    RUN_TEST(test_parses_interpolated_value);
    RUN_TEST(test_errors_on_unterminated_interpolation);
    RUN_TEST(test_errors_on_unterminated_interpolation_eof);
    RUN_TEST(test_errors_when_no_tokens_generated);
    RUN_TEST(test_errors_on_empty_key);
    RUN_TEST(test_errors_on_whitespace_only_key);
    RUN_TEST(test_errors_on_empty_interpolation_key);
    RUN_TEST(test_crlf_line_endings);
    RUN_TEST(test_lone_carriage_return_is_literal);
    RUN_TEST(test_crlf_multiline_continuation);
    RUN_TEST(test_crlf_after_interpolation);
    RUN_TEST(test_crlf_comment);
    RUN_TEST(test_errors_on_unterminated_interpolation_crlf);
    RUN_TEST(test_utf8_bom_is_stripped);
    RUN_TEST(test_run_tokenizer_errors_on_empty_file);
    RUN_TEST(test_double_quotes_are_stripped);
    RUN_TEST(test_double_quotes_preserve_inner_whitespace);
    RUN_TEST(test_single_quotes_suppress_interpolation);
    RUN_TEST(test_single_quotes_keep_backslashes_literal);
    RUN_TEST(test_interpolation_inside_double_quotes);
    RUN_TEST(test_continuation_inside_double_quotes);
    RUN_TEST(test_quoted_empty_values_are_allowed);
    RUN_TEST(test_mid_value_quotes_stay_literal);
    RUN_TEST(test_trailing_whitespace_after_closing_quote_ok);
    RUN_TEST(test_errors_on_unterminated_quote);
    RUN_TEST(test_errors_on_unterminated_quote_eof);
    RUN_TEST(test_errors_on_content_after_closing_quote);
    RUN_TEST(test_export_prefix_is_stripped);
    RUN_TEST(test_export_alone_is_a_key);
    RUN_TEST(test_errors_on_key_with_space);
    RUN_TEST(test_errors_on_key_starting_with_digit);
    RUN_TEST(test_errors_on_nul_inside_interpolation);
    return UNITY_END();
}
