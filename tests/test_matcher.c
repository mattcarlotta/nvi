#include "accessors.h"
#include "file.h"
#include "matcher.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// Build a file_ext_t from the real accessor table for an extension.
static file_ext_t ext_for(const char *ext) {
    const ext_entry *e = find_ext(ext);
    TEST_ASSERT_NOT_NULL(e);
    file_ext_t fe = {.ext = e->ext, .accessors = e->accessors, .accessor_count = e->count};
    return fe;
}

// Build a borrowed file_details_t over a literal (matcher never mutates it).
static file_details_t file_over(const char *src) {
    file_details_t f = {.contents = (char *)src, .path = "test", .len = strlen(src)};
    return f;
}

static void expect_key(const env_key_match_t *m, const char *want) {
    TEST_ASSERT_EQUAL_size_t(strlen(want), m->key_len);
    TEST_ASSERT_EQUAL_STRING_LEN(want, m->key, m->key_len);
}

// --- ident pattern with location (process.env.API_KEY at byte 23) ---

static void test_ident_key_with_location(void) {
    file_ext_t fe = ext_for("ts");
    file_details_t f = file_over("const k = process.env.API_KEY;\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "API_KEY");
    TEST_ASSERT_EQUAL_size_t(1, matches.items[0].line);
    TEST_ASSERT_EQUAL_size_t(23, matches.items[0].byte);

    free_env_key_matches(&matches);
}

// --- bracket-quoted pattern with location (byte points at the key, 24) ---

static void test_bracket_quoted_key_with_location(void) {
    file_ext_t fe = ext_for("ts");
    file_details_t f = file_over("const k = process.env[\"API_KEY\"];\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "API_KEY");
    TEST_ASSERT_EQUAL_size_t(1, matches.items[0].line);
    TEST_ASSERT_EQUAL_size_t(24, matches.items[0].byte);

    free_env_key_matches(&matches);
}

// --- line/byte tracking across multiple quoted keys (python) ---

static void test_tracks_lines_across_quoted_keys(void) {
    file_ext_t fe = ext_for("py");
    file_details_t f = file_over("\n\nos.getenv(\"FOO\")\nos.getenv(\"BAR\")\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(2, matches.count);
    expect_key(&matches.items[0], "FOO");
    TEST_ASSERT_EQUAL_size_t(3, matches.items[0].line);
    TEST_ASSERT_EQUAL_size_t(12, matches.items[0].byte);
    expect_key(&matches.items[1], "BAR");
    TEST_ASSERT_EQUAL_size_t(4, matches.items[1].line);
    TEST_ASSERT_EQUAL_size_t(12, matches.items[1].byte);

    free_env_key_matches(&matches);
}

// --- dynamic (non-literal) keys are skipped ---

static void test_skips_dynamic_keys(void) {
    file_ext_t fe = ext_for("py");
    file_details_t f = file_over("os.getenv(name) os.getenv(\"REAL\")\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "REAL");

    free_env_key_matches(&matches);
}

// --- prefixes that begin mid-identifier are skipped ---

static void test_skips_mid_identifier_prefix(void) {
    file_ext_t fe = ext_for("ts");
    file_details_t f = file_over("xprocess.env.FAKE process.env.REAL\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(1, matches.count);
    expect_key(&matches.items[0], "REAL");

    free_env_key_matches(&matches);
}

// --- braced pattern strips optional surrounding quotes (perl) ---

static void test_braced_strips_optional_quotes(void) {
    file_ext_t fe = ext_for("pl");
    file_details_t f = file_over("$ENV{FOO} $ENV{'BAR'}\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(2, matches.count);
    expect_key(&matches.items[0], "FOO");
    expect_key(&matches.items[1], "BAR");

    free_env_key_matches(&matches);
}

// --- a few language tables (from accessors_test.zig) ---

static void test_python_getenv_and_environ_forms(void) {
    file_ext_t fe = ext_for("py");
    file_details_t f = file_over("x = os.getenv(\"A\")\ny = os.environ[\"B\"]\nz = os.environ.get(\"C\")\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(3, matches.count);
    expect_key(&matches.items[0], "A");
    expect_key(&matches.items[1], "B");
    expect_key(&matches.items[2], "C");

    free_env_key_matches(&matches);
}

static void test_rust_var_and_macros(void) {
    file_ext_t fe = ext_for("rs");
    file_details_t f =
        file_over("let a = env::var(\"A\").unwrap();\nlet b = env!(\"B\");\nlet c = option_env!(\"C\");\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(3, matches.count);
    expect_key(&matches.items[0], "A");
    expect_key(&matches.items[1], "B");
    expect_key(&matches.items[2], "C");

    free_env_key_matches(&matches);
}

static void test_powershell_sigil_and_dotnet_accessor(void) {
    file_ext_t fe = ext_for("ps1");
    file_details_t f = file_over("$a = $env:A\n$b = [Environment]::GetEnvironmentVariable(\"B\")\n");

    env_key_matches_t matches = {0};
    scan_file_content(&f, &fe, &matches);

    TEST_ASSERT_EQUAL_size_t(2, matches.count);
    expect_key(&matches.items[0], "A");
    expect_key(&matches.items[1], "B");

    free_env_key_matches(&matches);
}

static void test_tcl_paren_and_nushell_dotted(void) {
    file_ext_t tcl = ext_for("tcl");
    file_details_t f1 = file_over("set a $env(A)\n");
    env_key_matches_t m1 = {0};
    scan_file_content(&f1, &tcl, &m1);
    TEST_ASSERT_EQUAL_size_t(1, m1.count);
    expect_key(&m1.items[0], "A");
    free_env_key_matches(&m1);

    file_ext_t nu = ext_for("nu");
    file_details_t f2 = file_over("let a = $env.A\n");
    env_key_matches_t m2 = {0};
    scan_file_content(&f2, &nu, &m2);
    TEST_ASSERT_EQUAL_size_t(1, m2.count);
    expect_key(&m2.items[0], "A");
    free_env_key_matches(&m2);
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
    return UNITY_END();
}
