#include "arg.h"
#include "parser.h"
#include "test_capture.h"
#include "tokenizer.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

#if defined(_WIN32) && defined(_MSC_VER)
static void set_env(const char *k, const char *v) { _putenv_s(k, v); }
static void clear_env(const char *k) { _putenv_s(k, ""); }
#else
static void set_env(const char *k, const char *v) { setenv(k, v, 1); }
static void clear_env(const char *k) { unsetenv(k); }
#endif

typedef struct {
    const args_t *args;
    token_list_t *tl;
    env_map_t *envs;
    result_t result;
} parser_ctx_t;

static void call_parser(void *ctx) {
    parser_ctx_t *c = ctx;
    c->result = run_parser(c->args, c->tl, c->envs);
}

static result_t parser_silent(const args_t *args, token_list_t *tl, env_map_t *envs) {
    parser_ctx_t ctx = {.args = args, .tl = tl, .envs = envs};
    char sink[1];
    capture_fd(stderr, sink, sizeof(sink), call_parser, &ctx);
    return ctx.result;
}

static const char *lookup(const env_map_t *m, const char *key) {
    for (size_t i = 0; i < m->count; ++i) {
        if (strcmp(m->items[i].key, key) == 0) {
            return m->items[i].value;
        }
    }
    return NULL;
}

static token_t make_token(const char *key, value_kind_t kind, const char *value, value_token_t *slot) {
    *slot = (value_token_t){.value = (char *)value, .value_len = strlen(value), .kind = kind, .line = 1, .byte = 1};
    token_t tok = {.key = (char *)key, .file = "testp.env"};
    tok.values.items = slot;
    tok.values.count = 1;
    tok.values.capacity = 1;
    return tok;
}

static void test_sets_normalized_key_value(void) {
    value_token_t v;
    token_t toks[] = {make_token("KEY", LITERAL, "value", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};
    args_t args = {0};

    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, envs.count);
    TEST_ASSERT_EQUAL_STRING("value", lookup(&envs, "KEY"));
    free_envs(&envs);
}

static void test_concatenates_multiple_value_tokens(void) {
    value_token_t vs[2] = {
        {.value = (char *)"12", .value_len = 2, .kind = LITERAL, .line = 1, .byte = 1},
        {.value = (char *)"34", .value_len = 2, .kind = LITERAL, .line = 2, .byte = 1},
    };
    token_t tok = {.key = (char *)"MULTI", .file = "testp.env"};
    tok.values.items = vs;
    tok.values.count = 2;
    tok.values.capacity = 2;
    token_list_t tl = {.items = &tok, .count = 1, .capacity = 1};
    args_t args = {0};

    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("1234", lookup(&envs, "MULTI"));
    free_envs(&envs);
}

static void test_resolves_interpolation_from_environment(void) {
    set_env("NVI_TEST_HOME", "/home/test");

    value_token_t v;
    token_t toks[] = {make_token("DIR", INTERPOLATED, "NVI_TEST_HOME", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};
    args_t args = {0};

    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("/home/test", lookup(&envs, "DIR"));
    free_envs(&envs);

    clear_env("NVI_TEST_HOME");
}

static void test_resolves_interpolation_from_previous_env(void) {
    clear_env("NVI_TEST_FOO");

    value_token_t vfoo, vbaz;
    token_t toks[] = {
        make_token("NVI_TEST_FOO", LITERAL, "bar", &vfoo),
        make_token("NVI_TEST_BAZ", INTERPOLATED, "NVI_TEST_FOO", &vbaz),
    };
    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};
    args_t args = {0};

    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("bar", lookup(&envs, "NVI_TEST_BAZ"));
    free_envs(&envs);
}

static void test_required_env_present_passes(void) {
    value_token_t v;
    token_t toks[] = {make_token("REQUIRED", LITERAL, "ok", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    const char *req[] = {"REQUIRED"};
    args.required.items = (const char **)req;
    args.required.count = 1;
    const char *cmd[] = {"echo"};
    args.command.items = cmd;
    args.command.count = 1;

    env_map_t envs = {0};
    result_t r = parser_silent(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("ok", lookup(&envs, "REQUIRED"));
    free_envs(&envs);
}

static void test_errors_when_nothing_parses(void) {
    token_list_t tl = {0};
    args_t args = {0};
    env_map_t envs = {0};
    result_t r = parser_silent(&args, &tl, &envs);
    TEST_ASSERT_FALSE(r.ok);
    free_envs(&envs);
}

static void test_duplicate_key_updates_in_place(void) {
    value_token_t v1, v2;
    token_t toks[] = {
        make_token("KEY", LITERAL, "first", &v1),
        make_token("KEY", LITERAL, "second", &v2),
    };
    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};
    args_t args = {0};

    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, envs.count);
    TEST_ASSERT_EQUAL_STRING("second", lookup(&envs, "KEY"));
    free_envs(&envs);
}

static void test_index_survives_growth(void) {
    enum { N = 100 };
    static char keys[N][16];
    static value_token_t vs[N];
    static token_t toks[N];

    for (size_t i = 0; i < N; ++i) {
        snprintf(keys[i], sizeof(keys[i]), "KEY_%zu", i);
        toks[i] = make_token(keys[i], LITERAL, "v", &vs[i]);
    }

    token_list_t tl = {.items = toks, .count = N, .capacity = N};
    args_t args = {0};

    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(N, envs.count);

    env_t *first = get_env_from_map(&envs, "KEY_0");
    env_t *last = get_env_from_map(&envs, "KEY_99");
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(last);
    TEST_ASSERT_EQUAL_STRING("v", first->value);
    TEST_ASSERT_EQUAL_STRING("v", last->value);
    TEST_ASSERT_NULL(get_env_from_map(&envs, "KEY_100"));
    free_envs(&envs);
}

static void test_errors_when_required_env_missing(void) {
    value_token_t v;
    token_t toks[] = {make_token("OTHER", LITERAL, "x", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    const char *req[] = {"REQUIRED"};
    args.required.items = (const char **)req;
    args.required.count = 1;
    const char *cmd[] = {"echo"};
    args.command.items = cmd;
    args.command.count = 1;

    env_map_t envs = {0};
    result_t r = parser_silent(&args, &tl, &envs);
    TEST_ASSERT_FALSE(r.ok);
    free_envs(&envs);
}

static void test_fallback_used_when_unset(void) {
    clear_env("NVI_TEST_FB_UNSET");
    value_token_t v;
    token_t toks[] = {make_token("KEY", INTERPOLATED, "NVI_TEST_FB_UNSET:-fell back", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("fell back", lookup(&envs, "KEY"));
    free_envs(&envs);
}

static void test_fallback_used_when_map_value_empty(void) {
    value_token_t v1;
    value_token_t v2;
    token_t toks[] = {
        make_token("EMPTYSRC", LITERAL, "", &v1),
        make_token("KEY", INTERPOLATED, "EMPTYSRC:-fb", &v2),
    };
    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};

    args_t args = {0};
    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("fb", lookup(&envs, "KEY"));
    free_envs(&envs);
}

static void test_value_wins_over_fallback(void) {
    set_env("NVI_TEST_FB_SET", "real");
    value_token_t v;
    token_t toks[] = {make_token("KEY", INTERPOLATED, "NVI_TEST_FB_SET:-fb", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("real", lookup(&envs, "KEY"));
    clear_env("NVI_TEST_FB_SET");
    free_envs(&envs);
}

static void test_defined_empty_without_fallback_resolves_empty(void) {
    value_token_t v1;
    value_token_t v2;
    token_t toks[] = {
        make_token("EMPTYSRC", LITERAL, "", &v1),
        make_token("KEY", INTERPOLATED, "EMPTYSRC", &v2),
    };
    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};

    args_t args = {0};
    env_map_t envs = {0};
    result_t r = run_parser(&args, &tl, &envs);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("", lookup(&envs, "KEY"));
    free_envs(&envs);
}

static void test_errors_on_undefined_interpolation(void) {
    clear_env("NVI_TEST_UNDEF");
    value_token_t v;
    token_t toks[] = {make_token("KEY", INTERPOLATED, "NVI_TEST_UNDEF", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    env_map_t envs = {0};
    result_t r = parser_silent(&args, &tl, &envs);
    TEST_ASSERT_FALSE(r.ok);
    TEST_ASSERT_EQUAL_INT(1, r.code);
    free_envs(&envs);
}

static void test_errors_when_required_env_is_empty(void) {
    value_token_t v;
    token_t toks[] = {make_token("KEY", LITERAL, "", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    const char *req[] = {"KEY"};
    args.required.items = (const char **)req;
    args.required.count = 1;
    const char *cmd[] = {"echo"};
    args.command.items = cmd;
    args.command.count = 1;

    env_map_t envs = {0};
    result_t r = parser_silent(&args, &tl, &envs);
    TEST_ASSERT_FALSE(r.ok);
    free_envs(&envs);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sets_normalized_key_value);
    RUN_TEST(test_concatenates_multiple_value_tokens);
    RUN_TEST(test_resolves_interpolation_from_environment);
    RUN_TEST(test_resolves_interpolation_from_previous_env);
    RUN_TEST(test_required_env_present_passes);
    RUN_TEST(test_errors_when_nothing_parses);
    RUN_TEST(test_duplicate_key_updates_in_place);
    RUN_TEST(test_index_survives_growth);
    RUN_TEST(test_errors_when_required_env_missing);
    RUN_TEST(test_fallback_used_when_unset);
    RUN_TEST(test_fallback_used_when_map_value_empty);
    RUN_TEST(test_value_wins_over_fallback);
    RUN_TEST(test_defined_empty_without_fallback_resolves_empty);
    RUN_TEST(test_errors_on_undefined_interpolation);
    RUN_TEST(test_errors_when_required_env_is_empty);
    return UNITY_END();
}
