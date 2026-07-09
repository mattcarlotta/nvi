#include "accessors.h"
#include "arena.h"
#include "unity.h"
#include <string.h>

static arena_t test_arena;

void setUp(void) { arena_init(&test_arena, 0); }
void tearDown(void) { arena_free(&test_arena); }

static void test_get_scan_extension_known(void) {
    const ext_entry *ts = get_scan_extension("ts");
    TEST_ASSERT_NOT_NULL(ts);
    TEST_ASSERT_EQUAL_STRING("ts", ts->ext);
    TEST_ASSERT_TRUE(ts->count > 0);
}

static void test_get_scan_extension_unknown_is_null(void) {
    TEST_ASSERT_NULL(get_scan_extension("xyz"));
    TEST_ASSERT_NULL(get_scan_extension(""));
    TEST_ASSERT_NULL(get_scan_extension("txt"));
}

static void test_js_family_shares_one_table(void) {
    const ext_entry *js = get_scan_extension("js");
    const ext_entry *ts = get_scan_extension("ts");
    const ext_entry *mjs = get_scan_extension("mjs");
    TEST_ASSERT_NOT_NULL(js);
    TEST_ASSERT_NOT_NULL(ts);
    TEST_ASSERT_NOT_NULL(mjs);
    TEST_ASSERT_EQUAL_PTR(js->accessors, ts->accessors);
    TEST_ASSERT_EQUAL_PTR(js->accessors, mjs->accessors);
}

static void test_kotlin_groovy_reuse_java_table(void) {
    const ext_entry *java = get_scan_extension("java");
    const ext_entry *kt = get_scan_extension("kt");
    const ext_entry *groovy = get_scan_extension("groovy");
    TEST_ASSERT_NOT_NULL(java);
    TEST_ASSERT_NOT_NULL(kt);
    TEST_ASSERT_NOT_NULL(groovy);
    TEST_ASSERT_EQUAL_PTR(java->accessors, kt->accessors);
    TEST_ASSERT_EQUAL_PTR(java->accessors, groovy->accessors);
}

// --- runtime map: append_file_extension + get_file_extension round trip ---

static void test_file_ext_map_round_trip(void) {
    file_ext_map_t map = {0};

    const ext_entry *ts = get_scan_extension("ts");
    TEST_ASSERT_NOT_NULL(ts);
    append_file_extension(&test_arena, &map, ts);

    const file_ext_t *file_ext = get_file_extension(&map, "ts");
    TEST_ASSERT_NOT_NULL(file_ext);
    TEST_ASSERT_EQUAL_STRING("ts", file_ext->ext);
    TEST_ASSERT_EQUAL_PTR(ts->accessors, file_ext->accessors);

    TEST_ASSERT_NULL(get_file_extension(&map, "py"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_scan_extension_known);
    RUN_TEST(test_get_scan_extension_unknown_is_null);
    RUN_TEST(test_js_family_shares_one_table);
    RUN_TEST(test_kotlin_groovy_reuse_java_table);
    RUN_TEST(test_file_ext_map_round_trip);
    return UNITY_END();
}
