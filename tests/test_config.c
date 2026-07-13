#include "arena.h"
#include "arg.h"
#include "config.h"
#include "macros.h"
#include "test_capture.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

static arena_t test_arena;

void setUp(void) { test_arena = (arena_t){0}; }
void tearDown(void) { arena_free(&test_arena); }

static void write_config_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "cannot create config file fixture");
    fwrite(contents, 1, strlen(contents), f);
    fclose(f);
}

typedef struct {
    int argc;
    const char **argv;
    config_t config;
    result_t result;
} config_ctx_t;

static void call_load(void *ctx) {
    config_ctx_t *c = ctx;
    c->result = load_config_file(&test_arena, c->argc, c->argv, &c->config);
}

static config_ctx_t load_silent(int argc, const char **argv) {
    config_ctx_t ctx = {.argc = argc, .argv = argv};
    char sink[1];
    capture_fd(stderr, sink, sizeof(sink), call_load, &ctx);
    return ctx;
}

// --- passthrough ---

static void test_passthrough_without_config_file(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--dry-run"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_TRUE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(ARR_LEN(argv), c.config.argc);
    TEST_ASSERT_EQUAL_PTR(argv, c.config.argv);
    TEST_ASSERT_NULL(c.config.path);
}

static void test_at_after_delimiter_is_not_expanded(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--", "deploy", "@manifest"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_TRUE(c.result.ok);
    TEST_ASSERT_EQUAL_PTR(argv, c.config.argv);
    TEST_ASSERT_NULL(c.config.path);
}

// --- expansion ---

static void test_expands_tokens_at_splice_point(void) {
    write_config_file("build/cf_basic.nvi", "--files .env .env.local\n--threads 2\n");
    const char *argv[] = {"nvi", "@build/cf_basic.nvi", "--dry-run"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_TRUE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(7, c.config.argc);
    TEST_ASSERT_EQUAL_STRING("nvi", c.config.argv[0]);
    TEST_ASSERT_EQUAL_STRING("--files", c.config.argv[1]);
    TEST_ASSERT_EQUAL_STRING(".env", c.config.argv[2]);
    TEST_ASSERT_EQUAL_STRING(".env.local", c.config.argv[3]);
    TEST_ASSERT_EQUAL_STRING("--threads", c.config.argv[4]);
    TEST_ASSERT_EQUAL_STRING("2", c.config.argv[5]);
    TEST_ASSERT_EQUAL_STRING("--dry-run", c.config.argv[6]);
    TEST_ASSERT_EQUAL_STRING("build/cf_basic.nvi", c.config.path);
}

static void test_skips_comments_and_crlf(void) {
    write_config_file("build/cf_comments.nvi", "# project config\r\n--dry-run # trailing comment\r\n\r\n");
    const char *argv[] = {"nvi", "@build/cf_comments.nvi"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_TRUE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(2, c.config.argc);
    TEST_ASSERT_EQUAL_STRING("--dry-run", c.config.argv[1]);
}

static void test_errors_on_empty_config_file(void) {
    write_config_file("build/cf_empty.nvi", "");
    const char *argv[] = {"nvi", "@build/cf_empty.nvi", "--dry-run"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(1, c.result.code);
}

static void test_errors_on_comment_only_config_file(void) {
    write_config_file("build/cf_comment_only.nvi", "# nothing but comments\n# and blank lines\n\n");
    const char *argv[] = {"nvi", "@build/cf_comment_only.nvi", "--dry-run"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(1, c.result.code);
}

static void test_command_delimiter_survives_after_splice(void) {
    write_config_file("build/cf_cmd.nvi", "--files .env\n");
    const char *argv[] = {"nvi", "@build/cf_cmd.nvi", "--", "echo", "hi"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_TRUE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(6, c.config.argc);
    TEST_ASSERT_EQUAL_STRING("--", c.config.argv[3]);
    TEST_ASSERT_EQUAL_STRING("echo", c.config.argv[4]);
    TEST_ASSERT_EQUAL_STRING("hi", c.config.argv[5]);
}

// --- errors ---

static void test_errors_on_missing_file(void) {
    const char *argv[] = {"nvi", "@build/cf_does_not_exist.nvi"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(1, c.result.code);
}

static void test_errors_on_empty_path(void) {
    const char *argv[] = {"nvi", "@"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(2, c.result.code);
}

static void test_errors_on_missing_nvi_extension(void) {
    write_config_file("build/cf_wrong.conf", "--dry-run\n");
    const char *argv[] = {"nvi", "@build/cf_wrong.conf"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(1, c.result.code);
}

static void test_accepts_nvi_environment_variant(void) {
    write_config_file("build/.nvi.staging", "--dry-run\n");
    const char *argv[] = {"nvi", "@build/.nvi.staging"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_TRUE(c.result.ok);
    TEST_ASSERT_EQUAL_STRING("--dry-run", c.config.argv[1]);
}

static void test_errors_on_absolute_path(void) {
    const char *argv[] = {"nvi", "@/etc/.nvi"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(1, c.result.code);
}

static void test_errors_on_escaping_path(void) {
    const char *argv[] = {"nvi", "@../.nvi"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(1, c.result.code);
}

static void test_errors_on_multiple_config_files(void) {
    const char *argv[] = {"nvi", "@build/cf_basic.nvi", "@build/cf_cmd.nvi"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(2, c.result.code);
}

static void test_errors_on_embedded_command_delimiter(void) {
    write_config_file("build/cf_delim.nvi", "--files .env\n-- echo hi\n");
    const char *argv[] = {"nvi", "@build/cf_delim.nvi"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(2, c.result.code);
}

static void test_errors_on_nested_config_file(void) {
    write_config_file("build/cf_nested.nvi", "@build/cf_basic.nvi\n");
    const char *argv[] = {"nvi", "@build/cf_nested.nvi"};
    config_ctx_t c = load_silent(ARR_LEN(argv), argv);
    TEST_ASSERT_FALSE(c.result.ok);
    TEST_ASSERT_EQUAL_INT(2, c.result.code);
}

// --- end to end with parse_args ---

typedef struct {
    config_ctx_t cf;
    args_t args;
    result_t result;
} e2e_ctx_t;

static void call_load_then_parse(void *ctx) {
    e2e_ctx_t *c = ctx;
    call_load(&c->cf);
    c->result = c->cf.result;
    if (c->result.ok) {
        c->result = parse_args(&test_arena, &c->cf.config, &c->args);
    }
}

static void test_cli_flags_win_over_config_file(void) {
    write_config_file("build/cf_e2e.nvi", "--files .env\n--threads 1\n");
    const char *argv[] = {"nvi", "@build/cf_e2e.nvi", "--files", ".env.local", "--dry-run"};
    e2e_ctx_t c = {.cf = {.argc = ARR_LEN(argv), .argv = argv}};
    char sink[4096];
    capture_fd(stderr, sink, sizeof(sink), call_load_then_parse, &c);
    TEST_ASSERT_TRUE(c.result.ok);
    // config files parse first, CLI files append after and override during parsing
    TEST_ASSERT_EQUAL_size_t(2, c.args.files.count);
    TEST_ASSERT_EQUAL_STRING(".env", c.args.files.items[0]);
    TEST_ASSERT_EQUAL_STRING(".env.local", c.args.files.items[1]);
    TEST_ASSERT_TRUE(c.args.dry_run);
    TEST_ASSERT_EQUAL_UINT8(1, c.args.scan_threads);
    TEST_ASSERT_EQUAL_STRING("build/cf_e2e.nvi", c.args.config_path);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_passthrough_without_config_file);
    RUN_TEST(test_at_after_delimiter_is_not_expanded);
    RUN_TEST(test_expands_tokens_at_splice_point);
    RUN_TEST(test_skips_comments_and_crlf);
    RUN_TEST(test_errors_on_empty_config_file);
    RUN_TEST(test_errors_on_comment_only_config_file);
    RUN_TEST(test_command_delimiter_survives_after_splice);
    RUN_TEST(test_errors_on_missing_file);
    RUN_TEST(test_errors_on_empty_path);
    RUN_TEST(test_errors_on_missing_nvi_extension);
    RUN_TEST(test_accepts_nvi_environment_variant);
    RUN_TEST(test_errors_on_absolute_path);
    RUN_TEST(test_errors_on_escaping_path);
    RUN_TEST(test_errors_on_multiple_config_files);
    RUN_TEST(test_errors_on_embedded_command_delimiter);
    RUN_TEST(test_errors_on_nested_config_file);
    RUN_TEST(test_cli_flags_win_over_config_file);
    return UNITY_END();
}
