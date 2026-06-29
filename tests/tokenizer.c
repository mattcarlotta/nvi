#include "tokenizer.h"
#include "arg.h"
#include "capture.h"
#include "file.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static result_t tokenize(const char *src, tokenizer_t *out) {
    args_t args = {0};
    file_details_t file = {.contents = (char *)src, .path = "test.env", .len = strlen(src)};
    *out = (tokenizer_t){0};
    return generate_tokens(&args, &file, out);
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
    TEST_ASSERT_EQUAL_INT(LITERAL, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("value", val(&t, 0, 0)->value);
    free_tokenizer(&t);
}

static void test_key_value_without_trailing_newline(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=value", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_STRING("value", val(&t, 0, 0)->value);
    free_tokenizer(&t);
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
    free_tokenizer(&t);
}

static void test_interpolation_inside_multiline(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=abc\\\n${OTHER}def\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_EQUAL_size_t(3, t.tokens.items[0].values.count);
    TEST_ASSERT_EQUAL_INT(LITERAL, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("abc", val(&t, 0, 0)->value);
    TEST_ASSERT_EQUAL_INT(INTERPOLATED, val(&t, 0, 1)->kind);
    TEST_ASSERT_EQUAL_STRING("OTHER", val(&t, 0, 1)->value);
    TEST_ASSERT_EQUAL_INT(LITERAL, val(&t, 0, 2)->kind);
    TEST_ASSERT_EQUAL_STRING("def", val(&t, 0, 2)->value);
    free_tokenizer(&t);
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
    TEST_ASSERT_EQUAL_INT(INTERPOLATED, val(&t, 0, 3)->kind);
    TEST_ASSERT_EQUAL_STRING("MESSAGE", val(&t, 0, 3)->value);
    TEST_ASSERT_EQUAL_STRING("/4", val(&t, 0, 4)->value);
    TEST_ASSERT_EQUAL_STRING("Lm5Mn== test@example.com", val(&t, 0, 5)->value);
    free_tokenizer(&t);
}

static void test_equals_is_literal_after_key(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=a==b\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_STRING("a==b", val(&t, 0, 0)->value);
    free_tokenizer(&t);
}

static void test_parses_a_comment(void) {
    tokenizer_t t;
    result_t r = tokenize("# a comment\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, t.tokens.count);
    TEST_ASSERT_NULL(t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_INT(COMMENTED, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("# a comment", val(&t, 0, 0)->value);
    free_tokenizer(&t);
}

static void test_parses_interpolated_value(void) {
    tokenizer_t t;
    result_t r = tokenize("KEY=${OTHER}\n", &t);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("KEY", t.tokens.items[0].key);
    TEST_ASSERT_EQUAL_INT(INTERPOLATED, val(&t, 0, 0)->kind);
    TEST_ASSERT_EQUAL_STRING("OTHER", val(&t, 0, 0)->value);
    free_tokenizer(&t);
}

// --- error paths (these intentionally log diagnostics to stderr) ---

static void test_errors_on_unterminated_interpolation(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=${OTHER\n", &t);
    TEST_ASSERT_FALSE(r.ok);
    free_tokenizer(&t);
}

static void test_errors_on_unterminated_interpolation_eof(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=${OTHER", &t);
    TEST_ASSERT_FALSE(r.ok);
    free_tokenizer(&t);
}

static void test_errors_when_no_tokens_generated(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("novalue\n", &t);
    TEST_ASSERT_FALSE(r.ok);
    free_tokenizer(&t);
}

static void test_errors_on_empty_key(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("=abc123\n", &t);
    TEST_ASSERT_FALSE(r.ok);
    free_tokenizer(&t);
}

static void test_errors_on_whitespace_only_key(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("   =value\n", &t);
    TEST_ASSERT_FALSE(r.ok);
    free_tokenizer(&t);
}

static void test_errors_on_empty_interpolation_key(void) {
    tokenizer_t t;
    result_t r = tokenize_capture_fd("KEY=abc${}123\n", &t);
    TEST_ASSERT_FALSE(r.ok);
    free_tokenizer(&t);
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
    return UNITY_END();
}
