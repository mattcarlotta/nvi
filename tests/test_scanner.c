#include "arg.h"
#include "dynarr.h"
#include "hashmap.h"
#include "scanner.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_merge_skips_ignored_keys(void) {
    args_t args = {0}; // dry_run = false, so the merge stays quiet
    DYN_ARR_APPEND(&args.ignored, "NODE_ENV");

    scanner_t scanner = {0};
    hashmap_append(&scanner.envs, "NODE_ENV", strlen("NODE_ENV"), 0);
    hashmap_append(&scanner.envs, "API_KEY", strlen("API_KEY"), 0);

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(1, args.required.count);
    TEST_ASSERT_TRUE(list_contains(&args.required, "API_KEY"));
    TEST_ASSERT_FALSE(list_contains(&args.required, "NODE_ENV"));

    free_hashmap(&scanner.envs);
    free_list(&args.required);
    free_list(&args.ignored);
}

static void test_merge_dedups_already_required(void) {
    args_t args = {0};
    DYN_ARR_APPEND(&args.required, "API_KEY");

    scanner_t scanner = {0};
    hashmap_append(&scanner.envs, "API_KEY", strlen("API_KEY"), 0);
    hashmap_append(&scanner.envs, "DB_URL", strlen("DB_URL"), 0);

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(2, args.required.count);
    TEST_ASSERT_TRUE(list_contains(&args.required, "API_KEY"));
    TEST_ASSERT_TRUE(list_contains(&args.required, "DB_URL"));

    free_hashmap(&scanner.envs);
    free_list(&args.required);
}

static void test_merge_empty_envs_is_noop(void) {
    args_t args = {0};
    scanner_t scanner = {0};

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(0, args.required.count);

    free_hashmap(&scanner.envs);
    free_list(&args.required);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_merge_skips_ignored_keys);
    RUN_TEST(test_merge_dedups_already_required);
    RUN_TEST(test_merge_empty_envs_is_noop);
    return UNITY_END();
}
