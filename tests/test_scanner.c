#include "arena.h"
#include "arg.h"
#include "dynarr.h"
#include "hashmap.h"
#include "scanner.h"
#include "unity.h"
#include <string.h>

static arena_t test_arena;

void setUp(void) { arena_init(&test_arena, 0); }
void tearDown(void) { arena_free(&test_arena); }

static void test_merge_skips_ignored_keys(void) {
    args_t args = {.arena = &test_arena}; // dry_run = false, so the merge stays quiet
    DYN_ARR_APPEND(&test_arena, &args.ignored, "NODE_ENV");

    scanner_t scanner = {0};
    hashmap_append(&test_arena, &scanner.envs, "NODE_ENV", strlen("NODE_ENV"), 0);
    hashmap_append(&test_arena, &scanner.envs, "API_KEY", strlen("API_KEY"), 0);

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(1, args.required.count);
    TEST_ASSERT_TRUE(list_contains(&args.required, "API_KEY"));
    TEST_ASSERT_FALSE(list_contains(&args.required, "NODE_ENV"));
}

static void test_merge_dedups_already_required(void) {
    args_t args = {.arena = &test_arena};
    DYN_ARR_APPEND(&test_arena, &args.required, "API_KEY");

    scanner_t scanner = {0};
    hashmap_append(&test_arena, &scanner.envs, "API_KEY", strlen("API_KEY"), 0);
    hashmap_append(&test_arena, &scanner.envs, "DB_URL", strlen("DB_URL"), 0);

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(2, args.required.count);
    TEST_ASSERT_TRUE(list_contains(&args.required, "API_KEY"));
    TEST_ASSERT_TRUE(list_contains(&args.required, "DB_URL"));
}

static void test_merge_empty_envs_is_noop(void) {
    args_t args = {.arena = &test_arena};
    scanner_t scanner = {0};

    merge_required_envs(&args, &scanner);

    TEST_ASSERT_EQUAL_size_t(0, args.required.count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_merge_skips_ignored_keys);
    RUN_TEST(test_merge_dedups_already_required);
    RUN_TEST(test_merge_empty_envs_is_noop);
    return UNITY_END();
}
