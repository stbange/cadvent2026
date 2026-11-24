/* SPDX-License-Identifier: MIT */
/* RPG kit party: formation, orders, overclock, the radial ring. */
#include "kits/rpg/party.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_rpg_party__squad_formation(void)
{
    cadv_vec3 leader = cadv_v3(10, 0, 10);
    cadv_vec3 a;
    cadv_vec3 b;
    cadv_vec3 c;
    /* facing -z (yaw 0): everyone stands behind (+z), the pair to either side */
    a = cadv_formation_slot(leader, 0.0f, 0, 3, 2.0f);
    b = cadv_formation_slot(leader, 0.0f, 1, 3, 2.0f);
    c = cadv_formation_slot(leader, 0.0f, 2, 3, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, a.z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, b.z);
    TEST_ASSERT_TRUE(a.x < 10.0f && b.x > 10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 14.0f, c.z); /* the odd one out at the back, centred */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, c.x);
    /* turn round: the wedge turns with the leader */
    a = cadv_formation_slot(leader, CADV_PI, 0, 3, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 8.0f, a.z);
    TEST_ASSERT_TRUE(a.x > 10.0f);
    TEST_ASSERT_EQUAL_STRING("COVER", cadv_order_name(CADV_ORDER_COVER));
    TEST_ASSERT_EQUAL_STRING("?", cadv_order_name((cadv_order)9));
}

static void test_rpg_party__overclock_crash_timing(void)
{
    cadv_overclock o;
    int i;
    int crashed_at = -1;
    cadv_overclock_init(&o, 100.0f, 10.0f, 20.0f, 5.0f);
    TEST_ASSERT_TRUE(cadv_overclock_set(&o, CADV_OC_SPEED, true));
    TEST_ASSERT_TRUE(cadv_overclock_set(&o, CADV_OC_AIM, true));
    TEST_ASSERT_FALSE(cadv_overclock_set(&o, (cadv_slider)7, true));
    TEST_ASSERT_EQUAL_INT(2, cadv_overclock_active(&o));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cadv_overclock_level(&o, CADV_OC_AIM));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_overclock_level(&o, CADV_OC_AWARE));
    /* two sliders drain 20 a second: dry after five seconds */
    for (i = 0; i < 60 * 8 && crashed_at < 0; ++i) {
        if (cadv_overclock_tick(&o, 1.0f / 60.0f)) {
            crashed_at = i;
        }
    }
    TEST_ASSERT_INT_WITHIN(2, 300, crashed_at);
    TEST_ASSERT_TRUE(cadv_overclock_crashed(&o));
    TEST_ASSERT_EQUAL_INT(0, cadv_overclock_active(&o));
    TEST_ASSERT_EQUAL_FLOAT(-0.5f, cadv_overclock_level(&o, CADV_OC_SPEED)); /* the crash slows you */
    TEST_ASSERT_FALSE(cadv_overclock_set(&o, CADV_OC_SPEED, true));
    /* the crash lasts its length, recovering at half rate */
    for (i = 0; i < 60 * 5 - 2; ++i) {
        TEST_ASSERT_FALSE(cadv_overclock_tick(&o, 1.0f / 60.0f));
    }
    TEST_ASSERT_TRUE(cadv_overclock_crashed(&o));
    cadv_overclock_tick(&o, 1.0f / 60.0f);
    cadv_overclock_tick(&o, 1.0f / 60.0f);
    TEST_ASSERT_FALSE(cadv_overclock_crashed(&o));
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 50.0f, o.stamina);
    for (i = 0; i < 60 * 10; ++i) {
        cadv_overclock_tick(&o, 1.0f / 60.0f);
    }
    TEST_ASSERT_EQUAL_FLOAT(100.0f, o.stamina);
    TEST_ASSERT_TRUE(cadv_overclock_set(&o, CADV_OC_AWARE, true));
    TEST_ASSERT_TRUE(cadv_overclock_set(&o, CADV_OC_AWARE, false));
}

static void test_rpg_party__radial_ring(void)
{
    cadv_ring r = {false, 0, 2, 0.0f, 0.0f};
    TEST_ASSERT_EQUAL_INT(-1, cadv_ring_close(&r)); /* a closed ring chooses nothing */
    cadv_ring_open(&r, 4);
    cadv_ring_look(&r, 0.01f, 0.0f);
    TEST_ASSERT_EQUAL_INT(-1, r.sel); /* dead zone */
    cadv_ring_look(&r, 0.0f, 0.5f);
    TEST_ASSERT_EQUAL_INT(0, r.sel); /* up */
    cadv_ring_look(&r, 0.9f, -1.0f);
    TEST_ASSERT_EQUAL_INT(1, r.sel); /* right */
    cadv_ring_look(&r, -2.0f, -2.0f);
    TEST_ASSERT_EQUAL_INT(2, r.sel); /* down */
    cadv_ring_look(&r, -2.0f, 1.0f);
    TEST_ASSERT_EQUAL_INT(3, r.sel); /* left */
    TEST_ASSERT_EQUAL_INT(3, cadv_ring_close(&r));
    TEST_ASSERT_EQUAL_INT(-1, cadv_ring_close(&r));
    cadv_ring_look(&r, 1.0f, 0.0f); /* ignored while closed */
    TEST_ASSERT_FALSE(r.open);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rpg_party__squad_formation);
    RUN_TEST(test_rpg_party__overclock_crash_timing);
    RUN_TEST(test_rpg_party__radial_ring);
    return UNITY_END();
}
