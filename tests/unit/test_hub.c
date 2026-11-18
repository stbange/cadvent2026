/* SPDX-License-Identifier: MIT */
#include <string.h>

#include "hub/hub.h"
#include "hub/layout.h"
#include "portal/portal.h"
#include "unity.h"

static cadv_hub h;
static cadv_progress pr;
static cadv_friends fr;
static cadv_input_frame in;

static void start(int y, int m, int d, bool dev)
{
    cadv_progress_reset(&pr);
    cadv_friends_default(&fr);
    cadv_hub_init(&h, &pr, &fr, cadv_datetime_make(y, m, d, 12, 0), 2026, dev);
}

void setUp(void)
{
    memset(&in, 0, sizeof in);
    start(2026, 12, 5, false);
}
void tearDown(void) {}

static uint32_t run(int ticks)
{
    uint32_t ev = 0;
    int i;
    for (i = 0; i < ticks; ++i) {
        ev |= cadv_hub_tick(&h, &in);
        in.pressed = 0;
    }
    return ev;
}

static void test_layout_slots(void)
{
    cadv_portal_slot s1 = cadv_hall_portal(1);
    cadv_portal_slot s7 = cadv_hall_portal(7);
    cadv_portal_slot s13 = cadv_hall_portal(13);
    cadv_portal_slot s19 = cadv_hall_portal(19);
    cadv_portal_slot s24 = cadv_hall_portal(24);
    cadv_portal_slot s0 = cadv_hall_portal(0);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, s1.normal.x);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, s7.normal.x);
    TEST_ASSERT_EQUAL_FLOAT(CADV_BALCONY_Y, s13.base.y);
    TEST_ASSERT_EQUAL_FLOAT(CADV_BALCONY_Y, s19.base.y);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, s24.normal.z);
    TEST_ASSERT_EQUAL_FLOAT(s1.base.z, s0.base.z);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -14.5f, cadv_hall_portal_front(1, 1.5f).x);
    TEST_ASSERT_TRUE(cadv_hall_socket(1).x < 0.0f && cadv_hall_socket(23).x > 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(-16.0f, cadv_hall_tree().z);
    TEST_ASSERT_TRUE(cadv_hall_legend_plaque(5).x > cadv_hall_legend_plaque(0).x);
}

static void test_layout_paths(void)
{
    cadv_vec3 p[8];
    TEST_ASSERT_EQUAL_INT(4, cadv_hall_path(24, p, 8));
    TEST_ASSERT_EQUAL_INT(3, cadv_hall_path(3, p, 8));
    TEST_ASSERT_EQUAL_INT(4, cadv_hall_path(15, p, 8));
    TEST_ASSERT_EQUAL_FLOAT(CADV_BALCONY_Y, p[2].y);
    TEST_ASSERT_EQUAL_INT(2, cadv_hall_path(20, p, 2));
}

static void test_states_follow_calendar_and_progress(void)
{
    TEST_ASSERT_EQUAL_INT(CADV_PS_THAWING, h.state[0]);       /* Day 1 has shipped */
    TEST_ASSERT_EQUAL_INT(CADV_PS_STILL_THAWING, h.state[1]); /* Day 2 has not */
    TEST_ASSERT_EQUAL_INT(CADV_PS_SEALED, h.state[5]);
    cadv_hub_set_time(&h, cadv_datetime_make(2026, 12, 24, 0, 0));
    TEST_ASSERT_EQUAL_INT(CADV_PS_STARS_MISSING, h.state[23]);
    start(2026, 12, 5, true); /* dev: everything open, arch filled */
    TEST_ASSERT_EQUAL_INT(CADV_PS_THAWING, h.state[0]);
    pr.p[0].entered = true;
    pr.p[1].entered = true;
    pr.p[1].snowflakes = 3;
    cadv_hub_update_states(&h);
    TEST_ASSERT_EQUAL_INT(CADV_PS_OPEN, h.state[0]);
    TEST_ASSERT_EQUAL_INT(CADV_PS_MASTERED, h.state[1]);
    TEST_ASSERT_EQUAL_STRING("mastered", cadv_portal_state_name(CADV_PS_MASTERED));
}

static void test_hub_hall__blocked_sealed_portal(void)
{
    uint32_t ev;
    cadv_portal_slot s = cadv_hall_portal(6);
    cadv_hub_teleport(&h, s.base.x + 0.4f, 0, s.base.z, 0.5f * CADV_PI);
    ev = run(2);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_DENIED);
    TEST_ASSERT_FALSE(ev & CADV_HUB_EV_ENTER);
    TEST_ASSERT_NOT_NULL(strstr(h.message, "SEALED"));
    TEST_ASSERT_NOT_NULL(strstr(h.message, "opens in"));
}

static void test_hub_hall__enter_and_return(void)
{
    uint32_t ev;
    cadv_portal_slot s;
    start(2026, 12, 5, true);
    s = cadv_hall_portal(3);
    cadv_hub_teleport(&h, s.base.x + 0.4f, 0, s.base.z, 0.5f * CADV_PI);
    ev = run(2);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_ENTER);
    TEST_ASSERT_EQUAL_INT(3, h.enter_day);
    cadv_hub_return_to(&h, 3);
    TEST_ASSERT_TRUE(h.player.pos.x > s.base.x + 1.5f);
    TEST_ASSERT_FALSE(run(10) & CADV_HUB_EV_ENTER); /* standing in front, not inside */
}

static void test_hub_hall__first_visit_of_a_newly_unlocked_portal(void)
{
    uint32_t ev;
    start(2026, 12, 5, true);
    cadv_hub_teleport(&h, -9.0f, 0, -4.0f, 0.5f * CADV_PI); /* look at portal 1 */
    h.player.pitch = 0.0f;
    ev = run(2);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_REVEAL);
    TEST_ASSERT_TRUE(h.reveal_timer[0] > 0);
    ev = run(100);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_SAVE);
    TEST_ASSERT_TRUE(pr.p[0].entered);
    TEST_ASSERT_EQUAL_INT(CADV_PS_OPEN, h.state[0]);
    TEST_ASSERT_FALSE(run(30) & CADV_HUB_EV_REVEAL); /* only once */
}

static void test_shining_stars__setting_stars_and_arch_messages(void)
{
    cadv_vec3 p = cadv_hall_arch_use_point();
    uint32_t ev;
    pr.p[2].star_collected = true;
    pr.p[4].star_collected = true;
    cadv_hub_teleport(&h, p.x, 0, p.z, 0);
    run(1);
    TEST_ASSERT_TRUE(h.focus_arch);
    TEST_ASSERT_NOT_NULL(strstr(h.prompt, "STAR ARCH 0/23"));
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    ev = run(1);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_SOCKETED);
    TEST_ASSERT_EQUAL_INT(2, cadv_progress_stars_socketed(&pr));
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    run(1);
    TEST_ASSERT_NOT_NULL(strstr(h.message, "MISSING: 1 2 4"));
    cadv_progress_dev_fill(&pr);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    run(1);
    TEST_ASSERT_EQUAL_STRING("THE STAR ARCH IS COMPLETE", h.message);
}

static void test_shining_stars__missing_stars_message_at_portal_24(void)
{
    cadv_portal_slot s;
    cadv_hub_set_time(&h, cadv_datetime_make(2026, 12, 24, 9, 0));
    s = cadv_hall_portal(24);
    cadv_hub_teleport(&h, s.base.x, 0, s.base.z + 0.4f, 0);
    TEST_ASSERT_TRUE(run(2) & CADV_HUB_EV_DENIED);
    TEST_ASSERT_EQUAL_STRING("THE ARCH NEEDS 23 MORE STARS", h.message);
}

static void test_beams(void)
{
    int days[24];
    start(2026, 11, 25, false);
    TEST_ASSERT_EQUAL_INT(0, cadv_hub_beams(&h, days, 24)); /* nothing open yet */
    start(2026, 12, 7, true);
    TEST_ASSERT_EQUAL_INT(1, cadv_hub_beams(&h, days, 24));
    TEST_ASSERT_EQUAL_INT(24, days[0]);
    TEST_ASSERT_EQUAL_INT(0, cadv_hub_beams(&h, days, 0));
    start(2026, 12, 24, false);
    TEST_ASSERT_EQUAL_INT(23, cadv_hub_beams(&h, days, 24));
}

static void test_tree_crown_epilogue_replay(void)
{
    uint32_t ev;
    cadv_hub_teleport(&h, 2.6f, 0, -13.2f, 0);
    run(1);
    TEST_ASSERT_TRUE(h.focus_tree);
    TEST_ASSERT_EQUAL_STRING("THE GREAT FIR", h.prompt);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    run(1);
    TEST_ASSERT_EQUAL_STRING("THE TREE WAITS FOR THE 24TH STAR", h.message);
    pr.p[23].star_collected = true;
    run(1);
    TEST_ASSERT_EQUAL_STRING("USE TO CROWN THE TREE", h.prompt);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    ev = run(1);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_CROWNED);
    TEST_ASSERT_TRUE(h.epilogue_timer > 0);
    ev = run(CADV_HUB_EPILOGUE_TICKS);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_EPILOGUE_END);
    TEST_ASSERT_TRUE(pr.epilogue_seen);
    run(1);
    TEST_ASSERT_EQUAL_STRING("USE TO REPLAY THE EPILOGUE", h.prompt);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    run(1);
    TEST_ASSERT_TRUE(h.epilogue_timer > 0);
    run(200);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE); /* skip */
    TEST_ASSERT_TRUE(run(1) & CADV_HUB_EV_EPILOGUE_END);
}

static void test_hub_hall__mark_found_and_legend_prompt(void)
{
    cadv_vec3 p = cadv_hall_legend_plaque(1);
    double v;
    pr.p[0].marks_found = 2u; /* day 1 mark 1 belongs to friend 1 */
    cadv_hub_teleport(&h, p.x, 0, p.z - 2.0f, CADV_PI); /* face south */
    h.player.pitch = 0.2f;
    run(1);
    TEST_ASSERT_EQUAL_INT(1, h.focus_legend);
    TEST_ASSERT_NOT_NULL(strstr(h.prompt, "Bloodmaker - 1/"));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "legend.1", &v));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, v);
}

static void test_portal_prompts_and_use_messages(void)
{
    start(2026, 12, 5, true);
    pr.p[0].completed = true;
    pr.p[0].snowflakes = 2;
    pr.p[0].star_collected = false;
    cadv_hub_teleport(&h, -11.0f, 0, -4.0f, 0.5f * CADV_PI);
    run(1);
    TEST_ASSERT_EQUAL_INT(1, h.focus_day);
    TEST_ASSERT_NOT_NULL(strstr(h.prompt, "Sleighed!  **  [STAR MISSING]"));
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    run(1);
    TEST_ASSERT_NOT_NULL(strstr(h.message, "DAY 1"));
    pr.p[0].star_collected = true;
    pr.p[0].snowflakes = 1;
    run(1);
    TEST_ASSERT_NOT_NULL(strstr(h.prompt, "*  [STAR]"));
    start(2026, 12, 5, false);
    cadv_hub_teleport(&h, -11.0f, 0, -9.0f, 0.5f * CADV_PI);
    run(1);
    TEST_ASSERT_NOT_NULL(strstr(h.prompt, "STILL THAWING"));
    cadv_hub_set_time(&h, cadv_datetime_make(2026, 12, 24, 9, 0));
    cadv_hub_teleport(&h, 0, 0, -35.0f, 0);
    run(1);
    TEST_ASSERT_TRUE(h.focus_arch || h.focus_day == 24);
}

static void test_player_events_and_queries(void)
{
    uint32_t ev;
    double v;
    in.move_y = 1.0f;
    ev = run(120);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_STEP);
    in.move_y = 0.0f;
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    ev = cadv_hub_tick(&h, &in);
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_JUMP);
    in.pressed = in.held = 0;
    h.player.vel.y = -8.0f; /* fall hard */
    h.player.pos.y = 1.0f;
    h.player.on_ground = false;
    TEST_ASSERT_TRUE(run(60) & CADV_HUB_EV_LANDED);
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "player.z", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "player.x", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "player.y", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "player.yaw", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "stars_carried", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "stars_socketed", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "snowflakes", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "candles", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "crowned", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "epilogue", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "focus", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "beams", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "outcome", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "state.24", &v));
    TEST_ASSERT_TRUE(cadv_hub_query(&h, "reveal.3", &v));
    TEST_ASSERT_FALSE(cadv_hub_query(&h, "state.25", &v));
    TEST_ASSERT_FALSE(cadv_hub_query(&h, "legend.9", &v));
    TEST_ASSERT_FALSE(cadv_hub_query(&h, "nonsense", &v));
}

static void test_hub_bot_walks_into_newest_portal(void)
{
    int i;
    uint32_t ev = 0;
    cadv_input_frame f;
    start(2026, 12, 3, true); /* everything open: newest is 24 */
    pr.p[23].entered = true;  /* skip the reveal pause */
    cadv_hub_update_states(&h);
    for (i = 0; i < 4000 && !(ev & CADV_HUB_EV_ENTER); ++i) {
        cadv_hub_bot(&h, &f);
        ev |= cadv_hub_tick(&h, &f);
    }
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_ENTER);
    TEST_ASSERT_EQUAL_INT(24, h.enter_day);
    start(2026, 11, 20, false); /* nothing open: bot idles */
    cadv_hub_bot(&h, &f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, f.move_y);
}

static void test_hub_bot_balcony_and_ground(void)
{
    cadv_input_frame f;
    int i;
    uint32_t ev = 0;
    cadv_progress_reset(&pr);
    cadv_friends_default(&fr);
    cadv_hub_init(&h, &pr, &fr, cadv_datetime_make(2026, 12, 15, 12, 0), 2026, false);
    /* unshipped portals are never targets: the bot heads for Day 1 */
    cadv_hub_bot(&h, &f);
    TEST_ASSERT_TRUE(f.look_x != 0.0f || f.move_y > 0.0f);
    h.unlock_all = true;
    for (i = 0; i < CADV_PORTAL_COUNT; ++i) {
        pr.p[i].entered = true;
    }
    cadv_hub_set_time(&h, cadv_datetime_make(2026, 12, 15, 12, 0));
    for (i = 15; i < 24; ++i) {
        h.state[i] = CADV_PS_SEALED; /* newest open = 15 (balcony) */
    }
    for (i = 0; i < 4000 && !(ev & CADV_HUB_EV_ENTER); ++i) {
        cadv_hub_bot(&h, &f);
        ev |= cadv_hub_tick(&h, &f);
        if (!(ev & CADV_HUB_EV_ENTER)) {
            int k;
            for (k = 15; k < 24; ++k) {
                h.state[k] = CADV_PS_SEALED;
            }
        }
    }
    TEST_ASSERT_TRUE(ev & CADV_HUB_EV_ENTER);
    TEST_ASSERT_EQUAL_INT(15, h.enter_day);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_layout_slots);
    RUN_TEST(test_layout_paths);
    RUN_TEST(test_states_follow_calendar_and_progress);
    RUN_TEST(test_hub_hall__blocked_sealed_portal);
    RUN_TEST(test_hub_hall__enter_and_return);
    RUN_TEST(test_hub_hall__first_visit_of_a_newly_unlocked_portal);
    RUN_TEST(test_shining_stars__setting_stars_and_arch_messages);
    RUN_TEST(test_shining_stars__missing_stars_message_at_portal_24);
    RUN_TEST(test_beams);
    RUN_TEST(test_tree_crown_epilogue_replay);
    RUN_TEST(test_hub_hall__mark_found_and_legend_prompt);
    RUN_TEST(test_portal_prompts_and_use_messages);
    RUN_TEST(test_player_events_and_queries);
    RUN_TEST(test_hub_bot_walks_into_newest_portal);
    RUN_TEST(test_hub_bot_balcony_and_ground);
    return UNITY_END();
}
