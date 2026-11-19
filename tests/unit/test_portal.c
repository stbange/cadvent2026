/* SPDX-License-Identifier: MIT */
#include <string.h>

#include "portal/portal.h"
#include "portals/test_range.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;

void setUp(void)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 1;
    cfg.seed = 7;
    cfg.difficulty = CADV_DIFF_DEFAULT;
    cfg.data_dir = CADV_SOURCE_DIR; /* Day 1 loads its map from assets/ */
}
void tearDown(void) { cadv_session_end(&s); }

static void test_portal_catalog__catalogue_completeness(void)
{
    int day;
    TEST_ASSERT_NULL(cadv_portal_get(0));
    TEST_ASSERT_NULL(cadv_portal_get(25));
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        const cadv_portal_desc *d = cadv_portal_get(day);
        TEST_ASSERT_EQUAL_INT(day, d->day);
        TEST_ASSERT_NOT_NULL(d->title);
        TEST_ASSERT_TRUE(d->caps & CADV_CAP_SOLO);
        TEST_ASSERT_EQUAL(day <= 5, d->implemented); /* real games ship one by one */
    }
    TEST_ASSERT_EQUAL_STRING("Iron Yule", cadv_portal_get(6)->title);
    TEST_ASSERT_EQUAL_STRING("Polar Alert", cadv_portal_get(24)->title);
}

static void test_session_lifecycle(void)
{
    cadv_portal_result r;
    cadv_input_frame in;
    memset(&in, 0, sizeof in);
    TEST_ASSERT_FALSE(cadv_session_active(&s));
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, r.outcome);
    cadv_session_tick(&s, &in); /* no-op without a portal */
    TEST_ASSERT_FALSE(cadv_session_start(&s, NULL, &cfg));
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(1), &cfg));
    TEST_ASSERT_TRUE(cadv_session_active(&s));
    cadv_session_tick(&s, &in);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_RUNNING, r.outcome);
    TEST_ASSERT_EQUAL_INT(1, r.ticks);
    cadv_session_end(&s);
    TEST_ASSERT_FALSE(cadv_session_active(&s));
    TEST_ASSERT_TRUE(s.arena_peak >= cadv_test_range_state_size());
    cadv_session_end(&s); /* double end is harmless */
}

static bool fail_init(void *state, const cadv_portal_cfg *c) { (void)state; (void)c; return false; }

static void test_session_init_failure(void)
{
    cadv_portal_desc d = *cadv_portal_get(1);
    d.init = fail_init;
    TEST_ASSERT_FALSE(cadv_session_start(&s, &d, &cfg));
    TEST_ASSERT_FALSE(cadv_session_active(&s));
}

static int gpu_freed;
static void fake_free(void *state) { (void)state; gpu_freed++; }

static void test_session_calls_gpu_free(void)
{
    gpu_freed = 0;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(2), &cfg));
    s.gpu_free = fake_free;
    cadv_session_end(&s);
    TEST_ASSERT_EQUAL_INT(1, gpu_freed);
}

static void play_bot(int max)
{
    int i;
    for (i = 0; i < max; ++i) {
        cadv_input_frame in;
        if (cadv_session_result(&s).outcome != CADV_OUTCOME_RUNNING) {
            break;
        }
        s.desc->bot(s.state, &in);
        cadv_session_tick(&s, &in);
    }
}

static void test_portal_framework__story_mode_selection(void)
{
    cadv_portal_result r;
    cfg.difficulty = CADV_DIFF_STORY;
    cfg.day = 9;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(9), &cfg));
    play_bot(20000);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_TRUE(r.star_collected); /* shining_stars__star_reachable_in_story_mode */
    TEST_ASSERT_TRUE(r.snowflakes >= 1 && r.snowflakes <= 2); /* no speed star in story mode */
}

static void test_shining_stars__star_reachable_in_story_mode(void)
{
    cfg.difficulty = CADV_DIFF_STORY;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(1), &cfg));
    play_bot(20000);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
}

static void test_bot_wins_on_hard(void)
{
    cfg.difficulty = CADV_DIFF_HARD;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(6), &cfg)); /* test range */
    play_bot(30000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
}

static void test_queries_teleport_and_quit(void)
{
    double v;
    cadv_input_frame in;
    const char *keys[] = {"player.x", "player.y", "player.z", "player.yaw", "player.health", "enemies.alive",
                          "star.taken", "keys", "outcome", "marks", "ticks", "door.open"};
    size_t i;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(6), &cfg));
    for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        TEST_ASSERT_TRUE(s.desc->query(s.state, keys[i], &v));
    }
    TEST_ASSERT_FALSE(s.desc->query(s.state, "bogus", &v));
    s.desc->teleport(s.state, 1, 0, 2, 0.5f);
    s.desc->query(s.state, "player.z", &v);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, v);
    memset(&in, 0, sizeof in);
    in.pressed = CADV_ACT_BIT(CADV_ACT_PAUSE);
    cadv_session_tick(&s, &in);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    cadv_session_tick(&s, &in); /* ticks after the end are ignored */
}

static void test_death_loses_and_locked_door_message(void)
{
    cadv_input_frame in;
    int i;
    memset(&in, 0, sizeof in);
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(6), &cfg));
    /* walk to the locked door and use it */
    s.desc->teleport(s.state, 0, 0, -7.0f, 0.0f);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    cadv_session_tick(&s, &in);
    in.pressed = 0;
    /* stand still until the imps win */
    for (i = 0; i < 20000 && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_session_tick(&s, &in);
    }
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
}

static void test_checkpoint_spawn(void)
{
    double v;
    cfg.checkpoint = 1;
    cfg.day = 6;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(6), &cfg));
    s.desc->query(s.state, "keys", &v);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, v);
    TEST_ASSERT_EQUAL_INT(1, cadv_session_result(&s).checkpoint);
}

static void test_custom_friend_names_reach_the_portal(void)
{
    const char *names[] = {"Ana", "Ben"};
    cfg.friends = names;
    cfg.friend_count = 2;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(1), &cfg));
    cadv_session_end(&s);
    {
        static const char *many[14] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n"};
        cfg.friends = many;
        cfg.friend_count = 14; /* more than CADV_MAX_FRIENDS: clamped */
        TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(1), &cfg));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_catalog__catalogue_completeness);
    RUN_TEST(test_session_lifecycle);
    RUN_TEST(test_session_init_failure);
    RUN_TEST(test_session_calls_gpu_free);
    RUN_TEST(test_portal_framework__story_mode_selection);
    RUN_TEST(test_shining_stars__star_reachable_in_story_mode);
    RUN_TEST(test_bot_wins_on_hard);
    RUN_TEST(test_queries_teleport_and_quit);
    RUN_TEST(test_death_loses_and_locked_door_message);
    RUN_TEST(test_checkpoint_spawn);
    RUN_TEST(test_custom_friend_names_reach_the_portal);
    return UNITY_END();
}
