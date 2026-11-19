/* SPDX-License-Identifier: MIT */
/* Day 4 - Ice Tomb of the Winter King: levers, doors, water, the chase, the door. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p04_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p04_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 4;
    cfg.seed = 7;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(4), &cfg));
    st = (p04_state *)s.state;
}

static void start(int checkpoint) { start_with(checkpoint, CADV_DIFF_DEFAULT); }

void setUp(void) { memset(&in, 0, sizeof in); }
void tearDown(void) { cadv_session_end(&s); }

static void tick(int n)
{
    int i;
    for (i = 0; i < n; ++i) {
        cadv_session_tick(&s, &in);
        in.pressed = 0;
        in.look_x = in.look_y = 0.0f;
    }
}

static void press(cadv_action a)
{
    in.pressed |= CADV_ACT_BIT(a);
    in.held |= CADV_ACT_BIT(a);
    tick(1);
    in.held &= ~CADV_ACT_BIT(a);
}

static double query(const char *key)
{
    double v = -1.0;
    TEST_ASSERT_TRUE(cadv_p04_query(st, key, &v));
    return v;
}

static void calm(void)
{
    int i;
    for (i = 0; i < P04_MAX_ENEMIES; ++i) {
        if (st->enemies[i].type != P04_COLOSSUS) {
            st->enemies[i].active = false;
        }
    }
}

static void stand(float x, float y, float z, float yaw, float pitch)
{
    cadv_p04_teleport(st, x, y, z, yaw);
    st->me.pitch = pitch;
}

static void test_portal_catalog__portal_04_ice_tomb(void)
{
    const cadv_portal_desc *d = cadv_portal_get(4);
    int n;
    TEST_ASSERT_TRUE(d->implemented);
    TEST_ASSERT_EQUAL_STRING("p04_ice_tomb", d->id);
    TEST_ASSERT_TRUE(d->state_size >= sizeof(p04_state));
    start(0);
    TEST_ASSERT_EQUAL_INT(0, (int)query("gears"));
    TEST_ASSERT_EQUAL_INT(100, (int)query("player.health"));
    TEST_ASSERT_EQUAL_INT(0, (int)query("door.open"));
    TEST_ASSERT_TRUE(cadv_p04_route(&n) != NULL && n > 50);
    TEST_ASSERT_EQUAL_STRING("JOURNAL OF FOEN-X", st->mark_text[2]);
}

static void test_portal_04_ice_tomb__lever_opens_the_timed_door(void)
{
    cadv_vec3 l = cadv_p04_lever_pos(P04_LEVER_HALL);
    start(0);
    calm();
    stand(l.x - 0.8f, l.y, l.z, -0.5f * CADV_PI, -0.45f);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(1, (int)query("door.open"));
    TEST_ASSERT_EQUAL_STRING("A DOOR GRINDS OPEN SOMEWHERE - HURRY!", st->message);
    tick(14 * 60);
    TEST_ASSERT_EQUAL_INT(0, (int)query("door.open")); /* it shuts again */
    TEST_ASSERT_FALSE(st->lever[P04_LEVER_HALL]);    /* and the lever springs back */
    /* standing in the doorway keeps it open */
    press(CADV_ACT_USE);
    stand(5.5f, 6.0f, 51.5f, 0.0f, 0.0f);
    st->door_timer = 1;
    tick(2);
    TEST_ASSERT_EQUAL_INT(1, (int)query("door.open"));
    /* taking gear 1 jams it open for good */
    stand(2.5f, 6.0f, 51.5f, 0.0f, 0.0f);
    tick(2);
    TEST_ASSERT_EQUAL_INT(1, (int)query("gears"));
    TEST_ASSERT_EQUAL_INT(1, st->checkpoint);
    tick(30 * 60);
    TEST_ASSERT_EQUAL_INT(1, (int)query("door.open"));
}

static void test_block_puzzle_reaches_the_platform(void)
{
    start(0);
    calm();
    TEST_ASSERT_EQUAL_INT(0, (int)query("block"));
    cadv_tiles_set(&st->tiles, 19, 6, 56, CADV_TILE_AIR);
    cadv_tiles_set(&st->tiles, 21, 6, 61, CADV_TILE_BLOCK);
    TEST_ASSERT_EQUAL_INT(1, (int)query("block"));
}

static void test_portal_04_ice_tomb__pool_lever_and_the_air_meter(void)
{
    cadv_vec3 l = cadv_p04_lever_pos(P04_LEVER_POOL);
    int i;
    start(1);
    calm();
    stand(l.x, 2.3f, l.z + 0.8f, 0.0f, -0.6f);
    tick(2);
    TEST_ASSERT_EQUAL_INT(CADV_CLIMB_SWIM, (int)query("player.state"));
    press(CADV_ACT_USE);
    TEST_ASSERT_TRUE(st->lever[P04_LEVER_POOL]);
    TEST_ASSERT_EQUAL_INT(CADV_TILE_WATER, cadv_tiles_get(&st->tiles, 15, 3, 18)); /* the grate is gone */
    /* stay under: breath runs out, then it hurts, then Mara drowns */
    for (i = 0; i < 60 * 60 && st->deaths == 0; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(1, st->deaths);
    TEST_ASSERT_EQUAL_INT(100, st->hp);
    TEST_ASSERT_EQUAL_STRING("MARA FALLS... BACK TO THE LAST CRYSTAL", st->message);
}

static void test_portal_04_ice_tomb__save_crystals_and_respawn(void)
{
    cadv_vec3 c = cadv_p04_crystal_pos(0);
    start(1);
    calm();
    stand(c.x, c.y, c.z + 1.0f, 0.0f, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(1, (int)query("crystals"));
    TEST_ASSERT_EQUAL_STRING("SAVE CRYSTAL - PROGRESS SAVED", st->message);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(2, (int)query("crystals"));
    /* fall into the abyss: back at the crystal */
    cadv_tiles_fill(&st->tiles, 6, 0, 60, 8, 5, 62, CADV_TILE_AIR);
    stand(7.5f, 6.0f, 61.5f, 0.0f, 0.0f);
    tick(200);
    TEST_ASSERT_EQUAL_INT(1, (int)query("deaths"));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, c.z - 1.0f, (float)query("player.z"));
    TEST_ASSERT_EQUAL_STRING("SWALLOWED BY THE GLACIER...", st->message);
}

static void test_journal_vent_and_ice_wall(void)
{
    float yaw;
    cadv_vec3 j = cadv_p04_plaque_pos(P04_PLAQUE_JOURNAL, &yaw);
    cadv_vec3 l = cadv_p04_lever_pos(P04_LEVER_VENT);
    start(2);
    calm();
    stand(j.x + 1.2f, 4.0f, j.z, 0.5f * CADV_PI, 0.0f);
    tick(1); /* seeing the journal registers the mark first */
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_STRING("\"WARM AIR FROM THE VENTS MELTS THE ICE.\"", st->message);
    TEST_ASSERT_TRUE(st->marks_found & 4u);
    stand(l.x - 0.9f, l.y, l.z + 0.5f, -0.5f * CADV_PI, -0.45f);
    press(CADV_ACT_USE);
    TEST_ASSERT_TRUE(st->lever[P04_LEVER_VENT]);
    TEST_ASSERT_EQUAL_INT(0, (int)query("ice.melted"));
    tick(160);
    TEST_ASSERT_EQUAL_INT(1, (int)query("ice.melted"));
    TEST_ASSERT_EQUAL_INT(CADV_TILE_AIR, cadv_tiles_get(&st->tiles, 34, 7, 28));
}

static void test_portal_04_ice_tomb__the_colossus_chase(void)
{
    int i;
    start(2);
    calm();
    st->ice_melted = true;
    cadv_tiles_fill(&st->tiles, 32, 6, 28, 37, 12, 29, CADV_TILE_AIR);
    stand(34.5f, 6.0f, 7.2f, 0.0f, 0.0f);
    tick(5);
    TEST_ASSERT_EQUAL_INT(3, (int)query("gears"));
    TEST_ASSERT_EQUAL_INT(1, (int)query("chase"));
    /* standing still: the floor goes and the Colossus catches up */
    for (i = 0; i < 600 && st->deaths == 0; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(1, st->deaths);
    TEST_ASSERT_EQUAL_INT(0, (int)query("chase"));
    TEST_ASSERT_EQUAL_INT(2, (int)query("gears")); /* gear 3 back on its pedestal */
    TEST_ASSERT_EQUAL_INT(CADV_TILE_ROCK, cadv_tiles_get(&st->tiles, 34, 5, 10)); /* floor restored */
    /* reaching the solar chamber ends the chase */
    stand(34.5f, 6.0f, 7.2f, 0.0f, 0.0f);
    tick(2);
    TEST_ASSERT_EQUAL_INT(1, (int)query("chase"));
    stand(34.5f, 6.0f, 45.5f, 0.0f, 0.0f);
    tick(2);
    TEST_ASSERT_EQUAL_INT(0, (int)query("chase"));
}

static void test_portal_04_ice_tomb__three_gears_open_the_solar_door(void)
{
    cadv_vec3 d = cadv_p04_solar_door();
    cadv_portal_result r;
    int i;
    start(2);
    calm();
    stand(d.x, d.y, d.z - 1.8f, CADV_PI, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_STRING("THE SOLAR DOOR WANTS THREE SUN GEARS (2/3)", st->message);
    st->gear_bits = 7u;
    st->gears = 3;
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(1, (int)query("gears.set"));
    for (i = 0; i < 10 && !st->star_taken; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(1, (int)query("star.taken"));
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_RUNNING, cadv_session_result(&s).outcome); /* banner first */
    tick(150);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_TRUE(r.star_collected);
    TEST_ASSERT_EQUAL_INT(2, r.snowflakes); /* no saves used, no jade */
}

static void test_portal_catalog__portal_04_win_condition(void)
{
    cadv_portal_result r;
    start(2);
    st->gear_bits = 7u;
    st->gears = 3;
    st->jade = 3;
    st->crystal_uses = 5;
    st->gears_set = true;
    st->star_active = true;
    st->star_pos = cadv_v3_add(st->me.pos, cadv_v3(0, 1, 0));
    tick(400);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_EQUAL_INT(2, r.snowflakes); /* win + jade, too many saves */
}

static void test_portal_04_ice_tomb__resume_at_a_checkpoint(void)
{
    start(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("gears"));
    TEST_ASSERT_EQUAL_INT(1, (int)query("door.open"));
    TEST_ASSERT_EQUAL_INT(1, (int)query("block"));
    TEST_ASSERT_TRUE(query("route") > 10);
    cadv_session_end(&s);
    start(2);
    TEST_ASSERT_EQUAL_INT(2, (int)query("gears"));
    TEST_ASSERT_EQUAL_INT(CADV_TILE_WATER, cadv_tiles_get(&st->tiles, 15, 3, 18));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 27.5f, (float)query("player.x"));
}

static void test_blade_marks_pickups_and_fall_damage(void)
{
    int hp;
    start(1);
    calm();
    /* the blade hurts when it sweeps through you */
    stand(cadv_p04_blade_pos(st).x, 6.0f, 39.5f, 0.0f, 0.0f);
    while (st->ticks % 20 != 19) {
        stand(cadv_p04_blade_pos(st).x, 6.0f, 39.5f, 0.0f, 0.0f);
        tick(1);
    }
    stand(cadv_p04_blade_pos(st).x, 6.0f, 39.5f, 0.0f, 0.0f);
    tick(1);
    TEST_ASSERT_TRUE(st->hp < 100);
    TEST_ASSERT_EQUAL_STRING("THE BLADE!", st->message);
    /* a medical kit heals; at full health it stays */
    stand(12.5f, 6.0f, 62.5f, 0.0f, 0.0f);
    tick(2);
    TEST_ASSERT_EQUAL_INT(100, st->hp);
    /* the blade carving is a friend's mark */
    stand(15.8f, 6.0f, 41.5f, 0.5f * CADV_PI, 0.3f);
    tick(1);
    TEST_ASSERT_TRUE(st->marks_found & 2u);
    /* a long fall hurts */
    hp = st->hp;
    stand(15.5f, 12.0f, 54.5f, 0.0f, 0.0f);
    tick(120);
    TEST_ASSERT_TRUE(st->hp < hp);
    /* jade */
    stand(22.5f, 9.0f, 56.5f, 0.0f, 0.0f);
    tick(2);
    TEST_ASSERT_EQUAL_INT(1, (int)query("jade"));
}

static void test_creatures_and_pistols(void)
{
    int w;
    start(0);
    w = 0; /* first wolf on the raised floor */
    stand(st->enemies[w].pos.x, 6.0f, st->enemies[w].pos.z + 4.0f, 0.0f, -0.1f);
    tick(60);
    TEST_ASSERT_TRUE(st->hp < 100); /* it bites */
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(120);
    in.held = 0;
    TEST_ASSERT_FALSE(st->enemies[w].active); /* auto-aim finds it */
    /* a bat swoops */
    stand(st->enemies[2].pos.x, 6.0f, st->enemies[2].pos.z + 2.0f, 0.0f, 0.5f);
    tick(200);
    TEST_ASSERT_TRUE(st->hurt_flash > 0 || st->hp < 100);
}

static void test_quit_and_queries(void)
{
    double v;
    start(0);
    TEST_ASSERT_FALSE(cadv_p04_query(st, "nope", &v));
    query("player.x");
    query("player.y");
    query("player.yaw");
    query("air");
    query("outcome");
    query("marks");
    query("ticks");
    press(CADV_ACT_PAUSE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    tick(1);
}

static void run_bot(cadv_difficulty diff, int checkpoint)
{
    int i;
    start_with(checkpoint, diff);
    for (i = 0; i < 40000 && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_p04_bot(st, &in);
        cadv_session_tick(&s, &in);
    }
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    cadv_session_end(&s);
}

static void test_bot_finishes_on_every_difficulty(void)
{
    run_bot(CADV_DIFF_STORY, 0);
    run_bot(CADV_DIFF_DEFAULT, 0);
    run_bot(CADV_DIFF_HARD, 0);
    run_bot(CADV_DIFF_DEFAULT, 1);
    start(0);
    st->route = 9999;
    cadv_p04_bot(st, &in); /* past the end: idle */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_catalog__portal_04_ice_tomb);
    RUN_TEST(test_portal_04_ice_tomb__lever_opens_the_timed_door);
    RUN_TEST(test_block_puzzle_reaches_the_platform);
    RUN_TEST(test_portal_04_ice_tomb__pool_lever_and_the_air_meter);
    RUN_TEST(test_portal_04_ice_tomb__save_crystals_and_respawn);
    RUN_TEST(test_journal_vent_and_ice_wall);
    RUN_TEST(test_portal_04_ice_tomb__the_colossus_chase);
    RUN_TEST(test_portal_04_ice_tomb__three_gears_open_the_solar_door);
    RUN_TEST(test_portal_catalog__portal_04_win_condition);
    RUN_TEST(test_portal_04_ice_tomb__resume_at_a_checkpoint);
    RUN_TEST(test_blade_marks_pickups_and_fall_damage);
    RUN_TEST(test_creatures_and_pistols);
    RUN_TEST(test_quit_and_queries);
    RUN_TEST(test_bot_finishes_on_every_difficulty);
    return UNITY_END();
}
