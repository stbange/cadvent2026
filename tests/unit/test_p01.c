/* SPDX-License-Identifier: MIT */
/* Day 1 - Sleighed!: rules of the Glitterdome Mall. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p01_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p01_state *st;
static cadv_input_frame in;

static void start(int checkpoint, cadv_difficulty diff)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 1;
    cfg.seed = 7;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    cfg.data_dir = CADV_SOURCE_DIR;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(1), &cfg));
    st = (p01_state *)s.state;
}

void setUp(void) { memset(&in, 0, sizeof in); }
void tearDown(void) { cadv_session_end(&s); }

static void tick(int n)
{
    int i;
    for (i = 0; i < n; ++i) {
        cadv_session_tick(&s, &in);
        in.pressed = 0;
    }
}

static void peace(void)
{
    int i;
    for (i = 0; i < st->fps.enemy_count; ++i) {
        st->fps.enemies[i].active = false; /* calm the mall for rule tests */
    }
}

static void face_prop(const char *kind)
{
    int i;
    for (i = 0; i < st->map.nprop; ++i) {
        if (strcmp(st->map.prop[i].name, kind) == 0) {
            cadv_vec3 p = st->map.prop[i].pos;
            float dx = 1.2f;
            /* stand west of the prop and look east, or east and look west */
            if (p.x > 30.0f || (p.x > 0 && strcmp(kind, "sink") == 0)) {
                dx = -1.2f;
            }
            if (strcmp(kind, "fountain") == 0) {
                cadv_p01_teleport(st, p.x, 0, p.z + 1.3f, 0.0f);
            } else {
                cadv_p01_teleport(st, p.x + dx, 0, p.z, dx > 0 ? 0.5f * CADV_PI : -0.5f * CADV_PI);
            }
            st->fps.player.pitch = -0.3f;
            return;
        }
    }
    TEST_FAIL_MESSAGE("prop missing");
}

static void test_portal_catalog__portal_01_sleighed(void)
{
    const cadv_portal_desc *d = cadv_portal_get(1);
    TEST_ASSERT_TRUE(d->implemented);
    TEST_ASSERT_EQUAL_STRING("Sleighed!", d->title);
    start(0, CADV_DIFF_DEFAULT);
    TEST_ASSERT_EQUAL_STRING("The Glitterdome Mall", st->map.name);
    TEST_ASSERT_TRUE(st->fps.enemy_count >= 20);
    TEST_ASSERT_EQUAL_INT(5, st->map.nsecret);
    TEST_ASSERT_EQUAL_INT(4, st->map.nmark);
    TEST_ASSERT_TRUE(st->boss_index >= 0);
    TEST_ASSERT_FALSE(st->fps.enemies[st->boss_index].active);
    TEST_ASSERT_EQUAL_INT(P01_WEAPONS, st->fps.wcount);
    TEST_ASSERT_EQUAL_STRING("Shrinkray", cadv_p01_weapons()[P01_WEAPON_SHRINK].name);
    TEST_ASSERT_EQUAL_STRING("Boarlord Overseer", cadv_p01_enemies()[P01_BOSS].name);
    TEST_ASSERT_NOT_NULL(strstr(st->mark_text[0], "DR. DOOM"));
}

static void test_missing_map_fails(void)
{
    cadv_session bad;
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 1;
    cfg.data_dir = "/nonexistent/";
    TEST_ASSERT_FALSE(cadv_session_start(&bad, cadv_portal_get(1), &cfg));
}

static void test_props_vending_fountain_sink_arcade(void)
{
    start(0, CADV_DIFF_DEFAULT);
    peace();
    face_prop("vending");
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_STRING("YOU'RE NOT THIRSTY", st->message);
    st->fps.player.health = 50;
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_INT(70, st->fps.player.health);
    st->fps.player.health = 50;
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_STRING("SOLD OUT", st->message);
    face_prop("fountain");
    st->fps.player.health = 50;
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_INT(52, st->fps.player.health);
    st->fps.player.health = 100;
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_STRING("REFRESHING. BUT YOU'RE FINE.", st->message);
    face_prop("arcade");
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_TRUE(st->secrets_found & (1u << 5));
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_STRING("GAME OVER. INSERT COIN.", st->message);
}

static void test_staff_door_needs_three_fuses(void)
{
    double v;
    start(0, CADV_DIFF_DEFAULT);
    peace();
    cadv_p01_teleport(st, 12.5f, 0, -18.0f, 0.0f);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_STRING("STAFF ONLY - NEEDS 3 FUSE CANDLES (0/3)", st->message);
    st->fps.player.keys = P01_KEYS_ALL;
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(60);
    TEST_ASSERT_TRUE(st->fps.doors[0].open >= 1.0f);
    TEST_ASSERT_TRUE(cadv_p01_query(st, "keys", &v));
    TEST_ASSERT_EQUAL_DOUBLE(7.0, v);
}

static void test_secrets_triggers_checkpoints_and_lift(void)
{
    const cadv_map_item *lift;
    start(0, CADV_DIFF_DEFAULT);
    peace();
    cadv_p01_teleport(st, 13.5f, 0, 19.0f, 0); /* garage closet */
    tick(1);
    TEST_ASSERT_TRUE(st->secrets_found & 1u);
    TEST_ASSERT_EQUAL_STRING("SECRET FOUND!", st->message);
    cadv_p01_teleport(st, 0, 0, -5.0f + 3.0f, 0); /* atrium zone quip */
    tick(1);
    TEST_ASSERT_TRUE(st->zones_seen & 1u);
    cadv_p01_teleport(st, 12.5f, 0, -22.5f, 0); /* checkpoint 1 */
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, st->checkpoint);
    st->fps.player.health = 60;
    st->fps.player.keys = 5;
    lift = cadv_map_find_trigger(&st->map, "lift");
    cadv_p01_teleport(st, 12.5f, 0, 0.5f * (lift->pos.z + lift->hi.z), 0);
    tick(2);
    TEST_ASSERT_TRUE(st->fps.player.pos.y > 11.0f); /* up on the roof */
    TEST_ASSERT_EQUAL_INT(60, st->fps.player.health); /* the lift keeps your stats */
    TEST_ASSERT_EQUAL_UINT32(5, st->fps.player.keys);
    TEST_ASSERT_EQUAL_INT(2, st->checkpoint);
    TEST_ASSERT_TRUE(st->boss_awake);
}

static void test_checkpoint_resume(void)
{
    start(1, CADV_DIFF_DEFAULT);
    TEST_ASSERT_EQUAL_UINT32(P01_KEYS_ALL, st->fps.player.keys);
    TEST_ASSERT_TRUE(st->fps.owned[P01_WEAPON_SHRINK]);
    TEST_ASSERT_FALSE(st->fps.world.s[st->fps.doors[0].solid].enabled);
    TEST_ASSERT_TRUE(st->fps.player.pos.z < -20.0f);
    cadv_session_end(&s);
    start(2, CADV_DIFF_HARD);
    TEST_ASSERT_TRUE(st->fps.player.pos.y > 11.0f);
    TEST_ASSERT_TRUE(st->fps.owned[P01_WEAPON_CHAINGUN]);
    TEST_ASSERT_TRUE(st->boss_awake);
}

static void test_siphon_star_exit_and_snowflakes(void)
{
    int i;
    cadv_portal_result r;
    const cadv_map_item *ex;
    start(2, CADV_DIFF_DEFAULT);
    peace();
    for (i = 0; i < st->fps.breakable_count; ++i) {
        if (st->fps.breakables[i].group == 1) {
            st->fps.breakables[i].hp = 1;
            cadv_fps_explode(&st->fps, cadv_v3_scale(cadv_v3_add(st->fps.world.s[st->fps.breakables[i].solid].box.lo,
                                                                   st->fps.world.s[st->fps.breakables[i].solid].box.hi), 0.5f),
                             1.0f, 10, false);
        }
    }
    tick(1);
    TEST_ASSERT_TRUE(st->siphon_down);
    TEST_ASSERT_TRUE(st->collapse_ticks > 0);
    TEST_ASSERT_TRUE(st->star_pickup >= 0);
    cadv_p01_teleport(st, 0, 12, -43.6f, 0);
    tick(2);
    TEST_ASSERT_TRUE(st->fps.star_taken);
    ex = cadv_map_find_trigger(&st->map, "exit");
    cadv_p01_teleport(st, 0, 12, 0.5f * (ex->pos.z + ex->hi.z), 0);
    tick(2);
    TEST_ASSERT_TRUE(st->win_timer > 0);
    tick(40); /* the banner shows for a moment before USE skips it */
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_TRUE(r.star_collected);
    TEST_ASSERT_EQUAL_INT(2, r.snowflakes); /* fast, but not all secrets */
    st->secrets_found = 0x3f;
    TEST_ASSERT_EQUAL_INT(3, cadv_session_result(&s).snowflakes);
}

static void test_dome_collapse_loses(void)
{
    start(2, CADV_DIFF_STORY);
    peace();
    st->siphon_down = true;
    st->collapse_ticks = 3;
    tick(5);
    TEST_ASSERT_TRUE(st->death_timer > 0);
    tick(130);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
}

static void test_death_quit_and_low_health(void)
{
    start(0, CADV_DIFF_DEFAULT);
    peace();
    st->fps.player.health = 20;
    tick(1);
    TEST_ASSERT_TRUE(st->low_health_quipped);
    st->fps.player.health = 1;
    st->fps.player.armor = 0;
    cadv_fps_explode(&st->fps, cadv_player_eye(&st->fps.player), 2.0f, 100, true);
    tick(1);
    tick(130);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
    cadv_session_end(&s);
    start(0, CADV_DIFF_DEFAULT);
    in.pressed = CADV_ACT_BIT(CADV_ACT_PAUSE);
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    tick(1); /* ignored after the end */
}

static void test_marks_and_queries(void)
{
    double v;
    const char *keys[] = {"player.x", "player.y", "player.z", "player.yaw", "player.health", "keys", "enemies.alive",
                          "secrets", "emitters", "siphon_down", "boss_awake", "star.taken", "collapse", "outcome",
                          "marks", "ticks", "route", "jetpack"};
    size_t i;
    start(0, CADV_DIFF_DEFAULT);
    peace();
    /* the wanted poster in the garage (mark index 2) */
    cadv_p01_teleport(st, -9.0f, 0, 20.0f, 0.5f * CADV_PI);
    st->fps.player.pitch = 0.1f;
    tick(2);
    TEST_ASSERT_TRUE(st->marks_found & (1u << 2));
    for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        TEST_ASSERT_TRUE(cadv_p01_query(st, keys[i], &v));
    }
    TEST_ASSERT_FALSE(cadv_p01_query(st, "bogus", &v));
}

static void test_kills_shrink_stomp_quips(void)
{
    int e;
    start(0, CADV_DIFF_DEFAULT);
    peace();
    e = st->links.enemy_index[0];
    st->fps.enemies[e].active = true;
    st->fps.enemies[e].pos = cadv_v3(0, 0, 25);
    st->fps.player.pos = cadv_v3(0, 0, 27);
    st->fps.player.yaw = 0;
    st->fps.owned[P01_WEAPON_SHRINK] = true;
    st->fps.weapon = P01_WEAPON_SHRINK;
    st->fps.ammo[3] = 5;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_EQUAL_INT(CADV_ENEMY_SHRUNK, st->fps.enemies[e].state);
    in.move_y = 1.0f;
    tick(60);
    TEST_ASSERT_EQUAL_INT(CADV_ENEMY_DEAD, st->fps.enemies[e].state);
    TEST_ASSERT_EQUAL_STRING("Squeak. Squish. Season's greetings.", st->quip);
}

static void test_bot_finishes_on_every_difficulty(void)
{
    cadv_difficulty d;
    for (d = CADV_DIFF_STORY; d <= CADV_DIFF_HARD; ++d) {
        int i;
        start(0, d);
        for (i = 0; i < 20000 && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
            cadv_input_frame f;
            cadv_p01_bot(st, &f);
            cadv_session_tick(&s, &f);
        }
        TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
        TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
        cadv_session_end(&s);
    }
    start(0, CADV_DIFF_DEFAULT); /* keep tearDown happy */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_catalog__portal_01_sleighed);
    RUN_TEST(test_missing_map_fails);
    RUN_TEST(test_props_vending_fountain_sink_arcade);
    RUN_TEST(test_staff_door_needs_three_fuses);
    RUN_TEST(test_secrets_triggers_checkpoints_and_lift);
    RUN_TEST(test_checkpoint_resume);
    RUN_TEST(test_siphon_star_exit_and_snowflakes);
    RUN_TEST(test_dome_collapse_loses);
    RUN_TEST(test_death_quit_and_low_health);
    RUN_TEST(test_marks_and_queries);
    RUN_TEST(test_kills_shrink_stomp_quips);
    RUN_TEST(test_bot_finishes_on_every_difficulty);
    return UNITY_END();
}
