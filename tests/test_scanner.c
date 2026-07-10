#include "arena.h"
#include "arg.h"
#include "dynarr.h"
#include "hashset.h"
#include "scanner.h"
#include "unity.h"
#include <string.h>

static arena_t test_arena;

void setUp(void) { arena_init(&test_arena, 0); }
void tearDown(void) { arena_free(&test_arena); }

static void test_merge_skips_ignored_keys(void) {
    args_t args = {0}; // dry_run = false, so the merge stays quiet
    DYN_ARR_APPEND(&test_arena, &args.ignored, "NODE_ENV");

    scanner_t scanner = {0};
    hashset_append(&test_arena, &scanner.env_keys, "NODE_ENV", strlen("NODE_ENV"));
    hashset_append(&test_arena, &scanner.env_keys, "API_KEY", strlen("API_KEY"));

    merge_required_envs(&test_arena, &args, &scanner);

    TEST_ASSERT_EQUAL_size_t(1, args.required.count);
    TEST_ASSERT_TRUE(set_contains(&args.required, "API_KEY"));
    TEST_ASSERT_FALSE(set_contains(&args.required, "NODE_ENV"));
}

static void test_merge_dedups_already_required(void) {
    args_t args = {0};
    DYN_ARR_APPEND(&test_arena, &args.required, "API_KEY");

    scanner_t scanner = {0};
    hashset_append(&test_arena, &scanner.env_keys, "API_KEY", strlen("API_KEY"));
    hashset_append(&test_arena, &scanner.env_keys, "DB_URL", strlen("DB_URL"));

    merge_required_envs(&test_arena, &args, &scanner);

    TEST_ASSERT_EQUAL_size_t(2, args.required.count);
    TEST_ASSERT_TRUE(set_contains(&args.required, "API_KEY"));
    TEST_ASSERT_TRUE(set_contains(&args.required, "DB_URL"));
}

static void test_merge_empty_envs_is_noop(void) {
    args_t args = {0};
    scanner_t scanner = {0};

    merge_required_envs(&test_arena, &args, &scanner);

    TEST_ASSERT_EQUAL_size_t(0, args.required.count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_merge_skips_ignored_keys);
    RUN_TEST(test_merge_dedups_already_required);
    RUN_TEST(test_merge_empty_envs_is_noop);
    return UNITY_END();
}
