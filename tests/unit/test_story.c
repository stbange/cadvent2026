/* SPDX-License-Identifier: MIT */
/* FPS kit story pieces: triggers, followers, squad AI, chargers. */
#include <string.h>

#include "kits/fps/story.h"
#include "unity.h"

static cadv_nav nav;
static cadv_world world;

void setUp(void)
{
    int x;
    int z;
    cadv_nav_init(&nav, 12, 8, 2.0f);
    cadv_world_clear(&world);
    cadv_world_add(&world, cadv_v3(0, -1, 0), cadv_v3(24, 0, 16), 0);
    for (z = 0; z < 8; ++z) {
        for (x = 0; x < 12; ++x) {
            /* a wall at x = 6 with a gap at the bottom row */
            bool wall = x == 6 && z < 7;
            cadv_nav_set(&nav, x, z, !wall);
            if (wall) {
                cadv_world_add(&world, cadv_v3(12, 0, (float)z * 2.0f), cadv_v3(14, 3, (float)(z + 1) * 2.0f), 1);
            }
        }
    }
}
void tearDown(void) {}

static void test_fps_story__sequence_triggers_reentrancy(void)
{
    cadv_trigger once;
    cadv_trigger again;
    cadv_trigger_make(&once, cadv_v3(0, 0, 0), cadv_v3(2, 2, 2), true);
    cadv_trigger_make(&again, cadv_v3(0, 0, 0), cadv_v3(2, 2, 2), false);
    TEST_ASSERT_FALSE(cadv_trigger_update(&once, cadv_v3(5, 1, 1)));
    TEST_ASSERT_TRUE(cadv_trigger_update(&once, cadv_v3(1, 1, 1)));
    TEST_ASSERT_FALSE(cadv_trigger_update(&once, cadv_v3(1, 1, 1)));  /* still inside */
    TEST_ASSERT_FALSE(cadv_trigger_update(&once, cadv_v3(5, 1, 1)));
    TEST_ASSERT_FALSE(cadv_trigger_update(&once, cadv_v3(1, 1, 1)));  /* once only */
    TEST_ASSERT_EQUAL_INT(1, once.fired);
    TEST_ASSERT_TRUE(cadv_trigger_update(&again, cadv_v3(1, 1, 1)));
    TEST_ASSERT_FALSE(cadv_trigger_update(&again, cadv_v3(5, 1, 1)));
    TEST_ASSERT_TRUE(cadv_trigger_update(&again, cadv_v3(1, 1, 1)));  /* every entry */
    TEST_ASSERT_EQUAL_INT(2, again.fired);
}

static void test_fps_story__follower_pathing_to_scanner(void)
{
    cadv_follower f;
    cadv_vec3 leader = cadv_v3(21, 0, 3);
    int i;
    float d = 0.0f;
    cadv_follower_init(&f, cadv_v3(3, 0, 3), 4.0f, 2.0f);
    TEST_ASSERT_TRUE(cadv_follower_tick(&f, &nav, &world, leader, 1.0f / 60.0f) > 10.0f); /* waiting */
    TEST_ASSERT_EQUAL_FLOAT(3.0f, f.pos.x);
    cadv_follower_toggle(&f);
    for (i = 0; i < 60 * 20; ++i) {
        d = cadv_follower_tick(&f, &nav, &world, leader, 1.0f / 60.0f);
    }
    TEST_ASSERT_TRUE(d <= 2.1f); /* around the wall through the gap */
    TEST_ASSERT_TRUE(f.pos.x > 14.0f);
    /* sent to the scanner, it walks there and says so */
    cadv_follower_goto(&f, cadv_v3(21, 0, 13));
    for (i = 0; i < 60 * 10 && !f.arrived; ++i) {
        cadv_follower_tick(&f, &nav, &world, leader, 1.0f / 60.0f);
    }
    TEST_ASSERT_TRUE(f.arrived);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 13.0f, f.pos.z);
    cadv_follower_toggle(&f); /* goto -> follow */
    TEST_ASSERT_EQUAL_INT(CADV_FOLLOW_ON, f.mode);
    cadv_follower_toggle(&f);
    TEST_ASSERT_EQUAL_INT(CADV_FOLLOW_WAIT, f.mode);
}

static void test_fps_story__squad_ai_state_transitions(void)
{
    cadv_squad_view v;
    memset(&v, 0, sizeof v);
    v.hp = 1.0f;
    v.target_hidden = 30.0f;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_PATROL, cadv_squad_decide(CADV_SQUAD_PATROL, &v));
    v.sees_target = true;
    v.target_dist = 20.0f;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_ENGAGE, cadv_squad_decide(CADV_SQUAD_PATROL, &v));
    v.allies_engaged = 1;
    v.flank_open = true;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_FLANK, cadv_squad_decide(CADV_SQUAD_ENGAGE, &v));
    v.flank_open = false;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_ENGAGE, cadv_squad_decide(CADV_SQUAD_ENGAGE, &v));
    v.allies_engaged = 2;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_SUPPRESS, cadv_squad_decide(CADV_SQUAD_ENGAGE, &v));
    v.hp = 0.2f;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_RETREAT, cadv_squad_decide(CADV_SQUAD_SUPPRESS, &v));
    /* the target ducks into cover: flank, then flush it out with a grenade */
    v.hp = 1.0f;
    v.sees_target = false;
    v.target_hidden = 1.0f;
    v.has_grenade = true;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_FLANK, cadv_squad_decide(CADV_SQUAD_ENGAGE, &v));
    v.target_hidden = 3.0f;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_GRENADE, cadv_squad_decide(CADV_SQUAD_FLANK, &v));
    v.has_grenade = false;
    TEST_ASSERT_EQUAL_INT(CADV_SQUAD_FLANK, cadv_squad_decide(CADV_SQUAD_GRENADE, &v));
    TEST_ASSERT_EQUAL_STRING("SUPPRESS", cadv_squad_state_name(CADV_SQUAD_SUPPRESS));
    TEST_ASSERT_EQUAL_STRING("?", cadv_squad_state_name((cadv_squad_state)42));
}

static void test_fps_story__suit_charger_limits(void)
{
    cadv_charger c = {30.0f, 20.0f};
    float suit = 50.0f;
    float got = cadv_charger_use(&c, &suit, 100.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, got); /* the rate limits it */
    got = cadv_charger_use(&c, &suit, 100.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, got); /* then the reserve runs dry */
    TEST_ASSERT_EQUAL_FLOAT(80.0f, suit);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_charger_use(&c, &suit, 100.0f, 1.0f));
    c.reserve = 50.0f;
    suit = 95.0f;
    TEST_ASSERT_EQUAL_FLOAT(5.0f, cadv_charger_use(&c, &suit, 100.0f, 1.0f)); /* never above full */
    TEST_ASSERT_EQUAL_FLOAT(45.0f, c.reserve);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fps_story__sequence_triggers_reentrancy);
    RUN_TEST(test_fps_story__follower_pathing_to_scanner);
    RUN_TEST(test_fps_story__squad_ai_state_transitions);
    RUN_TEST(test_fps_story__suit_charger_limits);
    return UNITY_END();
}
