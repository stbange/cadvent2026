/* SPDX-License-Identifier: MIT */
#include "core/rng.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* Reference output of the PCG32 demo program, seed 42, stream 54. */
static void test_matches_pcg32_reference_vector(void)
{
    static const uint32_t expected[6] = {0xa15c02b7u, 0x7b47f409u, 0xba1d3330u,
                                         0x83d2f293u, 0xbfa4784bu, 0xcbed606eu};
    cadv_rng r;
    int i;
    cadv_rng_seed(&r, 42u, 54u);
    for (i = 0; i < 6; ++i) {
        TEST_ASSERT_EQUAL_HEX32(expected[i], cadv_rng_next(&r));
    }
}

static void test_same_seed_same_sequence(void)
{
    cadv_rng a;
    cadv_rng b;
    int i;
    cadv_rng_seed(&a, 2026u, 12u);
    cadv_rng_seed(&b, 2026u, 12u);
    for (i = 0; i < 1000; ++i) {
        TEST_ASSERT_EQUAL_UINT32(cadv_rng_next(&a), cadv_rng_next(&b));
    }
}

static void test_below_stays_in_bounds(void)
{
    cadv_rng r;
    int i;
    int seen[24] = {0};
    cadv_rng_seed(&r, 1u, 1u);
    TEST_ASSERT_EQUAL_UINT32(0, cadv_rng_below(&r, 0));
    for (i = 0; i < 10000; ++i) {
        uint32_t v = cadv_rng_below(&r, 24u);
        TEST_ASSERT_TRUE(v < 24u);
        seen[v]++;
    }
    for (i = 0; i < 24; ++i) {
        TEST_ASSERT_TRUE(seen[i] > 0);
    }
    /* A bound near 2^32 makes rejection sampling loop at least sometimes. */
    for (i = 0; i < 64; ++i) {
        TEST_ASSERT_TRUE(cadv_rng_below(&r, 0x80000001u) < 0x80000001u);
    }
}

static void test_range_inclusive_and_swapped(void)
{
    cadv_rng r;
    int i;
    int lo_hit = 0;
    int hi_hit = 0;
    cadv_rng_seed(&r, 7u, 3u);
    for (i = 0; i < 2000; ++i) {
        int32_t v = cadv_rng_range(&r, 5, -5);
        TEST_ASSERT_TRUE(v >= -5 && v <= 5);
        lo_hit |= (v == -5);
        hi_hit |= (v == 5);
    }
    TEST_ASSERT_TRUE(lo_hit && hi_hit);
    TEST_ASSERT_EQUAL_INT32(9, cadv_rng_range(&r, 9, 9));
    (void)cadv_rng_range(&r, INT32_MIN, INT32_MAX); /* full span path */
}

static void test_float01_in_unit_interval(void)
{
    cadv_rng r;
    int i;
    cadv_rng_seed(&r, 99u, 0u);
    for (i = 0; i < 10000; ++i) {
        float f = cadv_rng_float01(&r);
        TEST_ASSERT_TRUE(f >= 0.0f && f < 1.0f);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_matches_pcg32_reference_vector);
    RUN_TEST(test_same_seed_same_sequence);
    RUN_TEST(test_below_stays_in_bounds);
    RUN_TEST(test_range_inclusive_and_swapped);
    RUN_TEST(test_float01_in_unit_interval);
    return UNITY_END();
}
