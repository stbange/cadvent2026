/* SPDX-License-Identifier: MIT */
/* Day 9 - Skifrost: training, flags, power, weapons, the match and the lap. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p09_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p09_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff)
{
    if (s.state) {
        cadv_session_end(&s);
    }
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 9;
    cfg.seed = 9;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(9), &cfg));
    st = (p09_state *)s.state;
}

static void start(int cp) { start_with(cp, CADV_DIFF_DEFAULT); }

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
    TEST_ASSERT_TRUE(cadv_p09_query(st, key, &v));
    return v;
}

/* a running match with the bots frozen */
static void match(void)
{
    start(1);
    st->bench = true;
    tick(P09_COUNT + 1);
    TEST_ASSERT_EQUAL_INT(P09_PH_MATCH, st->phase);
}

static void put(int i, cadv_vec3 p)
{
    st->a[i].k.pos = p;
    st->a[i].k.vel = cadv_v3(0, 0, 0);
    st->a[i].k.on_ground = true;
}

static void aim_at(cadv_vec3 p)
{
    cadv_vec3 e = cadv_skier_eye(&st->a[0].k);
    float d = cadv_sqrtf((p.x - e.x) * (p.x - e.x) + (p.z - e.z) * (p.z - e.z));
    st->a[0].k.yaw = cadv_atan2f(-(p.x - e.x), -(p.z - e.z));
    st->a[0].k.pitch = cadv_atan2f(p.y - e.y, d);
}

static int bot_run(int max)
{
    int i;
    for (i = 0; i < max && st->outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_input_frame b;
        cadv_p09_bot(st, &b);
        cadv_session_tick(&s, &b);
    }
    return i;
}

static void test_portal_09_skifrost__training_course(void)
{
    int i;
    start(0);
    TEST_ASSERT_EQUAL_INT(P09_PH_TRAIN, (int)query("phase"));
    TEST_ASSERT_EQUAL_INT(P09_TR_SKI, (int)query("train"));
    /* hold ski off the edge of the hill towards the basin */
    put(0, cadv_v3(256, st->base[0].stand.y, 96));
    st->a[0].k.yaw = CADV_PI;
    in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    for (i = 0; i < 60 * 12 && st->train == P09_TR_SKI; ++i) {
        tick(1);
    }
    in.held = 0;
    TEST_ASSERT_EQUAL_INT(P09_TR_JET, st->train);
    TEST_ASSERT_TRUE(query("speed") > 100.0);
    /* reach the beacon */
    put(0, cadv_v3(st->beacon.x, st->beacon.y - 1.5f, st->beacon.z));
    st->a[0].k.on_ground = false;
    tick(1);
    TEST_ASSERT_EQUAL_INT(P09_TR_DISC, st->train);
    /* the bot leads the drone and hits it; the match follows */
    for (i = 0; i < 60 * 20 && st->train != P09_TR_DONE; ++i) {
        cadv_input_frame b;
        cadv_p09_bot(st, &b);
        cadv_session_tick(&s, &b);
    }
    TEST_ASSERT_EQUAL_INT(P09_TR_DONE, st->train);
    TEST_ASSERT_EQUAL_INT(P09_PH_COUNT, st->phase);
    TEST_ASSERT_EQUAL_INT(1, st->checkpoint);
    tick(P09_COUNT + 1);
    TEST_ASSERT_EQUAL_INT(P09_PH_MATCH, st->phase);
    TEST_ASSERT_EQUAL_INT(20 * 60, (int)query("time") + 1);
    /* E skips the course */
    start(0);
    tick(40);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(P09_PH_COUNT, st->phase);
}

static void test_portal_09_skifrost__capture_the_flag(void)
{
    match();
    /* take their flag at a run, then bring it home */
    put(0, cadv_v3(st->base[1].stand.x, st->base[1].stand.y - 0.25f, st->base[1].stand.z + 5.0f));
    st->a[0].k.vel = cadv_v3(0, 0, -48.0f); /* gliding over the stand */
    in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    tick(4);
    in.held = 0;
    TEST_ASSERT_EQUAL_INT(1, (int)query("carrying"));
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_CARRIED, (int)query("flag.theirs"));
    TEST_ASSERT_TRUE(st->a[0].carry_max > P09_FAST);
    /* their carrier holds ours: no capture */
    cadv_flag_touch(&st->flag[0], 4, 1);
    put(0, cadv_v3(st->base[0].stand.x, st->base[0].stand.y - 0.25f, st->base[0].stand.z));
    tick(1);
    TEST_ASSERT_EQUAL_INT(0, (int)query("score.us"));
    cadv_flag_return(&st->flag[0]);
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("score.us"));
    TEST_ASSERT_EQUAL_INT(1, (int)query("caps"));
    TEST_ASSERT_EQUAL_INT(1, (int)query("fast"));
    TEST_ASSERT_EQUAL_INT(0, (int)query("carrying"));
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_HOME, (int)query("flag.theirs"));
    TEST_ASSERT_EQUAL_INT(2, st->checkpoint); /* our captures are kept */
    TEST_ASSERT_NOT_NULL(strstr(st->message, "RIME WOLVES SCORE"));
    /* the Owls score too, but that saves nothing */
    cadv_flag_touch(&st->flag[0], 4, 1);
    put(4, cadv_v3(st->base[1].stand.x, st->base[1].stand.y - 0.25f, st->base[1].stand.z));
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("score.them"));
    TEST_ASSERT_EQUAL_INT(2, st->checkpoint);
}

static void test_portal_09_skifrost__flag_return_timer(void)
{
    int i;
    match();
    put(0, cadv_v3(st->base[1].stand.x, st->base[1].stand.y - 0.25f, st->base[1].stand.z));
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("carrying"));
    /* toss it off the plateau */
    put(0, cadv_v3(200, 0, 300));
    put(0, cadv_v3(200, cadv_terrain_height(&st->terrain, 200, 300), 300));
    press(CADV_ACT_CROUCH);
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_DROPPED, (int)query("flag.theirs"));
    put(0, cadv_v3(100, cadv_terrain_height(&st->terrain, 100, 100), 100)); /* walk away */
    tick(60 * 3);
    TEST_ASSERT_TRUE(st->flag[1].pos.y <= cadv_terrain_height(&st->terrain, st->flag[1].pos.x, st->flag[1].pos.z) + 0.01f);
    for (i = 0; i < 60 * 40 && st->flag[1].state == CADV_FLAG_DROPPED; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_HOME, st->flag[1].state);
    TEST_ASSERT_TRUE(i > 60 * 25);
    TEST_ASSERT_NOT_NULL(strstr(st->feed[0], "WENT HOME"));
    /* a defender touching the dropped flag returns it at once */
    put(0, cadv_v3(st->base[1].stand.x, st->base[1].stand.y - 0.25f, st->base[1].stand.z));
    tick(1);
    put(0, cadv_v3(200, cadv_terrain_height(&st->terrain, 200, 300), 300));
    press(CADV_ACT_CROUCH);
    tick(60);
    put(6, st->flag[1].pos);
    put(0, cadv_v3(100, cadv_terrain_height(&st->terrain, 100, 100), 100));
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_HOME, st->flag[1].state);
    TEST_ASSERT_NOT_NULL(strstr(st->feed[0], "RETURNED"));
}

static void test_portal_09_skifrost__generator_powers_turret(void)
{
    int i;
    int shots = 0;
    match();
    /* an Owl walks into range of our turret: it fires */
    put(4, cadv_v3(st->base[0].turret.pos.x + 20.0f, st->base[0].stand.y - 0.25f, st->base[0].turret.pos.z + 20.0f));
    put(0, cadv_v3(60, cadv_terrain_height(&st->terrain, 60, 60), 60));
    for (i = 0; i < 60 * 8; ++i) {
        tick(1);
        if (st->base[0].turret.reload > 2.9f) {
            ++shots;
        }
        put(4, cadv_v3(st->base[0].turret.pos.x + 20.0f, st->base[0].stand.y - 0.25f, st->base[0].turret.pos.z + 20.0f));
    }
    TEST_ASSERT_TRUE(shots >= 2);
    /* our generator goes down: the turret falls silent and the station is dark */
    TEST_ASSERT_TRUE(cadv_generator_damage(&st->base[0].power, 1000.0f));
    TEST_ASSERT_EQUAL_INT(0, (int)query("gen.ours"));
    tick(10);
    shots = 0;
    for (i = 0; i < 60 * 8; ++i) {
        tick(1);
        shots += st->base[0].turret.reload > 2.9f ? 1 : 0;
    }
    TEST_ASSERT_EQUAL_INT(0, shots);
    put(0, st->base[0].station);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "STATION IS DARK"));
    /* the crew repairs it */
    tick(60 * 45);
    TEST_ASSERT_EQUAL_INT(1, (int)query("gen.ours"));
    /* explosives at their generator */
    st->base[1].power.hp = 20.0f;
    put(0, cadv_v3(st->base[1].gen.x + 8.0f, st->base[1].stand.y - 0.25f, st->base[1].gen.z));
    aim_at(st->base[1].gen);
    st->a[0].cd = 0.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    tick(30);
    TEST_ASSERT_EQUAL_INT(0, (int)query("gen.theirs"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "EMBER OWLS' GENERATOR IS DOWN"));
}

static void test_portal_09_skifrost__station_changes_armour(void)
{
    match();
    TEST_ASSERT_EQUAL_INT(CADV_ARMOUR_LIGHT, (int)query("armour"));
    press(CADV_ACT_WEAPON_3); /* light: two slots only */
    TEST_ASSERT_EQUAL_INT(P09_DISC, (int)query("weapon"));
    put(0, st->base[0].station);
    st->a[0].k.pos.x += 1.5f;
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(CADV_ARMOUR_MEDIUM, (int)query("armour"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "MEDIUM ARMOUR + ENERGY PACK"));
    press(CADV_ACT_WEAPON_3);
    TEST_ASSERT_EQUAL_INT(P09_GRENADE, (int)query("weapon"));
    press(CADV_ACT_NEXT_WEAPON);
    TEST_ASSERT_EQUAL_INT(P09_DISC, (int)query("weapon"));
    press(CADV_ACT_PREV_WEAPON);
    TEST_ASSERT_EQUAL_INT(P09_GRENADE, (int)query("weapon"));
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(CADV_ARMOUR_HEAVY, (int)query("armour"));
    TEST_ASSERT_FALSE(st->a[0].k.energy_pack);
    TEST_ASSERT_EQUAL_FLOAT(180.0f, (float)query("player.health"));
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(CADV_ARMOUR_LIGHT, (int)query("armour"));
    TEST_ASSERT_EQUAL_INT(P09_DISC, (int)query("weapon")); /* the grenade slot is gone */
}

static void test_portal_09_skifrost__discs_splash_and_push(void)
{
    cadv_vec3 at;
    float hp;
    int i;
    match();
    put(0, cadv_v3(256, cadv_terrain_height(&st->terrain, 256, 200), 200));
    at = cadv_v3(256, cadv_terrain_height(&st->terrain, 256, 230), 230);
    put(5, at);
    hp = st->a[5].k.hp;
    st->a[5].invuln = 0.0f;
    aim_at(cadv_p09_centre(&st->a[5]));
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    for (i = 0; i < 60 && st->a[5].k.hp >= hp; ++i) {
        tick(1);
        st->a[5].k.pos.x = at.x;
        st->a[5].k.pos.z = at.z;
    }
    TEST_ASSERT_TRUE(st->a[5].k.hp < hp - 30.0f); /* a direct hit */
    TEST_ASSERT_EQUAL_INT(0, st->a[5].last_hit_by);
    /* a mid-air disc kill: the announcer's special */
    put(6, cadv_v3(256, at.y + 12.0f, 230));
    st->a[6].k.on_ground = false;
    st->a[6].k.airtime = 1.0f;
    st->a[6].k.hp = 5.0f;
    st->a[6].invuln = 0.0f;
    aim_at(cadv_p09_centre(&st->a[6]));
    st->a[0].cd = 0.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    for (i = 0; i < 60 && st->a[6].alive; ++i) {
        st->a[6].k.pos = cadv_v3(256, at.y + 12.0f, 230);
        st->a[6].k.vel = cadv_v3(0, 0, 0);
        tick(1);
    }
    TEST_ASSERT_FALSE(st->a[6].alive);
    TEST_ASSERT_EQUAL_INT(1, (int)query("kills"));
    TEST_ASSERT_TRUE(st->marks_found & 1u);
    TEST_ASSERT_NOT_NULL(strstr(st->feed[0], "BLOODMAKER SPECIAL"));
    /* respawn back home */
    tick(P09_RESPAWN + 2);
    TEST_ASSERT_TRUE(st->a[6].alive);
    TEST_ASSERT_TRUE(st->a[6].k.pos.z > 400.0f);
    /* a disc jump: your own blast pushes you up and hurts a little */
    put(0, cadv_v3(256, cadv_terrain_height(&st->terrain, 256, 200), 200));
    st->a[0].invuln = 0.0f;
    st->a[0].k.pitch = -1.45f;
    st->a[0].cd = 0.0f;
    hp = st->a[0].k.hp;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    tick(2);
    TEST_ASSERT_TRUE(st->a[0].k.vel.y > 5.0f);
    TEST_ASSERT_TRUE(st->a[0].k.hp < hp && st->a[0].k.hp > hp - 20.0f);
}

static void test_portal_09_skifrost__chaingun_and_grenades(void)
{
    cadv_vec3 at;
    float hp;
    int i;
    match();
    put(0, cadv_v3(256, cadv_terrain_height(&st->terrain, 256, 200), 200));
    at = cadv_v3(256, cadv_terrain_height(&st->terrain, 256, 210), 210);
    put(7, at);
    st->a[7].invuln = 0.0f;
    hp = st->a[7].k.hp;
    press(CADV_ACT_WEAPON_2);
    TEST_ASSERT_EQUAL_INT(P09_CHAIN, (int)query("weapon"));
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    for (i = 0; i < 90; ++i) {
        aim_at(cadv_p09_centre(&st->a[7]));
        put(7, at);
        tick(1);
    }
    in.held = 0;
    TEST_ASSERT_TRUE(st->a[0].spin >= 1.0f);
    TEST_ASSERT_TRUE(st->a[7].k.hp < hp - 10.0f);
    tick(30);
    TEST_ASSERT_TRUE(st->a[0].spin < 1.0f); /* winds down */
    /* chaingun on their generator */
    st->a[7].alive = false;
    st->a[7].respawn = 100000;
    put(0, cadv_v3(st->base[1].gen.x + 6.0f, st->base[1].stand.y - 0.25f, st->base[1].gen.z));
    hp = st->base[1].power.hp;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    for (i = 0; i < 60; ++i) {
        aim_at(st->base[1].gen);
        tick(1);
    }
    in.held = 0;
    TEST_ASSERT_TRUE(st->base[1].power.hp < hp);
    /* a grenade arcs, bounces and goes off on its fuse */
    put(0, st->base[0].station);
    st->a[0].k.pos.x += 1.5f;
    press(CADV_ACT_USE);
    press(CADV_ACT_WEAPON_3);
    put(0, cadv_v3(256, cadv_terrain_height(&st->terrain, 256, 200), 200));
    st->a[0].k.yaw = CADV_PI;
    st->a[0].k.pitch = -0.6f;
    st->a[0].cd = 0.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_TRUE(st->proj[0].active || st->proj[1].active);
    tick(30);
    {
        int k;
        bool live = false;
        for (k = 0; k < P09_PROJ; ++k) {
            live = live || (st->proj[k].active && st->proj[k].kind == P09_PJ_GRENADE);
        }
        TEST_ASSERT_TRUE(live); /* bounced, still ticking */
    }
    tick(120);
    TEST_ASSERT_TRUE(st->boom_t > 0 || st->boom_pos.z > 200.0f);
}

static void test_portal_09_skifrost__time_and_sudden_death(void)
{
    match();
    st->timer = 1;
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("sudden"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "SUDDEN DEATH"));
    st->score[1] = 1;
    tick(1);
    TEST_ASSERT_EQUAL_INT(240, st->end_timer);
    tick(240);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, st->outcome);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
    /* ahead at the whistle: the lap; behind: lost */
    match();
    st->score[0] = 2;
    st->timer = 1;
    tick(1);
    TEST_ASSERT_EQUAL_INT(P09_PH_LAP, st->phase);
    match();
    st->score[1] = 2;
    st->timer = 1;
    tick(1);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "OWLS LEAD"));
    match();
    st->score[1] = 5;
    tick(1);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "TAKE THE FESTIVAL"));
    match();
    st->sudden = true;
    st->score[0] = 1;
    tick(1);
    TEST_ASSERT_EQUAL_INT(P09_PH_LAP, st->phase);
}

static void test_portal_09_skifrost__victory_lap(void)
{
    cadv_portal_result r;
    start(P09_LAP_CP);
    TEST_ASSERT_EQUAL_INT(P09_PH_LAP, (int)query("phase"));
    TEST_ASSERT_EQUAL_INT(5, (int)query("score.us"));
    TEST_ASSERT_EQUAL_INT(0, (int)query("star.carried"));
    put(0, cadv_v3(st->star_pos.x, st->star_pos.y - 1.4f, st->star_pos.z));
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("star.carried"));
    put(0, cadv_v3(st->base[0].stand.x, st->base[0].stand.y - 0.25f, st->base[0].stand.z + 1.0f));
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("star.taken"));
    tick(240);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_TRUE(r.star_collected);
    TEST_ASSERT_EQUAL_INT(2, r.snowflakes); /* 5-0, no fast capture */
    /* resuming after captures keeps them, the Owls start over */
    start(3);
    TEST_ASSERT_EQUAL_INT(2, (int)query("score.us"));
    TEST_ASSERT_EQUAL_INT(0, (int)query("score.them"));
    TEST_ASSERT_EQUAL_INT(P09_PH_COUNT, (int)query("phase"));
    start(P09_LAP_CP + 3);
    TEST_ASSERT_EQUAL_INT(3, (int)query("score.them"));
}

static void test_portal_09_skifrost__marks_and_queries(void)
{
    static const char *const names[] = {"Ann", "Bo", "Cyd", "Dee", "Eli", "Fay"};
    cadv_vec3 e;
    double v;
    cadv_session_end(&s);
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 9;
    cfg.seed = 3;
    cfg.checkpoint = 1;
    cfg.friends = names;
    cfg.friend_count = 6;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(9), &cfg));
    st = (p09_state *)s.state;
    st->bench = true;
    TEST_ASSERT_EQUAL_STRING("CYD", st->names[1]);
    TEST_ASSERT_EQUAL_STRING("DEE", st->names[4]);
    TEST_ASSERT_EQUAL_STRING("FAY HALFPIPE", st->mark_text[3]);
    tick(P09_COUNT + 20);
    TEST_ASSERT_TRUE(st->marks_found & 2u); /* the team mate at the spawn */
    put(4, cadv_v3(256, cadv_terrain_height(&st->terrain, 256, 240), 240));
    put(0, cadv_v3(256, cadv_terrain_height(&st->terrain, 256, 200), 200));
    tick(20);
    TEST_ASSERT_TRUE(st->marks_found & 4u); /* the Owl in sight */
    put(0, cadv_v3(130, cadv_terrain_height(&st->terrain, 130, 256), 256));
    tick(20);
    TEST_ASSERT_TRUE(st->marks_found & 8u);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "FAY HALFPIPE"));
    TEST_ASSERT_TRUE(cadv_p09_in_halfpipe(st->a[0].k.pos));
    TEST_ASSERT_EQUAL_INT((int)st->marks_found, (int)query("marks"));
    TEST_ASSERT_FALSE(cadv_p09_query(st, "nonsense", &v));
    cadv_p09_teleport(st, 300, 0, 300, 1.0f);
    e = cadv_skier_eye(&st->a[0].k);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (float)query("player.y"), e.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, (float)query("player.yaw"));
    TEST_ASSERT_TRUE(query("energy") > 0.0);
    TEST_ASSERT_TRUE(query("time") > 0.0);
    TEST_ASSERT_EQUAL_INT(0, (int)query("outcome"));
    TEST_ASSERT_TRUE(query("ticks") > 0.0);
    TEST_ASSERT_FLOAT_WITHIN(3.5f, 0.9f, cadv_p09_skill(st, 0));
    press(CADV_ACT_MAP);
    press(CADV_ACT_PAUSE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, st->outcome);
    tick(1); /* nothing more happens */
}

static void test_portal_09_skifrost__bot_wins_on_every_difficulty(void)
{
    int d;
    for (d = 0; d < 3; ++d) {
        cadv_portal_result r;
        start_with(d == 1 ? 0 : 1, (cadv_difficulty)d);
        bot_run(60 * 60 * 32);
        r = cadv_session_result(&s);
        TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
        TEST_ASSERT_TRUE(r.star_collected);
        TEST_ASSERT_TRUE(r.snowflakes >= 1);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_09_skifrost__training_course);
    RUN_TEST(test_portal_09_skifrost__capture_the_flag);
    RUN_TEST(test_portal_09_skifrost__flag_return_timer);
    RUN_TEST(test_portal_09_skifrost__generator_powers_turret);
    RUN_TEST(test_portal_09_skifrost__station_changes_armour);
    RUN_TEST(test_portal_09_skifrost__discs_splash_and_push);
    RUN_TEST(test_portal_09_skifrost__chaingun_and_grenades);
    RUN_TEST(test_portal_09_skifrost__time_and_sudden_death);
    RUN_TEST(test_portal_09_skifrost__victory_lap);
    RUN_TEST(test_portal_09_skifrost__marks_and_queries);
    RUN_TEST(test_portal_09_skifrost__bot_wins_on_every_difficulty);
    return UNITY_END();
}
