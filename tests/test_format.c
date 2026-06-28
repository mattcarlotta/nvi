#include "format.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

// Mirrors the parsing half of formatter_test.zig. The actual pair rendering
// ($env:K = '...', K=value) now lives in the emitter, which writes straight to
// stdout, so that behavior is covered by the integration (golden) tests rather
// than here.

static void test_get_format_known_values(void) {
    TEST_ASSERT_EQUAL_INT(FORMAT_POWERSHELL, get_format("powershell"));
    TEST_ASSERT_EQUAL_INT(FORMAT_NULL, get_format("nul"));
}

static void test_get_format_unknown(void) {
    TEST_ASSERT_EQUAL_INT(FORMAT_UNKNOWN, get_format("json"));
    TEST_ASSERT_EQUAL_INT(FORMAT_UNKNOWN, get_format(""));
}

static void test_get_format_name_round_trips(void) {
    TEST_ASSERT_EQUAL_STRING("powershell", get_format_name(FORMAT_POWERSHELL));
    TEST_ASSERT_EQUAL_STRING("nul", get_format_name(FORMAT_NULL));
    TEST_ASSERT_EQUAL_STRING("unknown", get_format_name(FORMAT_UNKNOWN));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_format_known_values);
    RUN_TEST(test_get_format_unknown);
    RUN_TEST(test_get_format_name_round_trips);
    return UNITY_END();
}
