/* SPDX-License-Identifier: MIT */
#include <stdint.h>

#include "core/arena.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

typedef struct big { double d[3]; } big;

static void test_alloc_is_aligned_and_zeroed(void)
{
    cadv_arena a;
    unsigned char *p;
    big *b;
    TEST_ASSERT_TRUE(cadv_arena_init(&a, 256));
    p = (unsigned char *)cadv_arena_alloc(&a, 3, 1);
    TEST_ASSERT_NOT_NULL(p);
    p[0] = 0xff;
    b = CADV_ARENA_NEW(&a, big);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_UINT(0, (uintptr_t)b % _Alignof(big));
    TEST_ASSERT_EQUAL_DOUBLE(0.0, b->d[2]);
    p = (unsigned char *)cadv_arena_alloc(&a, 8, 0); /* default alignment 16 */
    TEST_ASSERT_EQUAL_UINT(0, (uintptr_t)p % 16u);
    cadv_arena_free(&a);
    TEST_ASSERT_NULL(a.base);
}

static void test_exhaustion_returns_null(void)
{
    unsigned char buf[64];
    cadv_arena a;
    cadv_arena_init_buffer(&a, buf, sizeof buf);
    TEST_ASSERT_NOT_NULL(cadv_arena_alloc(&a, 60, 1));
    TEST_ASSERT_NULL(cadv_arena_alloc(&a, 8, 1));
    TEST_ASSERT_NULL(cadv_arena_alloc(&a, SIZE_MAX, 1));
    TEST_ASSERT_EQUAL_UINT(60, a.used);
    cadv_arena_free(&a); /* buffer not owned: must not be freed */
}

static void test_alignment_padding_past_capacity_fails(void)
{
    unsigned char buf[40];
    cadv_arena a;
    cadv_arena_init_buffer(&a, buf, 33);
    TEST_ASSERT_NOT_NULL(cadv_arena_alloc(&a, 1, 1));
    /* next 32-aligned offset may exceed capacity 33 */
    a.used = 33;
    TEST_ASSERT_NULL(cadv_arena_alloc(&a, 0, 32));
}

static void test_invalid_alignment_and_null_base(void)
{
    cadv_arena a;
    unsigned char buf[16];
    cadv_arena_init_buffer(&a, buf, sizeof buf);
    TEST_ASSERT_NULL(cadv_arena_alloc(&a, 4, 3));
    cadv_arena_init_buffer(&a, NULL, 0);
    TEST_ASSERT_NULL(cadv_arena_alloc(&a, 1, 1));
}

static void test_mark_reset_and_peak(void)
{
    cadv_arena a;
    size_t m;
    TEST_ASSERT_TRUE(cadv_arena_init(&a, 128));
    cadv_arena_alloc(&a, 16, 1);
    m = cadv_arena_mark(&a);
    cadv_arena_alloc(&a, 32, 1);
    TEST_ASSERT_EQUAL_UINT(48, a.used);
    cadv_arena_reset_to(&a, m);
    TEST_ASSERT_EQUAL_UINT(16, a.used);
    cadv_arena_reset_to(&a, 100); /* marks beyond 'used' are ignored */
    TEST_ASSERT_EQUAL_UINT(16, a.used);
    cadv_arena_reset(&a);
    TEST_ASSERT_EQUAL_UINT(0, a.used);
    TEST_ASSERT_EQUAL_UINT(48, a.peak);
    cadv_arena_free(&a);
}

static void test_zero_capacity_init(void)
{
    cadv_arena a;
    TEST_ASSERT_TRUE(cadv_arena_init(&a, 0));
    TEST_ASSERT_NULL(cadv_arena_alloc(&a, 1, 1));
    TEST_ASSERT_NOT_NULL(cadv_arena_alloc(&a, 0, 1));
    cadv_arena_free(&a);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_alloc_is_aligned_and_zeroed);
    RUN_TEST(test_exhaustion_returns_null);
    RUN_TEST(test_alignment_padding_past_capacity_fails);
    RUN_TEST(test_invalid_alignment_and_null_base);
    RUN_TEST(test_mark_reset_and_peak);
    RUN_TEST(test_zero_capacity_init);
    return UNITY_END();
}
