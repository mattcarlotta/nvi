#include "arena.h"
#include "arg.h"
#include "config.h"
#include "format.h"
#include "macros.h"
#include "nthread.h"
#include "test_capture.h"
#include "unity.h"
#include <string.h>

static arena_t test_arena;

void setUp(void) { test_arena = (arena_t){0}; }
void tearDown(void) { arena_free(&test_arena); }

typedef struct {
    config_t *config;
    args_t *a;
    result_t result;
} arg_ctx_t;

static void call_parse_args(void *ctx) {
    arg_ctx_t *c = ctx;
    c->result = parse_args(&test_arena, c->config, c->a);
}

// wraps a raw argv in a passthrough config_t, mirroring what load_config_file
// produces when no '@<path>' argument is present
static config_t as_config(int argc, const char **argv) { return (config_t){.argc = argc, .argv = argv}; }

static result_t parse_args_silent(int argc, const char **argv, args_t *a) {
    config_t config = as_config(argc, argv);
    arg_ctx_t ctx = {.config = &config, .a = a};
    char sink[1];
    capture_fd(stderr, sink, sizeof(sink), call_parse_args, &ctx);
    return ctx.result;
}

static result_t parse_args_direct(int argc, const char **argv, args_t *a) {
    config_t config = as_config(argc, argv);
    return parse_args(&test_arena, &config, a);
}

static void test_parses_multiple_files(void) {
    const char *argv[] = {"nvi", "--files", ".env.local", ".env"};
    args_t a = {0};
    result_t r = parse_args_direct(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, a.files.count);
    TEST_ASSERT_EQUAL_STRING(".env.local", a.files.items[0]);
    TEST_ASSERT_EQUAL_STRING(".env", a.files.items[1]);
}

static void test_errors_on_file_missing_env_extension(void) {
    const char *argv[] = {"nvi", "--files", "example"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_absolute_file_path(void) {
    const char *argv[] = {"nvi", "--files", "/home/.env"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_escaping_file_path(void) {
    const char *argv[] = {"nvi", "--files", "../.env"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_errors_on_missing_files_param(void) {
    const char *argv[] = {"nvi", "--files"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_FALSE(r.ok);
}

// --- format ---

static void test_parses_format_flag(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--format", "powershell"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_INT(FORMAT_POWERSHELL, a.format);
}

static void test_errors_on_invalid_format(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--format", "json"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_FALSE(r.ok);
}

// --- required / ignored ---

static void test_parses_required_envs(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--required", "ENV_1", "ENV_2"};
    args_t a = {0};
    result_t r = parse_args_direct(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, a.required.count);
    TEST_ASSERT_EQUAL_STRING("ENV_1", a.required.items[0]);
    TEST_ASSERT_EQUAL_STRING("ENV_2", a.required.items[1]);
}

static void test_parses_ignored_envs(void) {
    const char *argv[] = {"nvi", "scan", "mjs", "--ignored", "NODE_ENV"};
    args_t a = {0};
    result_t r = parse_args_direct(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, a.ignored.count);
    TEST_ASSERT_EQUAL_STRING("NODE_ENV", a.ignored.items[0]);
}

// -- threads --

static void test_parses_threads_flag(void) {
    // thread_count() is the parser's upper bound, so it's the largest portable value;
    // a hardcoded "2" fails on single-core machines
    char threads[4];
    int max = thread_count();
    snprintf(threads, sizeof(threads), "%d", max);

    const char *argv[] = {"nvi", "--scan", "c", "--threads", threads, "--", "echo", "hello"};
    args_t a = {0};
    result_t r = parse_args_direct(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t((size_t)max, a.scan_threads);
}

static void test_errors_on_invalid_threads_flag(void) {
    const char *argv[] = {"nvi", "--threads", "abc"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_FALSE(r.ok);
}

// --- scan ---

static void test_parses_scan_extensions(void) {
    const char *argv[] = {"nvi", "scan", "ts", "mjs"};
    args_t a = {0};
    result_t r = parse_args_direct(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, a.scan_exts.count);
    TEST_ASSERT_EQUAL_STRING("ts", a.scan_exts.items[0].ext);
    TEST_ASSERT_EQUAL_STRING("mjs", a.scan_exts.items[1].ext);
}

static void test_errors_on_unsupported_scan_extension(void) {
    const char *argv[] = {"nvi", "scan", "index.mjs"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_FALSE(r.ok);
}

// --- dry-run, delimiter, unknown, help/version ---

static void test_parses_dry_run_flag(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--dry-run"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_TRUE(a.dry_run);
}

static void test_reveal_defaults_to_false(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--dry-run"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_FALSE(a.reveal);
}

static void test_parses_reveal_flag(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--dry-run", "--reveal"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_TRUE(a.reveal);
}

static void test_parses_short_reveal_flag(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--dry-run", "-R"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_TRUE(a.reveal);
}

static void test_copies_config_path_from_config(void) {
    const char *argv[] = {"nvi", "--files", ".env"};
    args_t a = {0};
    config_t config = as_config(ARR_LEN(argv), argv);
    config.path = ".nvi";
    result_t r = parse_args(&test_arena, &config, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING(".nvi", a.config_path);
}

static void test_config_path_defaults_to_null(void) {
    const char *argv[] = {"nvi", "--files", ".env"};
    args_t a = {0};
    result_t r = parse_args_direct(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_NULL(a.config_path);
}

static void test_delimiter_sets_command(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--", "echo", "hi"};
    args_t a = {0};
    result_t r = parse_args_direct(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, a.command.count);
    TEST_ASSERT_EQUAL_STRING("echo", a.command.items[0]);
    TEST_ASSERT_EQUAL_STRING("hi", a.command.items[1]);
}

static void test_errors_on_unknown_flag(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--unknown", "x"};
    args_t a = {0};
    result_t r = parse_args_silent(ARR_LEN(argv), argv, &a);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_help_short_circuits(void) {
    const char *argv[] = {"nvi", "help"};
    args_t a = {0};
    config_t config = as_config(ARR_LEN(argv), argv);
    arg_ctx_t ctx = {.config = &config, .a = &a};
    char sink[1];
    capture_fd(stdout, sink, sizeof(sink), call_parse_args, &ctx);
    TEST_ASSERT_FALSE(ctx.result.ok);
}

static void test_version_short_circuits(void) {
    const char *argv[] = {"nvi", "version"};
    args_t a = {0};
    config_t config = as_config(ARR_LEN(argv), argv);
    arg_ctx_t ctx = {.config = &config, .a = &a};
    char sink[1];
    capture_fd(stdout, sink, sizeof(sink), call_parse_args, &ctx);
    TEST_ASSERT_FALSE(ctx.result.ok);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parses_multiple_files);
    RUN_TEST(test_errors_on_file_missing_env_extension);
    RUN_TEST(test_errors_on_absolute_file_path);
    RUN_TEST(test_errors_on_escaping_file_path);
    RUN_TEST(test_errors_on_missing_files_param);
    RUN_TEST(test_parses_format_flag);
    RUN_TEST(test_errors_on_invalid_format);
    RUN_TEST(test_parses_required_envs);
    RUN_TEST(test_parses_ignored_envs);
    RUN_TEST(test_parses_threads_flag);
    RUN_TEST(test_errors_on_invalid_threads_flag);
    RUN_TEST(test_parses_scan_extensions);
    RUN_TEST(test_errors_on_unsupported_scan_extension);
    RUN_TEST(test_parses_dry_run_flag);
    RUN_TEST(test_reveal_defaults_to_false);
    RUN_TEST(test_parses_reveal_flag);
    RUN_TEST(test_parses_short_reveal_flag);
    RUN_TEST(test_copies_config_path_from_config);
    RUN_TEST(test_config_path_defaults_to_null);
    RUN_TEST(test_delimiter_sets_command);
    RUN_TEST(test_errors_on_unknown_flag);
    RUN_TEST(test_help_short_circuits);
    RUN_TEST(test_version_short_circuits);
    return UNITY_END();
}
