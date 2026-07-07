#include "unity.h"
#include "unity_internals.h"
#include "utils.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// --- is_ident_start: letters and underscore only (NOT digits) ---

static void test_is_ident_start_accepts_letters_and_underscore(void) {
    TEST_ASSERT_TRUE(is_ident_start('a'));
    TEST_ASSERT_TRUE(is_ident_start('Z'));
    TEST_ASSERT_TRUE(is_ident_start('_'));
}

static void test_is_ident_start_rejects_digits_and_punctuation(void) {
    TEST_ASSERT_FALSE(is_ident_start('7'));
    TEST_ASSERT_FALSE(is_ident_start(' '));
    TEST_ASSERT_FALSE(is_ident_start('-'));
    TEST_ASSERT_FALSE(is_ident_start('.'));
    TEST_ASSERT_FALSE(is_ident_start('$'));
}

// --- is_ident_char: alphanumerics and underscore (mirrors Zig isIdentChar) ---

static void test_is_ident_char_accepts_alphanumerics_and_underscore(void) {
    TEST_ASSERT_TRUE(is_ident_char('a'));
    TEST_ASSERT_TRUE(is_ident_char('Z'));
    TEST_ASSERT_TRUE(is_ident_char('7'));
    TEST_ASSERT_TRUE(is_ident_char('_'));
}

static void test_is_ident_char_rejects_separators_and_punctuation(void) {
    TEST_ASSERT_FALSE(is_ident_char(' '));
    TEST_ASSERT_FALSE(is_ident_char('-'));
    TEST_ASSERT_FALSE(is_ident_char('.'));
    TEST_ASSERT_FALSE(is_ident_char('"'));
    TEST_ASSERT_FALSE(is_ident_char('$'));
}

// --- is_valid_key: ident_start followed by ident_chars ---

static void test_is_valid_key_accepts_well_formed_keys(void) {
    TEST_ASSERT_TRUE(is_valid_key("API_KEY", 7));
    TEST_ASSERT_TRUE(is_valid_key("_x", 2));
    TEST_ASSERT_TRUE(is_valid_key("a1b2", 4));
}

static void test_is_valid_key_rejects_empty_leading_digit_and_punctuation(void) {
    TEST_ASSERT_FALSE(is_valid_key("", 0));
    TEST_ASSERT_FALSE(is_valid_key("1ABC", 4));
    TEST_ASSERT_FALSE(is_valid_key("a-b", 3));
    TEST_ASSERT_FALSE(is_valid_key("a.b", 3));
}

// --- ends_with ---

static void test_ends_with(void) {
    TEST_ASSERT_TRUE(ends_with(".env.local", ".local"));
    TEST_ASSERT_TRUE(ends_with("a.dSYM", ".dSYM"));
    TEST_ASSERT_FALSE(ends_with("file", ".env"));
    TEST_ASSERT_TRUE(ends_with("anything", ""));
}

// --- is_blacklisted: exact dir names and known suffixes ---

static void test_is_blacklisted(void) {
    TEST_ASSERT_TRUE(is_blacklisted("node_modules"));
    TEST_ASSERT_TRUE(is_blacklisted("zig-out"));
    TEST_ASSERT_TRUE(is_blacklisted("Foo.dSYM"));
    TEST_ASSERT_FALSE(is_blacklisted("src"));
    TEST_ASSERT_FALSE(is_blacklisted("node_module"));
}

// --- index_of_scalar: first index of ch at/after pos, else len ---

static void test_index_of_scalar(void) {
    const char *s = "a.b.c";
    TEST_ASSERT_EQUAL_size_t(1, index_of_scalar(s, 5, 0, '.'));
    TEST_ASSERT_EQUAL_size_t(3, index_of_scalar(s, 5, 2, '.'));
    TEST_ASSERT_EQUAL_size_t(5, index_of_scalar(s, 5, 0, 'z'));
}

static void test_str_to_u8(void) {
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8(""));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8(" "));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8("00"));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8("01"));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8("abc"));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8("a1b2c8"));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8("256"));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8("-128"));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8("+128"));
    TEST_ASSERT_EQUAL_size_t(-1, str_to_u8("128x"));
    TEST_ASSERT_EQUAL_size_t(0, str_to_u8("0"));
    TEST_ASSERT_EQUAL_size_t(128, str_to_u8("128"));
    TEST_ASSERT_EQUAL_size_t(255, str_to_u8("255"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_is_ident_start_accepts_letters_and_underscore);
    RUN_TEST(test_is_ident_start_rejects_digits_and_punctuation);
    RUN_TEST(test_is_ident_char_accepts_alphanumerics_and_underscore);
    RUN_TEST(test_is_ident_char_rejects_separators_and_punctuation);
    RUN_TEST(test_is_valid_key_accepts_well_formed_keys);
    RUN_TEST(test_is_valid_key_rejects_empty_leading_digit_and_punctuation);
    RUN_TEST(test_ends_with);
    RUN_TEST(test_is_blacklisted);
    RUN_TEST(test_index_of_scalar);
    RUN_TEST(test_str_to_u8);
    return UNITY_END();
}
