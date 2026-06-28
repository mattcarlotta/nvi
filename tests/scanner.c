#include "scanner.h"
#include "arg.h"
#include "dynarr.h"
#include "set.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// merge_required_envs walks scanner->envs and appends each key to
// args->required, skipping any key already in args->ignored or already present
// in args->required. The seven scanContents.* cases from scanner_test.zig live
// in tests/matcher.c, since that logic became matcher's scan_file_content in the
// C port; this file covers the remaining scanner-level case.
//
// Every key below is a string literal (static storage). The set and lists
// borrow those pointers and free only their own backing arrays, so there is no
// heap key ownership to track here.

static void test_merge_skips_ignored_keys(void) {
    args_t args = {0}; // dry_run = false, so the merge stays quiet
    DYN_ARR_APPEND(&args.ignored, "NODE_ENV");

    scanner_t scanner = {0};
    set_add(&scanner.envs, "NODE_ENV", strlen("NODE_ENV"));
    set_add(&scanner.envs, "API_KEY", strlen("API_KEY"));

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(1, args.required.count);
    TEST_ASSERT_TRUE(list_contains(&args.required, "API_KEY"));
    TEST_ASSERT_FALSE(list_contains(&args.required, "NODE_ENV"));

    free_set(&scanner.envs);
    free_list(&args.required);
    free_list(&args.ignored);
}

// An env key already present in args->required is not appended a second time,
// while a new key is. Order is hash-dependent, so membership is asserted rather
// than position.
static void test_merge_dedups_already_required(void) {
    args_t args = {0};
    DYN_ARR_APPEND(&args.required, "API_KEY");

    scanner_t scanner = {0};
    set_add(&scanner.envs, "API_KEY", strlen("API_KEY"));
    set_add(&scanner.envs, "DB_URL", strlen("DB_URL"));

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(2, args.required.count);
    TEST_ASSERT_TRUE(list_contains(&args.required, "API_KEY"));
    TEST_ASSERT_TRUE(list_contains(&args.required, "DB_URL"));

    free_set(&scanner.envs);
    free_list(&args.required);
}

// An empty env set leaves required untouched (the early return path).
static void test_merge_empty_envs_is_noop(void) {
    args_t args = {0};
    scanner_t scanner = {0};

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(0, args.required.count);

    free_set(&scanner.envs);
    free_list(&args.required);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_merge_skips_ignored_keys);
    RUN_TEST(test_merge_dedups_already_required);
    RUN_TEST(test_merge_empty_envs_is_noop);
    return UNITY_END();
}
