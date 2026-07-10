#include "arena.h"
#include "hashset.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

static arena_t test_arena;

void setUp(void) { test_arena = (arena_t){0}; }
void tearDown(void) { arena_free(&test_arena); }

static void test_zero_value_set_is_empty(void) {
    hashset_t set = {0};
    TEST_ASSERT_FALSE(hashset_contains(&set, "MISSING", strlen("MISSING")));
    TEST_ASSERT_EQUAL_size_t(0, set.count);
}

static void test_append_and_contains(void) {
    hashset_t set = {0};
    TEST_ASSERT_TRUE(hashset_append(&test_arena, &set, "API_KEY", strlen("API_KEY")));
    TEST_ASSERT_TRUE(hashset_contains(&set, "API_KEY", strlen("API_KEY")));
    TEST_ASSERT_FALSE(hashset_contains(&set, "API_KE", strlen("API_KE")));
    TEST_ASSERT_FALSE(hashset_contains(&set, "API_KEYS", strlen("API_KEYS")));
    TEST_ASSERT_EQUAL_size_t(1, set.count);
}

static void test_duplicate_append_returns_false(void) {
    hashset_t set = {0};
    TEST_ASSERT_TRUE(hashset_append(&test_arena, &set, "API_KEY", strlen("API_KEY")));
    TEST_ASSERT_FALSE(hashset_append(&test_arena, &set, "API_KEY", strlen("API_KEY")));
    TEST_ASSERT_EQUAL_size_t(1, set.count);
}

// 100 keys forces multiple doublings past HASHSET_INIT_CAP (16 -> 32 -> 64 -> 128 -> 256
// under the 0.7 load factor), exercising the rehash loop several times over
static void test_growth_preserves_membership(void) {
    enum { N = 100 };
    static char keys[N][16];

    hashset_t set = {0};

    for (size_t i = 0; i < N; ++i) {
        snprintf(keys[i], sizeof(keys[i]), "KEY_%zu", i);
        TEST_ASSERT_TRUE(hashset_append(&test_arena, &set, keys[i], strlen(keys[i])));
    }

    TEST_ASSERT_EQUAL_size_t(N, set.count);
    TEST_ASSERT_TRUE(set.capacity > N); // grew past the initial capacity under the load factor

    // every key survives the rehashes, and duplicates are still rejected after growth
    for (size_t i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(hashset_contains(&set, keys[i], strlen(keys[i])));
        TEST_ASSERT_FALSE(hashset_append(&test_arena, &set, keys[i], strlen(keys[i])));
    }

    TEST_ASSERT_EQUAL_size_t(N, set.count);
    TEST_ASSERT_FALSE(hashset_contains(&set, "KEY_100", strlen("KEY_100")));
}

// same 64-bit hash cannot collide here realistically, but same home slot can; equal-length
// distinct keys with dense sequential suffixes are the cheapest way to force probe chains
static void test_probe_chains_across_growth(void) {
    enum { N = 40 };
    static char keys[N][8];

    hashset_t set = {0};

    for (size_t i = 0; i < N; ++i) {
        snprintf(keys[i], sizeof(keys[i]), "K%03zu", i);
        TEST_ASSERT_TRUE(hashset_append(&test_arena, &set, keys[i], strlen(keys[i])));
        // membership holds mid-growth, not just at the end
        TEST_ASSERT_TRUE(hashset_contains(&set, keys[i], strlen(keys[i])));
        TEST_ASSERT_TRUE(hashset_contains(&set, keys[0], strlen(keys[0])));
    }

    TEST_ASSERT_EQUAL_size_t(N, set.count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_zero_value_set_is_empty);
    RUN_TEST(test_append_and_contains);
    RUN_TEST(test_duplicate_append_returns_false);
    RUN_TEST(test_growth_preserves_membership);
    RUN_TEST(test_probe_chains_across_growth);
    return UNITY_END();
}
