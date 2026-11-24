/* SPDX-License-Identifier: MIT */
/* Day 10 - Neon Syndic: the cell, the crowd, the relays, the car, the tower, the VTOL. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p10_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p10_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff)
{
    if (s.state) {
        cadv_session_end(&s);
    }
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 10;
    cfg.seed = 10;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(10), &cfg));
    st = (p10_state *)s.state;
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
    TEST_ASSERT_TRUE(cadv_p10_query(st, key, &v));
    return v;
}

static void stand(int cx, int cz, float yaw) { cadv_p10_teleport(st, cadv_p10_cell(cx, cz).x, 0.0f, cadv_p10_cell(cx, cz).z, yaw); }

static void aim_at(cadv_vec3 p)
{
    cadv_vec3 e = cadv_p10_eye(st);
    float d = cadv_sqrtf((p.x - e.x) * (p.x - e.x) + (p.z - e.z) * (p.z - e.z));
    st->body.yaw = cadv_atan2f(-(p.x - e.x), -(p.z - e.z));
    st->body.pitch = cadv_atan2f(p.y - e.y, d);
}

static void calm(void)
{
    int i;
    for (i = 0; i < P10_ENEMIES; ++i) {
        st->e[i].active = false;
    }
}

static int bot_run(int max)
{
    int i;
    for (i = 0; i < max && st->outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_input_frame b;
        cadv_p10_bot(st, &b);
        cadv_session_tick(&s, &b);
    }
    return i;
}

static void test_portal_10_neon_syndic__squad_orders(void)
{
    int i;
    int g;
    start(0);
    st->bench = true;
    TEST_ASSERT_EQUAL_INT(3, (int)query("agents"));
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_FOLLOW, (int)query("order"));
    /* the cell keeps formation behind the lead as they walk */
    in.move_y = 1.0f;
    tick(120);
    in.move_y = 0.0f;
    tick(120);
    for (i = 0; i < P10_AGENTS; ++i) {
        TEST_ASSERT_TRUE(cadv_v3_dist(st->ag[i].f.pos, st->body.pos) < 5.0f);
    }
    /* hold: they stay put while the lead walks off */
    press(CADV_ACT_WEAPON_5);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_HOLD, (int)query("order"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "HOLD"));
    in.move_x = 1.0f;
    tick(90);
    in.move_x = 0.0f;
    TEST_ASSERT_TRUE(cadv_v3_dist(st->ag[0].f.pos, st->ag[0].hold) < 1.0f);
    /* the radial ring: look left for cover, release to order */
    in.held |= CADV_ACT_BIT(CADV_ACT_RADIAL);
    in.pressed |= CADV_ACT_BIT(CADV_ACT_RADIAL);
    in.look_x = -0.2f;
    tick(3);
    TEST_ASSERT_TRUE(st->ring.open);
    TEST_ASSERT_EQUAL_INT(3, st->ring.sel);
    in.look_x = 0.0f;
    in.held = 0;
    tick(1);
    TEST_ASSERT_FALSE(st->ring.open);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_COVER, (int)query("order"));
    /* attack with nobody hostile: refused; with a guard in the crosshair: on it */
    press(CADV_ACT_WEAPON_6);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "NO TARGET"));
    g = 0;
    st->e[g].hurt = 300; /* a guard who was shot at turns hostile */
    aim_at(cadv_v3(st->e[g].f.pos.x, 1.1f, st->e[g].f.pos.z));
    press(CADV_ACT_WEAPON_6);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_ATTACK, (int)query("order"));
    TEST_ASSERT_EQUAL_INT(g, st->ag[0].target);
    st->e[g].active = false;
    tick(2);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_FOLLOW, (int)query("order")); /* target gone: back to follow */
    press(CADV_ACT_WEAPON_4);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_FOLLOW, (int)query("order"));
}

static void test_portal_10_neon_syndic__charm_beacon_crowd(void)
{
    int i;
    cadv_portal_result r;
    start(0);
    st->bench = true;
    stand(18, 46, 0.0f);
    /* bring the market crowd close and hold the beacon */
    for (i = 0; i < P10_CIVS; ++i) {
        float a = (float)i * 0.4f;
        st->civ[i].f.pos = cadv_v3(st->body.pos.x + cadv_sinf(a) * (1.0f + (float)(i % 5)), 0, st->body.pos.z + cadv_cosf(a) * (1.0f + (float)(i % 5)));
    }
    in.held = CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
    tick(30);
    TEST_ASSERT_EQUAL_INT(0, (int)query("followers")); /* it takes a moment */
    tick(60 * 4);
    TEST_ASSERT_TRUE(query("followers") >= 6);
    tick(60 * 12);
    in.held = 0;
    TEST_ASSERT_TRUE(query("followers.max") >= 30); /* the crowd lets more follow */
    TEST_ASSERT_TRUE(query("followers") <= 40);
    /* a guard is swayed too once the crowd is big (but not past the limit) */
    for (i = 10; i < P10_CIVS; ++i) {
        st->civ[i].active = false;
    }
    tick(1);
    TEST_ASSERT_TRUE(query("followers") >= 4 && query("followers") <= 10);
    st->e[0].f.pos = cadv_v3(st->body.pos.x + 2.0f, 0, st->body.pos.z);
    in.held = CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
    for (i = 0; i < 60 * 6 && !st->e[0].ally; ++i) {
        tick(1);
    }
    in.held = 0;
    TEST_ASSERT_TRUE(st->e[0].ally);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "GUARD JOINS"));
    /* followers soak up fire */
    st->bench = false;
    st->alert = 90.0f;
    {
        int t = 100;
        while (st->civ[t - 100].state != P10_CIV_FOLLOW) {
            ++t;
        }
        st->civ[t - 100].hp = 1;
        st->civ[t - 100].f.pos = cadv_v3(st->body.pos.x + 1.0f, 0, st->body.pos.z);
    }
    tick(1);
    /* the record stands in the result */
    st->win_timer = 1;
    tick(1);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_EQUAL_INT(3, r.snowflakes); /* done, nobody lost, 30+ in the crowd */
}

static void test_portal_10_neon_syndic__overclock_sliders(void)
{
    float base;
    int i;
    start(0);
    calm();
    st->bench = true;
    stand(3, 58, -CADV_PI * 0.5f);
    in.move_y = 1.0f;
    tick(60);
    base = cadv_sqrtf(st->body.vel.x * st->body.vel.x + st->body.vel.z * st->body.vel.z);
    press(CADV_ACT_RUN);
    TEST_ASSERT_EQUAL_INT(1, (int)query("overclock"));
    stand(3, 58, -CADV_PI * 0.5f);
    tick(60);
    TEST_ASSERT_TRUE(cadv_sqrtf(st->body.vel.x * st->body.vel.x + st->body.vel.z * st->body.vel.z) > base * 1.4f);
    in.move_y = 0.0f;
    press(CADV_ACT_CROUCH);
    press(CADV_ACT_MAP);
    TEST_ASSERT_EQUAL_INT(3, (int)query("overclock"));
    TEST_ASSERT_TRUE(st->marks_found & 2u); /* the codename agent sees through walls */
    for (i = 0; i < 60 * 6 && !st->oc.crash_t; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(1, (int)query("crashed"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CRASH"));
    press(CADV_ACT_RUN);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "WON'T MOVE"));
    in.move_y = 1.0f;
    stand(3, 58, -CADV_PI * 0.5f);
    tick(60);
    TEST_ASSERT_TRUE(cadv_sqrtf(st->body.vel.x * st->body.vel.x + st->body.vel.z * st->body.vel.z) < base); /* legs of lead */
    in.move_y = 0.0f;
    tick(60 * 6);
    TEST_ASSERT_EQUAL_INT(0, (int)query("crashed"));
}

static void test_portal_10_neon_syndic__hacking_relays(void)
{
    int i;
    start(0);
    calm();
    /* the tower relay refuses before its turn */
    stand(88, 47, -CADV_PI * 0.5f);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "PROTECTED"));
    stand(17, 47, -CADV_PI * 0.5f);
    st->body.pos.x = st->relay[0].x + 1.6f;
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(0, (int)query("hacking"));
    /* a miss raises the alert */
    st->hack.angle = cadv_wrap_angle(st->hack.window + CADV_PI);
    if (st->hack.angle < 0.0f) {
        st->hack.angle += 2.0f * CADV_PI;
    }
    press(CADV_ACT_USE);
    TEST_ASSERT_TRUE(query("alert") >= 15.0);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "SLIPPED"));
    /* jump abandons, use starts again; then three good taps */
    press(CADV_ACT_JUMP);
    TEST_ASSERT_EQUAL_INT(-1, (int)query("hacking"));
    tick(60);
    press(CADV_ACT_USE);
    for (i = 0; i < 60 * 30 && st->hacking >= 0; ++i) {
        if (cadv_absf(cadv_wrap_angle(st->hack.angle - st->hack.window)) < st->hack.width * 0.5f) {
            press(CADV_ACT_USE);
        } else {
            tick(1);
        }
    }
    TEST_ASSERT_EQUAL_INT(1, (int)query("relays"));
    TEST_ASSERT_EQUAL_INT(P10_ACT_INDUSTRIAL, (int)query("act"));
    TEST_ASSERT_EQUAL_INT(1, st->checkpoint);
    TEST_ASSERT_EQUAL_INT(15, (int)query("gauss"));
    /* nothing else to use here */
    stand(4, 58, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "NOTHING"));
}

static void test_portal_10_neon_syndic__car_rams_barrier(void)
{
    int i;
    int before;
    start(1);
    calm();
    TEST_ASSERT_EQUAL_INT(P10_ACT_INDUSTRIAL, (int)query("act"));
    before = (int)query("barrier");
    TEST_ASSERT_EQUAL_INT(24, before);
    TEST_ASSERT_FALSE(cadv_nav_walkable(&st->nav, 46, 29));
    cadv_p10_teleport(st, st->car.pos.x + 1.5f, 0, st->car.pos.z, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(1, (int)query("driving"));
    /* line up on the industrial road and floor it north */
    st->car.pos = cadv_v3(93.0f, 0, 72.0f);
    st->car.yaw = 0.0f;
    in.move_y = 1.0f;
    for (i = 0; i < 60 * 6 && st->fac[P10_FAC_BARRIER].alive == before; ++i) {
        tick(1);
    }
    TEST_ASSERT_TRUE(query("barrier") < before);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CRUNCH"));
    TEST_ASSERT_TRUE(cadv_nav_walkable(&st->nav, 46, 29));
    TEST_ASSERT_TRUE(query("alert") >= 14.0);
    /* too fast to get out; brake, then out */
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(1, (int)query("driving"));
    in.move_y = -1.0f;
    tick(30);
    in.move_y = 0.0f;
    tick(120);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(0, (int)query("driving"));
    /* the wall stops the car: drive into a building */
    press(CADV_ACT_USE);
    st->car.pos = cadv_v3(93.0f, 0, 30.0f);
    st->car.yaw = -CADV_PI * 0.5f; /* east, into the factory wall */
    in.move_y = 1.0f;
    tick(120);
    TEST_ASSERT_TRUE(st->car.pos.x < 100.0f);
    in.move_x = 1.0f;
    in.move_y = -1.0f;
    tick(30);
    in.move_x = 0.0f;
    in.move_y = 0.0f;
}

static void test_portal_10_neon_syndic__facades_and_gauss(void)
{
    int i;
    int before;
    start(1);
    calm();
    /* the shop front onto the back alley: laser chips it, gauss blows it open */
    stand(25, 50, 0.0f);
    before = st->fac[1].alive;
    aim_at(cadv_facade_centre(&st->fac[1], 1, 0));
    press(CADV_ACT_WEAPON_2);
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(2);
    in.held = 0;
    TEST_ASSERT_TRUE(st->tracers[0].ttl > 0 || st->tracers[1].ttl > 0);
    for (i = 0; i < 4; ++i) {
        st->cd = 0.0f;
        in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
        tick(1);
        in.held = 0;
    }
    TEST_ASSERT_TRUE(st->fac[1].alive < before);
    press(CADV_ACT_WEAPON_3);
    TEST_ASSERT_EQUAL_INT(P10_W_GAUSS, (int)query("weapon"));
    aim_at(cadv_facade_centre(&st->fac[1], 0, 1));
    st->cd = 0.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    tick(30);
    TEST_ASSERT_TRUE(st->fac[1].alive < before - 3);
    TEST_ASSERT_EQUAL_INT(9, (int)query("gauss"));
    /* out of slugs */
    st->gauss_ammo = 0;
    st->cd = 0.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_NOT_NULL(strstr(st->message, "NO SLUGS"));
    press(CADV_ACT_NEXT_WEAPON);
    TEST_ASSERT_EQUAL_INT(P10_W_MINIGUN, (int)query("weapon"));
    press(CADV_ACT_PREV_WEAPON);
    TEST_ASSERT_EQUAL_INT(P10_W_GAUSS, (int)query("weapon"));
}

static void test_portal_10_neon_syndic__alert_and_reinforcements(void)
{
    int i;
    int g;
    float hp;
    start(0);
    for (i = 0; i < P10_ENEMIES; ++i) {
        if (st->e[i].active && st->e[i].kind == P10_E_GUARD) {
            TEST_ASSERT_FALSE(cadv_p10_hostile(st, i)); /* calm: guards patrol */
        }
    }
    tick(60 * 3);
    /* open fire on a guard with the minigun */
    g = 0;
    stand(13, 46, 0.0f);
    st->e[g].f.pos = cadv_v3(st->body.pos.x + 6.0f, 0, st->body.pos.z);
    st->e[g].f.mode = CADV_FOLLOW_WAIT;
    hp = (float)st->e[g].hp;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    for (i = 0; i < 90; ++i) {
        aim_at(cadv_v3(st->e[g].f.pos.x, 1.1f, st->e[g].f.pos.z));
        tick(1);
    }
    in.held = 0;
    TEST_ASSERT_TRUE(!st->e[g].active || st->e[g].hp < hp);
    TEST_ASSERT_TRUE(query("alert") > 5.0);
    /* raise it to the alarm: a van unloads */
    st->alert = 95.0f;
    st->reinforce_t = 1;
    tick(2);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "VAN"));
    TEST_ASSERT_TRUE(query("hostiles") >= 2);
    /* the civilians scatter */
    tick(60 * 3);
    {
        int fled = 0;
        for (i = 0; i < P10_CIVS; ++i) {
            fled += st->civ[i].state == P10_CIV_FLEE ? 1 : 0;
        }
        TEST_ASSERT_TRUE(fled > 0);
    }
    /* left alone it cools */
    calm();
    st->alert = 50.0f;
    tick(60 * 10);
    TEST_ASSERT_TRUE(query("alert") < 40.0);
}

static void test_portal_10_neon_syndic__tower_and_star(void)
{
    int i;
    start(2);
    TEST_ASSERT_EQUAL_INT(P10_ACT_TOWER, (int)query("act"));
    /* the relay is locked while turrets stand */
    stand(88, 47, -CADV_PI * 0.5f);
    st->body.pos.x = st->relay[2].x - 1.6f;
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "TURRETS LOCK"));
    for (i = 0; i < P10_ENEMIES; ++i) {
        if (st->e[i].active && st->e[i].kind == P10_E_TURRET) {
            st->e[i].hp = 1;
            st->e[i].hurt = 0;
        }
    }
    /* the laser finishes one */
    for (i = 0; i < P10_ENEMIES; ++i) {
        if (st->e[i].active && st->e[i].kind == P10_E_TURRET) {
            break;
        }
    }
    stand(70, 47, 0.0f);
    press(CADV_ACT_WEAPON_2);
    {
        int t = i;
        int k;
        for (k = 0; k < 30 && st->e[t].active; ++k) {
            aim_at(cadv_v3(st->e[t].f.pos.x, 1.4f, st->e[t].f.pos.z));
            st->cd = 0.0f;
            in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
            tick(1);
        }
        in.held = 0;
        TEST_ASSERT_FALSE(st->e[t].active);
    }
    calm();
    stand(88, 47, -CADV_PI * 0.5f);
    st->body.pos.x = st->relay[2].x - 1.6f;
    press(CADV_ACT_USE);
    for (i = 0; i < 60 * 30 && st->hacking >= 0; ++i) {
        if (cadv_absf(cadv_wrap_angle(st->hack.angle - st->hack.window)) < st->hack.width * 0.5f) {
            press(CADV_ACT_USE);
        } else {
            tick(1);
        }
    }
    TEST_ASSERT_TRUE(st->casing_open);
    TEST_ASSERT_EQUAL_INT(3, (int)query("relays"));
    cadv_p10_teleport(st, st->star_pos.x, 0, st->star_pos.z, 0.0f); /* the star waits right by the relay */
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("star.taken"));
    TEST_ASSERT_EQUAL_INT(P10_ACT_PLAZA, (int)query("act"));
    TEST_ASSERT_EQUAL_INT(3, st->checkpoint);
    TEST_ASSERT_EQUAL_INT(100, (int)query("player.health"));
}

static void test_portal_10_neon_syndic__extraction(void)
{
    cadv_portal_result r;
    int i;
    start(3);
    st->bench = true;
    TEST_ASSERT_EQUAL_INT(P10_ACT_PLAZA, (int)query("act"));
    TEST_ASSERT_EQUAL_INT(60, (int)query("hold"));
    /* away from the pad the clock waits */
    tick(60);
    TEST_ASSERT_EQUAL_INT(60, (int)query("hold"));
    cadv_p10_teleport(st, st->pad.x, 0, st->pad.z - 10.0f, 0.0f);
    for (i = 0; i < P10_HOLD + 5 && !st->vtol_down; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(1, (int)query("vtol"));
    cadv_p10_teleport(st, st->pad.x, 0, st->pad.z, 0.0f);
    tick(1);
    TEST_ASSERT_EQUAL_INT(P10_ACT_DONE, (int)query("act"));
    st->agent_lost = true;
    tick(240);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_TRUE(r.star_collected);
    TEST_ASSERT_EQUAL_INT(1, r.snowflakes);
    /* waves come while the VTOL is out */
    start(3);
    cadv_p10_teleport(st, st->pad.x, 0, st->pad.z - 10.0f, 0.0f);
    st->reinforce_t = 1;
    tick(2);
    TEST_ASSERT_TRUE(query("hostiles") >= 2);
}

static void test_portal_10_neon_syndic__losing_the_lead(void)
{
    int i;
    start(2);
    stand(70, 47, 0.0f);
    /* stand in the gauntlet and take it */
    for (i = 0; i < 60 * 60 && st->outcome == CADV_OUTCOME_RUNNING; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, st->outcome);
    TEST_ASSERT_EQUAL_INT(0, (int)query("player.health"));
    /* agents can fall too */
    start(2);
    st->ag[1].hp = 1;
    st->ag[1].f.pos = cadv_p10_cell(72, 47);
    st->ag[1].order = CADV_ORDER_HOLD;
    st->ag[1].hold = st->ag[1].f.pos;
    for (i = 0; i < 60 * 20 && st->ag[1].alive; ++i) {
        tick(1);
    }
    TEST_ASSERT_FALSE(st->ag[1].alive);
    TEST_ASSERT_TRUE(st->agent_lost);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "IS DOWN"));
}

static void test_portal_10_neon_syndic__marks_and_queries(void)
{
    static const char *const names[] = {"Ann", "Bo", "Cyd", "Dee", "Eli"};
    double v;
    cadv_session_end(&s);
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 10;
    cfg.seed = 3;
    cfg.friends = names;
    cfg.friend_count = 5;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(10), &cfg));
    st = (p10_state *)s.state;
    st->bench = true;
    TEST_ASSERT_EQUAL_STRING("DEE", st->names[0]);
    TEST_ASSERT_EQUAL_STRING("CYD CYBERNETICS", st->mark_text[0]);
    stand(18, 40, 0.0f);
    tick(2);
    TEST_ASSERT_TRUE(st->marks_found & 1u);
    stand(29, 53, 0.0f);
    tick(2);
    TEST_ASSERT_TRUE(st->marks_found & 4u);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "ELI WAS HERE"));
    TEST_ASSERT_EQUAL_INT((int)st->marks_found, (int)query("marks"));
    TEST_ASSERT_FALSE(cadv_p10_query(st, "nonsense", &v));
    TEST_ASSERT_TRUE(query("stamina") > 99.0);
    TEST_ASSERT_TRUE(query("player.y") > 1.0);
    TEST_ASSERT_TRUE(query("ticks") > 0.0);
    TEST_ASSERT_EQUAL_INT(0, (int)query("outcome"));
    TEST_ASSERT_EQUAL_INT(0, (int)query("star.taken"));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, (float)query("car.speed"));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, (float)query("player.yaw"));
    TEST_ASSERT_TRUE(query("player.x") > 0.0 && query("player.z") > 0.0);
    press(CADV_ACT_PAUSE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, st->outcome);
    tick(1);
}

static void test_portal_10_neon_syndic__bot_wins_on_every_difficulty(void)
{
    int d;
    for (d = 0; d < 3; ++d) {
        cadv_portal_result r;
        start_with(d == 1 ? 0 : d + 1, (cadv_difficulty)d);
        bot_run(60 * 60 * 30);
        r = cadv_session_result(&s);
        TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
        TEST_ASSERT_TRUE(r.star_collected);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_10_neon_syndic__squad_orders);
    RUN_TEST(test_portal_10_neon_syndic__charm_beacon_crowd);
    RUN_TEST(test_portal_10_neon_syndic__overclock_sliders);
    RUN_TEST(test_portal_10_neon_syndic__hacking_relays);
    RUN_TEST(test_portal_10_neon_syndic__car_rams_barrier);
    RUN_TEST(test_portal_10_neon_syndic__facades_and_gauss);
    RUN_TEST(test_portal_10_neon_syndic__alert_and_reinforcements);
    RUN_TEST(test_portal_10_neon_syndic__tower_and_star);
    RUN_TEST(test_portal_10_neon_syndic__extraction);
    RUN_TEST(test_portal_10_neon_syndic__losing_the_lead);
    RUN_TEST(test_portal_10_neon_syndic__marks_and_queries);
    RUN_TEST(test_portal_10_neon_syndic__bot_wins_on_every_difficulty);
    return UNITY_END();
}
