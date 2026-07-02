#include "emitter.h"
#include "arg.h"
#include "capture.h"
#include "format.h"
#include "parser.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include <io.h>
#define DUP _dup
#define DUP2 _dup2
#define FILENO _fileno
#define CLOSE _close
#else
#include <unistd.h>
#define DUP dup
#define DUP2 dup2
#define FILENO fileno
#define CLOSE close
#endif

void setUp(void) {}
void tearDown(void) {}

typedef struct {
    const args_t *args;
    const env_map_t *map;
} emit_ctx_t;

static void call_emitter(void *ctx) {
    emit_ctx_t *e = ctx;
    run_emitter(e->args, e->map);
}

static size_t capture_emit(const args_t *args, const env_map_t *map, char *out, size_t cap) {
    emit_ctx_t ctx = {args, map};
    return capture_fd(stdout, out, cap, call_emitter, &ctx);
}

static env_map_t map_of(env_t *items, size_t count) {
    env_map_t m = {.items = items, .count = count, .capacity = count};
    return m;
}

static args_t emit_args(format_t fmt, const char **cmd, size_t cmd_count) {
    args_t a = {0};
    a.format = fmt;
    a.command.items = cmd;
    a.command.count = cmd_count;
    return a;
}

static void test_nul_pairs_then_command(void) {
    env_t items[] = {{.key = "A", .value = (char *)"1"}, {.key = "B", .value = (char *)"two words"}};
    env_map_t m = map_of(items, 2);
    const char *cmd[] = {"echo", "hi"};
    args_t a = emit_args(FORMAT_NULL, cmd, 2);

    char buf[256];
    size_t n = capture_emit(&a, &m, buf, sizeof(buf));

    const char expected[] = "A=1\0B=two words\0echo\0hi\0";
    size_t exp_len = sizeof(expected) - 1;
    TEST_ASSERT_EQUAL_size_t(exp_len, n);
    TEST_ASSERT_EQUAL_MEMORY(expected, buf, exp_len);
}

static void test_nul_values_with_spaces_and_newlines(void) {
    env_t items[] = {{.key = "MULTI", .value = (char *)"line1\nline2"}};
    env_map_t m = map_of(items, 1);
    const char *cmd[] = {"env"};
    args_t a = emit_args(FORMAT_NULL, cmd, 1);

    char buf[256];
    size_t n = capture_emit(&a, &m, buf, sizeof(buf));

    const char expected[] = "MULTI=line1\nline2\0env\0";
    size_t exp_len = sizeof(expected) - 1;
    TEST_ASSERT_EQUAL_size_t(exp_len, n);
    TEST_ASSERT_EQUAL_MEMORY(expected, buf, exp_len);
}

static void test_nul_pairs_only_when_command_empty(void) {
    env_t items[] = {{.key = "A", .value = (char *)"1"}};
    env_map_t m = map_of(items, 1);
    args_t a = emit_args(FORMAT_NULL, NULL, 0);

    char buf[256];
    size_t n = capture_emit(&a, &m, buf, sizeof(buf));

    const char expected[] = "A=1\0";
    size_t exp_len = sizeof(expected) - 1;
    TEST_ASSERT_EQUAL_size_t(exp_len, n);
    TEST_ASSERT_EQUAL_MEMORY(expected, buf, exp_len);
}

static void test_powershell_assignments_then_call(void) {
    env_t items[] = {{.key = "A", .value = (char *)"1"}, {.key = "B", .value = (char *)"two words"}};
    env_map_t m = map_of(items, 2);
    const char *cmd[] = {"echo", "hi"};
    args_t a = emit_args(FORMAT_POWERSHELL, cmd, 2);

    char buf[256];
    size_t n = capture_emit(&a, &m, buf, sizeof(buf));
    buf[n < sizeof(buf) ? n : sizeof(buf) - 1] = '\0';

    TEST_ASSERT_EQUAL_STRING("$env:A = '1'\n$env:B = 'two words'\n& 'echo' 'hi'\n", buf);
}

static void test_powershell_escapes_single_quotes(void) {
    env_t items[] = {{.key = "MSG", .value = (char *)"it's o'clock"}};
    env_map_t m = map_of(items, 1);
    const char *cmd[] = {"env"};
    args_t a = emit_args(FORMAT_POWERSHELL, cmd, 1);

    char buf[256];
    size_t n = capture_emit(&a, &m, buf, sizeof(buf));
    buf[n < sizeof(buf) ? n : sizeof(buf) - 1] = '\0';

    TEST_ASSERT_EQUAL_STRING("$env:MSG = 'it''s o''clock'\n& 'env'\n", buf);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nul_pairs_then_command);
    RUN_TEST(test_nul_values_with_spaces_and_newlines);
    RUN_TEST(test_nul_pairs_only_when_command_empty);
    RUN_TEST(test_powershell_assignments_then_call);
    RUN_TEST(test_powershell_escapes_single_quotes);
    return UNITY_END();
}
