/* SPDX-License-Identifier: MIT */
#include "core/clock.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_accumulates_fixed_ticks(void)
{
    cadv_clock c;
    cadv_clock_init(&c);
    TEST_ASSERT_EQUAL_INT(0, cadv_clock_advance(&c, 0.010, 1.0));
    TEST_ASSERT_EQUAL_INT(1, cadv_clock_advance(&c, 0.010, 1.0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.2f, cadv_clock_alpha(&c));
    TEST_ASSERT_EQUAL_INT(2, cadv_clock_advance(&c, 1.0 / 30.0, 1.0));
    TEST_ASSERT_EQUAL_UINT64(3, c.tick);
}

static void test_negative_and_huge_steps(void)
{
    cadv_clock c;
    cadv_clock_init(&c);
    TEST_ASSERT_EQUAL_INT(0, cadv_clock_advance(&c, -1.0, 1.0));
    /* a 5 s hitch is clamped to 0.25 s = 15 ticks, capped at 8, backlog dropped */
    TEST_ASSERT_EQUAL_INT(CADV_MAX_TICKS_PER_FRAME, cadv_clock_advance(&c, 5.0, 1.0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_clock_alpha(&c));
}

static void test_speed_scales_time(void)
{
    cadv_clock c;
    cadv_clock_init(&c);
    TEST_ASSERT_EQUAL_INT(4, cadv_clock_advance(&c, 1.0 / 60.0 + 1e-9, 4.0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_accumulates_fixed_ticks);
    RUN_TEST(test_negative_and_huge_steps);
    RUN_TEST(test_speed_scales_time);
    return UNITY_END();
}
