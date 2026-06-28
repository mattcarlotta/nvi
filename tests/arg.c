#include "arg.h"
#include "format.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// parse_args treats argv[0] as the program name (parsing starts at index 1),
// so every argv below begins with "nvi". argc is the element count.
//
// NOTE on C-vs-Zig divergences exercised here:
//   - The C parser requires at least one of --files / --scan; a bare
//     "--dry-run -- cmd" invocation is a usage error in C (it was accepted in
//     Zig), so the dry-run case below pairs it with --files.
//   - An unknown flag is a hard usage error in C (it was a soft warning in Zig).

static args_t run(const char **argv, int argc, result_t *r) {
    args_t args = {0};
    *r = parse_args(argc, argv, &args);
    return args;
}

// --- files ---

static void test_parses_multiple_files(void) {
    const char *argv[] = {"nvi", "--files", ".env.local", ".env"};
    result_t r;
    args_t a = run(argv, 4, &r);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, a.files.count);
    TEST_ASSERT_EQUAL_STRING(".env.local", a.files.items[0]);
    TEST_ASSERT_EQUAL_STRING(".env", a.files.items[1]);
    free_args(&a);
}

static void test_errors_on_file_missing_env_extension(void) {
    const char *argv[] = {"nvi", "--files", "example"};
    result_t r;
    args_t a = run(argv, 3, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
}

static void test_errors_on_absolute_file_path(void) {
    const char *argv[] = {"nvi", "--files", "/home/.env"};
    result_t r;
    args_t a = run(argv, 3, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
}

static void test_errors_on_escaping_file_path(void) {
    const char *argv[] = {"nvi", "--files", "../.env"};
    result_t r;
    args_t a = run(argv, 3, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
}

static void test_errors_on_missing_files_param(void) {
    const char *argv[] = {"nvi", "--files"};
    result_t r;
    args_t a = run(argv, 2, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
}

// --- format ---

static void test_parses_format_flag(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--format", "powershell"};
    result_t r;
    args_t a = run(argv, 5, &r);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_INT(FORMAT_POWERSHELL, a.format);
    free_args(&a);
}

static void test_errors_on_invalid_format(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--format", "json"};
    result_t r;
    args_t a = run(argv, 5, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
}

// --- required / ignored ---

static void test_parses_required_envs(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--required", "ENV_1", "ENV_2"};
    result_t r;
    args_t a = run(argv, 6, &r);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, a.required.count);
    TEST_ASSERT_EQUAL_STRING("ENV_1", a.required.items[0]);
    TEST_ASSERT_EQUAL_STRING("ENV_2", a.required.items[1]);
    free_args(&a);
}

static void test_parses_ignored_envs(void) {
    const char *argv[] = {"nvi", "scan", "mjs", "--ignored", "NODE_ENV"};
    result_t r;
    args_t a = run(argv, 5, &r);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, a.ignored.count);
    TEST_ASSERT_EQUAL_STRING("NODE_ENV", a.ignored.items[0]);
    free_args(&a);
}

// --- scan ---

static void test_parses_scan_extensions(void) {
    const char *argv[] = {"nvi", "scan", "ts", "mjs"};
    result_t r;
    args_t a = run(argv, 4, &r);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, a.scan_exts.count);
    TEST_ASSERT_EQUAL_STRING("ts", a.scan_exts.items[0].ext);
    TEST_ASSERT_EQUAL_STRING("mjs", a.scan_exts.items[1].ext);
    free_args(&a);
}

static void test_errors_on_unsupported_scan_extension(void) {
    const char *argv[] = {"nvi", "scan", "index.mjs"};
    result_t r;
    args_t a = run(argv, 3, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
}

// --- dry-run, delimiter, unknown, help/version ---

static void test_parses_dry_run_flag(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--dry-run"};
    result_t r;
    args_t a = run(argv, 4, &r);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_TRUE(a.dry_run);
    free_args(&a);
}

static void test_delimiter_sets_command(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--", "echo", "hi"};
    result_t r;
    args_t a = run(argv, 6, &r);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(2, a.command.count);
    TEST_ASSERT_EQUAL_STRING("echo", a.command.items[0]);
    TEST_ASSERT_EQUAL_STRING("hi", a.command.items[1]);
    free_args(&a);
}

static void test_errors_on_unknown_flag(void) {
    const char *argv[] = {"nvi", "--files", ".env", "--unknown", "x"};
    result_t r;
    args_t a = run(argv, 5, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
}

static void test_help_short_circuits(void) {
    const char *argv[] = {"nvi", "help"};
    result_t r;
    args_t a = run(argv, 2, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
}

static void test_version_short_circuits(void) {
    const char *argv[] = {"nvi", "version"};
    result_t r;
    args_t a = run(argv, 2, &r);
    TEST_ASSERT_FALSE(r.ok);
    free_args(&a);
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
    RUN_TEST(test_parses_scan_extensions);
    RUN_TEST(test_errors_on_unsupported_scan_extension);
    RUN_TEST(test_parses_dry_run_flag);
    RUN_TEST(test_delimiter_sets_command);
    RUN_TEST(test_errors_on_unknown_flag);
    RUN_TEST(test_help_short_circuits);
    RUN_TEST(test_version_short_circuits);
    return UNITY_END();
}
