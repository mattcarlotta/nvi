#include "accessors.h"
#include "arena.h"
#include "file.h"
#include "matcher.h"
#include "unity.h"
#include <string.h>

static arena_t test_arena;

void setUp(void) { arena_init(&test_arena, 0); }
void tearDown(void) { arena_free(&test_arena); }

static file_ext_t ext_for(const char *ext) {
    const ext_entry *e = get_scan_extension(ext);
    TEST_ASSERT_NOT_NULL(e);
    return (file_ext_t){.ext = e->ext, .accessors = e->accessors, .accessor_count = e->count};
}

static file_details_t mock_file(const char *src) {
    file_details_t f = {.contents = (char *)src, .path = "test", .len = strlen(src)};
    return f;
}

static void expect_key(const env_key_match_t *m, const char *want) {
    TEST_ASSERT_EQUAL_size_t(strlen(want), m->key_len);
    TEST_ASSERT_EQUAL_STRING_LEN(want, m->key, m->key_len);
}

static void test_ident_key_with_location(void) {
    file_ext_t fe = ext_for("ts");
    file_details_t f = mock_file("const k = process.env.API_KEY;\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "API_KEY");
    TEST_ASSERT_EQUAL_size_t(1, matches.items[0].line);
    TEST_ASSERT_EQUAL_size_t(23, matches.items[0].byte);
}

static void test_bracket_quoted_key_with_location(void) {
    file_ext_t fe = ext_for("ts");
    file_details_t f = mock_file("const k = process.env[\"API_KEY\"];\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "API_KEY");
    TEST_ASSERT_EQUAL_size_t(1, matches.items[0].line);
    TEST_ASSERT_EQUAL_size_t(24, matches.items[0].byte);
}

static void test_tracks_lines_across_quoted_keys(void) {
    file_ext_t fe = ext_for("py");
    file_details_t f = mock_file("\n\nos.getenv(\"FOO\")\nos.getenv(\"BAR\")\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(2, matches.count);
    expect_key(&matches.items[0], "FOO");
    TEST_ASSERT_EQUAL_size_t(3, matches.items[0].line);
    TEST_ASSERT_EQUAL_size_t(12, matches.items[0].byte);
    expect_key(&matches.items[1], "BAR");
    TEST_ASSERT_EQUAL_size_t(4, matches.items[1].line);
    TEST_ASSERT_EQUAL_size_t(12, matches.items[1].byte);
}

static void test_skips_dynamic_keys(void) {
    file_ext_t fe = ext_for("py");
    file_details_t f = mock_file("os.getenv(name) os.getenv(\"REAL\")\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "REAL");
}

static void test_skips_mid_identifier_prefix(void) {
    file_ext_t fe = ext_for("ts");
    file_details_t f = mock_file("xprocess.env.FAKE process.env.REAL\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "REAL");
}

static void test_braced_strips_optional_quotes(void) {
    file_ext_t fe = ext_for("pl");
    file_details_t f = mock_file("$ENV{FOO} $ENV{'BAR'}\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(2, matches.count);
    expect_key(&matches.items[0], "FOO");
    expect_key(&matches.items[1], "BAR");
}

static void test_python_getenv_and_environ_forms(void) {
    file_ext_t fe = ext_for("py");
    file_details_t f = mock_file("x = os.getenv(\"A\")\ny = os.environ[\"B\"]\nz = os.environ.get(\"C\")\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(3, matches.count);
    expect_key(&matches.items[0], "A");
    expect_key(&matches.items[1], "B");
    expect_key(&matches.items[2], "C");
}

static void test_rust_var_and_macros(void) {
    file_ext_t fe = ext_for("rs");
    file_details_t f =
        mock_file("let a = env::var(\"A\").unwrap();\nlet b = env!(\"B\");\nlet c = option_env!(\"C\");\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(3, matches.count);
    expect_key(&matches.items[0], "A");
    expect_key(&matches.items[1], "B");
    expect_key(&matches.items[2], "C");
}

static void test_powershell_sigil_and_dotnet_accessor(void) {
    file_ext_t fe = ext_for("ps1");
    file_details_t f = mock_file("$a = $env:A\n$b = [Environment]::GetEnvironmentVariable(\"B\")\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(2, matches.count);
    expect_key(&matches.items[0], "A");
    expect_key(&matches.items[1], "B");
}

static void test_tcl_paren_and_nushell_dotted(void) {
    file_ext_t tcl = ext_for("tcl");
    file_details_t f1 = mock_file("set a $env(A)\n");
    env_key_matches_t m1 = {0};
    scan_file_content(&test_arena, &f1, &tcl, &m1);
    TEST_ASSERT_EQUAL_size_t(1, m1.count);
    expect_key(&m1.items[0], "A");

    file_ext_t nu = ext_for("nu");
    file_details_t f2 = mock_file("let a = $env.A\n");
    env_key_matches_t m2 = {0};
    scan_file_content(&test_arena, &f2, &nu, &m2);
    TEST_ASSERT_EQUAL_size_t(1, m2.count);
    expect_key(&m2.items[0], "A");
}

static void test_yaml_expansion_with_location(void) {
    file_ext_t fe = ext_for("yml");
    file_details_t f = mock_file("image: ${IMAGE_TAG}\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "IMAGE_TAG");
    TEST_ASSERT_EQUAL_size_t(1, matches.items[0].line);
    TEST_ASSERT_EQUAL_size_t(10, matches.items[0].byte);
}

static void test_yaml_expansion_operator_forms(void) {
    file_ext_t fe = ext_for("yaml");
    file_details_t f = mock_file("a: ${PORT:-8080}\nb: ${DB_URL:?required}\nc: ${HOST-localhost}\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(3, matches.count);
    expect_key(&matches.items[0], "PORT");
    expect_key(&matches.items[1], "DB_URL");
    expect_key(&matches.items[2], "HOST");
}

static void test_yaml_expansion_nested_default(void) {
    file_ext_t fe = ext_for("yml");
    file_details_t f = mock_file("url: ${PRIMARY:-${FALLBACK}}\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(2, matches.count);
    expect_key(&matches.items[0], "PRIMARY");
    expect_key(&matches.items[1], "FALLBACK");
}

static void test_yaml_expansion_skips_escaped_dollars(void) {
    file_ext_t fe = ext_for("yml");
    file_details_t f = mock_file("a: $${LITERAL} b: ${REAL} c: $$${ESCAPED_THEN_REAL}\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "REAL");
}

static void test_yaml_expansion_skips_actions_and_unterminated(void) {
    file_ext_t fe = ext_for("yml");
    file_details_t f = mock_file("run: ${{ env.SECRET }}\nbroken: ${NO_BRACE\nok: ${VALID}\n");

    env_key_matches_t matches = {0};
    scan_file_content(&test_arena, &f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "VALID");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ident_key_with_location);
    RUN_TEST(test_bracket_quoted_key_with_location);
    RUN_TEST(test_tracks_lines_across_quoted_keys);
    RUN_TEST(test_skips_dynamic_keys);
    RUN_TEST(test_skips_mid_identifier_prefix);
    RUN_TEST(test_braced_strips_optional_quotes);
    RUN_TEST(test_python_getenv_and_environ_forms);
    RUN_TEST(test_rust_var_and_macros);
    RUN_TEST(test_powershell_sigil_and_dotnet_accessor);
    RUN_TEST(test_tcl_paren_and_nushell_dotted);
    RUN_TEST(test_yaml_expansion_with_location);
    RUN_TEST(test_yaml_expansion_operator_forms);
    RUN_TEST(test_yaml_expansion_nested_default);
    RUN_TEST(test_yaml_expansion_skips_escaped_dollars);
    RUN_TEST(test_yaml_expansion_skips_actions_and_unterminated);
    return UNITY_END();
}
