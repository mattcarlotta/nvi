#include "arena.h"
#include "arg.h"
#include "parser.h"
#include "test_capture.h"
#include "tokenizer.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

static arena_t test_arena;

void setUp(void) { test_arena = (arena_t){0}; }
void tearDown(void) { arena_free(&test_arena); }

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
    parser_t *parser;
    result_t result;
} parser_ctx_t;

static void call_parser(void *ctx) {
    parser_ctx_t *c = ctx;
    c->result = run_parser(&test_arena, c->args, c->tl, c->parser);
}

static result_t parser_silent(const args_t *args, token_list_t *tl, parser_t *parser) {
    parser_ctx_t ctx = {.args = args, .tl = tl, .parser = parser};
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
    token_t toks[] = {make_token("KEY", LITERAL_VALUE, "value", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};
    args_t args = {0};

    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, parser.env_map.count);
    TEST_ASSERT_EQUAL_STRING("value", lookup(&parser.env_map, "KEY"));
}

static void test_concatenates_multiple_value_tokens(void) {
    value_token_t vs[2] = {
        {.value = (char *)"12", .value_len = 2, .kind = LITERAL_VALUE, .line = 1, .byte = 1},
        {.value = (char *)"34", .value_len = 2, .kind = LITERAL_VALUE, .line = 2, .byte = 1},
    };
    token_t tok = {.key = (char *)"MULTI", .file = "testp.env"};
    tok.values.items = vs;
    tok.values.count = 2;
    tok.values.capacity = 2;
    token_list_t tl = {.items = &tok, .count = 1, .capacity = 1};
    args_t args = {0};

    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("1234", lookup(&parser.env_map, "MULTI"));
}

static void test_resolves_interpolation_from_environment(void) {
    set_env("NVI_TEST_HOME", "/home/test");

    value_token_t v;
    token_t toks[] = {make_token("DIR", INTERPOLATED_KEY, "NVI_TEST_HOME", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};
    args_t args = {0};

    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("/home/test", lookup(&parser.env_map, "DIR"));

    clear_env("NVI_TEST_HOME");
}

static void test_resolves_interpolation_from_previous_env(void) {
    clear_env("NVI_TEST_FOO");

    value_token_t vfoo, vbaz;
    token_t toks[] = {
        make_token("NVI_TEST_FOO", LITERAL_VALUE, "bar", &vfoo),
        make_token("NVI_TEST_BAZ", INTERPOLATED_KEY, "NVI_TEST_FOO", &vbaz),
    };
    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};
    args_t args = {0};

    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("bar", lookup(&parser.env_map, "NVI_TEST_BAZ"));
}

static void test_required_env_present_passes(void) {
    value_token_t v;
    token_t toks[] = {make_token("REQUIRED", LITERAL_VALUE, "ok", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    const char *req[] = {"REQUIRED"};
    args.required.items = (const char **)req;
    args.required.count = 1;
    const char *cmd[] = {"echo"};
    args.command.items = cmd;
    args.command.count = 1;

    parser_t parser = {0};
    result_t r = parser_silent(&args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("ok", lookup(&parser.env_map, "REQUIRED"));
}

static void test_errors_when_nothing_parses(void) {
    token_list_t tl = {0};
    args_t args = {0};
    parser_t parser = {0};
    result_t r = parser_silent(&args, &tl, &parser);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_duplicate_key_updates_in_place(void) {
    value_token_t v1, v2;
    token_t toks[] = {
        make_token("KEY", LITERAL_VALUE, "first", &v1),
        make_token("KEY", LITERAL_VALUE, "second", &v2),
    };
    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};
    args_t args = {0};

    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(1, parser.env_map.count);
    TEST_ASSERT_EQUAL_STRING("second", lookup(&parser.env_map, "KEY"));
}

static void test_index_survives_growth(void) {
    enum { N = 100 };
    static char keys[N][16];
    static value_token_t vs[N];
    static token_t toks[N];

    for (size_t i = 0; i < N; ++i) {
        snprintf(keys[i], sizeof(keys[i]), "KEY_%zu", i);
        toks[i] = make_token(keys[i], LITERAL_VALUE, "v", &vs[i]);
    }

    token_list_t tl = {.items = toks, .count = N, .capacity = N};
    args_t args = {0};

    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(N, parser.env_map.count);

    env_t *first = get_env_from_map(&parser.env_map, "KEY_0");
    env_t *last = get_env_from_map(&parser.env_map, "KEY_99");
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(last);
    TEST_ASSERT_EQUAL_STRING("v", first->value);
    TEST_ASSERT_EQUAL_STRING("v", last->value);
    TEST_ASSERT_NULL(get_env_from_map(&parser.env_map, "KEY_100"));
}

static void test_errors_when_required_env_missing(void) {
    value_token_t v;
    token_t toks[] = {make_token("OTHER", LITERAL_VALUE, "x", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    const char *req[] = {"REQUIRED"};
    args.required.items = (const char **)req;
    args.required.count = 1;
    const char *cmd[] = {"echo"};
    args.command.items = cmd;
    args.command.count = 1;

    parser_t parser = {0};
    result_t r = parser_silent(&args, &tl, &parser);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_fallback_used_when_unset(void) {
    clear_env("NVI_TEST_FB_UNSET");
    value_token_t v;
    token_t toks[] = {make_token("KEY", INTERPOLATED_KEY, "NVI_TEST_FB_UNSET:-fell back", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("fell back", lookup(&parser.env_map, "KEY"));
}

static void test_fallback_used_when_map_value_empty(void) {
    value_token_t v1;
    value_token_t v2;
    token_t toks[] = {
        make_token("EMPTYSRC", LITERAL_VALUE, "", &v1),
        make_token("KEY", INTERPOLATED_KEY, "EMPTYSRC:-fb", &v2),
    };
    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};

    args_t args = {0};
    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("fb", lookup(&parser.env_map, "KEY"));
}

static void test_value_wins_over_fallback(void) {
    set_env("NVI_TEST_FB_SET", "real");
    value_token_t v;
    token_t toks[] = {make_token("KEY", INTERPOLATED_KEY, "NVI_TEST_FB_SET:-fb", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("real", lookup(&parser.env_map, "KEY"));
    clear_env("NVI_TEST_FB_SET");
}

static void test_defined_empty_without_fallback_resolves_empty(void) {
    value_token_t v1;
    value_token_t v2;
    token_t toks[] = {
        make_token("EMPTYSRC", LITERAL_VALUE, "", &v1),
        make_token("KEY", INTERPOLATED_KEY, "EMPTYSRC", &v2),
    };
    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};

    args_t args = {0};
    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("", lookup(&parser.env_map, "KEY"));
}

// interpolation expansion bomb: SEED holds ~600KB, BOMB references it twice
// (~1.2MB), which must trip MAX_ENV_VALUE_SIZE instead of compounding
static void test_errors_when_interpolation_exceeds_max_value_size(void) {
    const size_t seed_len = (MAX_ENV_VALUE_SIZE / 2) + (64 * 1024);
    char *seed = arena_alloc(&test_arena, seed_len + 1);
    memset(seed, 'x', seed_len);
    seed[seed_len] = '\0';

    value_token_t seed_v;
    value_token_t bomb_v[2];
    token_t toks[] = {make_token("SEED", LITERAL_VALUE, seed, &seed_v), {0}};

    bomb_v[0] = (value_token_t){.value = "SEED", .value_len = 4, .kind = INTERPOLATED_KEY, .line = 2, .byte = 1};
    bomb_v[1] = bomb_v[0];
    toks[1] = (token_t){.key = "BOMB", .file = "testp.env"};
    toks[1].values.items = bomb_v;
    toks[1].values.count = 2;
    toks[1].values.capacity = 2;

    token_list_t tl = {.items = toks, .count = 2, .capacity = 2};

    args_t args = {0};
    parser_t parser = {0};
    result_t r = parser_silent(&args, &tl, &parser);
    TEST_ASSERT_FALSE(r.ok);
    TEST_ASSERT_EQUAL_INT(1, r.code);
}

static void test_value_at_max_value_size_passes(void) {
    char *max = arena_alloc(&test_arena, MAX_ENV_VALUE_SIZE + 1);
    memset(max, 'x', MAX_ENV_VALUE_SIZE);
    max[MAX_ENV_VALUE_SIZE] = '\0';

    value_token_t v;
    token_t toks[] = {make_token("KEY", LITERAL_VALUE, max, &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    parser_t parser = {0};
    result_t r = run_parser(&test_arena, &args, &tl, &parser);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_size_t(MAX_ENV_VALUE_SIZE, strlen(lookup(&parser.env_map, "KEY")));
}

static void test_errors_on_undefined_interpolation(void) {
    clear_env("NVI_TEST_UNDEF");
    value_token_t v;
    token_t toks[] = {make_token("KEY", INTERPOLATED_KEY, "NVI_TEST_UNDEF", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    parser_t parser = {0};
    result_t r = parser_silent(&args, &tl, &parser);
    TEST_ASSERT_FALSE(r.ok);
    TEST_ASSERT_EQUAL_INT(1, r.code);
}

static void test_errors_when_required_env_is_empty(void) {
    value_token_t v;
    token_t toks[] = {make_token("KEY", LITERAL_VALUE, "", &v)};
    token_list_t tl = {.items = toks, .count = 1, .capacity = 1};

    args_t args = {0};
    const char *req[] = {"KEY"};
    args.required.items = (const char **)req;
    args.required.count = 1;
    const char *cmd[] = {"echo"};
    args.command.items = cmd;
    args.command.count = 1;

    parser_t parser = {0};
    result_t r = parser_silent(&args, &tl, &parser);
    TEST_ASSERT_FALSE(r.ok);
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
    RUN_TEST(test_errors_when_interpolation_exceeds_max_value_size);
    RUN_TEST(test_value_at_max_value_size_passes);
    RUN_TEST(test_errors_when_required_env_is_empty);
    return UNITY_END();
}
