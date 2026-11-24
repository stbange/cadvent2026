/* SPDX-License-Identifier: MIT */
/* RPG kit persuasion and hacking. */
#include "core/mathx.h"
#include "kits/rpg/persuade.h"
#include "unity.h"

static float cadv_absf_hack(float a, float b) { return cadv_absf(cadv_wrap_angle(a - b)); }

void setUp(void) {}
void tearDown(void) {}

static int ticks_to_convert(const cadv_charm *c, float dist, int followers, int resist)
{
    float p = 0.0f;
    int i;
    for (i = 0; i < 60 * 20; ++i) {
        if (cadv_charm_tick(c, &p, dist, followers, resist, 1.0f / 60.0f)) {
            return i + 1;
        }
    }
    return -1;
}

static void test_rpg_persuade__persuasion_range_and_limit(void)
{
    cadv_charm c = {8.0f, 4, 0.5f, 1.0f};
    float p = 0.5f;
    int near;
    int far;
    TEST_ASSERT_EQUAL_INT(4, cadv_charm_limit(&c, 0));
    TEST_ASSERT_EQUAL_INT(14, cadv_charm_limit(&c, 20)); /* the crowd lets more follow */
    near = ticks_to_convert(&c, 1.0f, 0, 0);
    far = ticks_to_convert(&c, 7.5f, 0, 0);
    TEST_ASSERT_TRUE(near > 0 && far > near); /* closer is quicker */
    TEST_ASSERT_EQUAL_INT(-1, ticks_to_convert(&c, 9.0f, 0, 0)); /* out of range */
    /* guards need a crowd first, and take longer */
    TEST_ASSERT_EQUAL_INT(-1, ticks_to_convert(&c, 2.0f, 3, 1));
    TEST_ASSERT_TRUE(ticks_to_convert(&c, 2.0f, 4, 1) > ticks_to_convert(&c, 2.0f, 4, 0));
    TEST_ASSERT_EQUAL_INT(0, cadv_charm_needs(0));
    TEST_ASSERT_EQUAL_INT(8, cadv_charm_needs(2));
    /* progress fades out of range */
    TEST_ASSERT_FALSE(cadv_charm_tick(&c, &p, 20.0f, 0, 0, 0.25f));
    TEST_ASSERT_EQUAL_FLOAT(0.25f, p);
    /* a full following refuses more: limit with zero growth */
    {
        cadv_charm tight = {8.0f, 2, 0.0f, 1.0f};
        TEST_ASSERT_EQUAL_INT(-1, ticks_to_convert(&tight, 1.0f, 2, 0));
        TEST_ASSERT_TRUE(ticks_to_convert(&tight, 1.0f, 1, 0) > 0);
    }
}

static void test_rpg_persuade__hacking_timing_ring(void)
{
    cadv_hack h;
    int i;
    int hits = 0;
    cadv_hack_start(&h, 3, 3.0f, 0.3f, 42u);
    TEST_ASSERT_FALSE(h.done);
    /* pressing far from the window is a miss */
    h.angle = h.window + 1.5f;
    TEST_ASSERT_EQUAL_INT(-1, cadv_hack_press(&h));
    TEST_ASSERT_EQUAL_INT(1, h.misses);
    /* wait for the marker to sweep into the window, then press */
    for (i = 0; i < 60 * 30 && !h.done; ++i) {
        cadv_hack_tick(&h, 1.0f / 60.0f);
        if (cadv_absf_hack(h.angle, h.window) < 0.05f) {
            hits += cadv_hack_press(&h) == 1 ? 1 : 0;
        }
    }
    TEST_ASSERT_TRUE(h.done);
    TEST_ASSERT_EQUAL_INT(3, hits);
    TEST_ASSERT_EQUAL_INT(0, cadv_hack_press(&h));
    cadv_hack_tick(&h, 10.0f); /* finished: frozen */
    TEST_ASSERT_TRUE(h.angle >= 0.0f && h.angle < 6.3f);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rpg_persuade__persuasion_range_and_limit);
    RUN_TEST(test_rpg_persuade__hacking_timing_ring);
    return UNITY_END();
}
