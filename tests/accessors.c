#include "accessors.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// --- find_ext: known and unknown extensions ---

static void test_find_ext_known(void) {
    const ext_entry *ts = find_ext("ts");
    TEST_ASSERT_NOT_NULL(ts);
    TEST_ASSERT_EQUAL_STRING("ts", ts->ext);
    TEST_ASSERT_TRUE(ts->count > 0);
}

static void test_find_ext_unknown_is_null(void) {
    TEST_ASSERT_NULL(find_ext("xyz"));
    TEST_ASSERT_NULL(find_ext(""));
    TEST_ASSERT_NULL(find_ext("txt"));
}

// --- shared tables: several extensions resolve to the same accessor array ---
// Mirrors the "shared tables behave identically" cases in accessors_test.zig.

static void test_js_family_shares_one_table(void) {
    const ext_entry *js = find_ext("js");
    const ext_entry *ts = find_ext("ts");
    const ext_entry *mjs = find_ext("mjs");
    TEST_ASSERT_NOT_NULL(js);
    TEST_ASSERT_NOT_NULL(ts);
    TEST_ASSERT_NOT_NULL(mjs);
    TEST_ASSERT_EQUAL_PTR(js->accessors, ts->accessors);
    TEST_ASSERT_EQUAL_PTR(js->accessors, mjs->accessors);
}

static void test_kotlin_groovy_reuse_java_table(void) {
    const ext_entry *java = find_ext("java");
    const ext_entry *kt = find_ext("kt");
    const ext_entry *groovy = find_ext("groovy");
    TEST_ASSERT_NOT_NULL(java);
    TEST_ASSERT_NOT_NULL(kt);
    TEST_ASSERT_NOT_NULL(groovy);
    TEST_ASSERT_EQUAL_PTR(java->accessors, kt->accessors);
    TEST_ASSERT_EQUAL_PTR(java->accessors, groovy->accessors);
}

// --- runtime map: file_ext_append + get_file_ext round trip ---

static void test_file_ext_map_round_trip(void) {
    file_ext_map_t map = {0};

    const ext_entry *ts = find_ext("ts");
    TEST_ASSERT_NOT_NULL(ts);
    file_ext_append(&map, ts);

    const file_ext_t *got = get_file_ext(&map, "ts");
    TEST_ASSERT_NOT_NULL(got);
    TEST_ASSERT_EQUAL_STRING("ts", got->ext);
    TEST_ASSERT_EQUAL_PTR(ts->accessors, got->accessors);

    TEST_ASSERT_NULL(get_file_ext(&map, "py"));

    free_file_ext_map(&map);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_find_ext_known);
    RUN_TEST(test_find_ext_unknown_is_null);
    RUN_TEST(test_js_family_shares_one_table);
    RUN_TEST(test_kotlin_groovy_reuse_java_table);
    RUN_TEST(test_file_ext_map_round_trip);
    return UNITY_END();
}
