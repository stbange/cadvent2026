/* SPDX-License-Identifier: MIT */
/* Day 8 - Cascade: the arrival, the test, the ruins, Blackice, the array. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p08_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p08_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 8;
    cfg.seed = 7;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(8), &cfg));
    st = (p08_state *)s.state;
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
    TEST_ASSERT_TRUE(cadv_p08_query(st, key, &v));
    return v;
}

static void stand(int cx, int cz, float yaw) { cadv_p08_teleport(st, cadv_p08_cell(cx, cz).x, 0.0f, cadv_p08_cell(cx, cz).z, yaw); }

static void face(cadv_vec3 p)
{
    cadv_vec3 e = cadv_p08_eye(st);
    st->body.yaw = cadv_atan2f(-(p.x - e.x), -(p.z - e.z));
    st->body.pitch = cadv_atan2f(p.y - e.y, cadv_sqrtf((p.x - e.x) * (p.x - e.x) + (p.z - e.z) * (p.z - e.z)));
}

static void clear_enemies(void)
{
    int i;
    st->t_blackice.fired = 1; /* no squads unless a test asks for them */
    for (i = 0; i < P08_ENEMIES; ++i) {
        st->e[i].active = false;
    }
}

static void test_portal_catalog__portal_08_cascade(void)
{
    const cadv_portal_desc *d = cadv_portal_get(8);
    TEST_ASSERT_EQUAL_STRING("Cascade", d->title);
    TEST_ASSERT_TRUE(d->implemented);
    TEST_ASSERT_TRUE(sizeof(p08_state) <= CADV_P08_STATE_SIZE);
    start(0);
    TEST_ASSERT_EQUAL_DOUBLE(P08_ACT_TRAM, query("act"));
    TEST_ASSERT_EQUAL_DOUBLE(3, query("scientists"));
    TEST_ASSERT_EQUAL_DOUBLE(0, query("enemies"));
    TEST_ASSERT_EQUAL_STRING("OFFICER BRANDT", cadv_p08_npc_name(P08_NPC_BRANDT));
    TEST_ASSERT_EQUAL_STRING("?", cadv_p08_npc_name(9));
    TEST_ASSERT_EQUAL_STRING("CROSSBOW", cadv_p08_weapons()[P08_W_CROSSBOW].name);
    TEST_ASSERT_TRUE(cadv_nav_walkable(&st->nav, 25, 10));
    TEST_ASSERT_FALSE(cadv_nav_walkable(&st->nav, 31, 10)); /* the chamber door is shut */
}

static void test_portal_08_cascade__no_cutscene_ever_takes_control(void)
{
    float yaw;
    start(0);
    /* during the monorail ride the player still looks and walks around the car */
    yaw = st->body.yaw;
    in.look_x = 0.05f;
    in.move_x = 1.0f;
    tick(30);
    in.look_x = 0.0f;
    in.move_x = 0.0f;
    TEST_ASSERT_TRUE(st->body.yaw != yaw);
    TEST_ASSERT_TRUE(st->tram_moving);
    tick(60 * 10);
    TEST_ASSERT_FALSE(st->tram_moving);
    TEST_ASSERT_EQUAL_DOUBLE(P08_ACT_LOBBY, query("act"));
    /* the lobby: the chamber needs the suit */
    stand(28, 7, 0.0f);
    tick(2);
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "Suits"));
    stand(29, 7, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(25, query("suit"));
    TEST_ASSERT_TRUE(cadv_nav_walkable(&st->nav, 31, 10));
    /* the friend's door sign and the maintenance bot */
    stand(30, 11, 0.0f);
    tick(1);
    TEST_ASSERT_TRUE(st->marks_found & 1u);
    cadv_p08_teleport(st, st->npc[P08_NPC_C6P].f.pos.x, 0, st->npc[P08_NPC_C6P].f.pos.z + 1.0f, 0.0f);
    tick(1);
    TEST_ASSERT_TRUE(st->marks_found & 2u);
    /* push the cart: the cascade follows while the player keeps moving */
    cadv_p08_teleport(st, st->cart_x - 1.5f, 0, 22.0f, -0.5f * CADV_PI);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(P08_ACT_TEST, query("act"));
    in.move_y = -1.0f;
    tick(60 * 8);
    in.move_y = 0.0f;
    TEST_ASSERT_EQUAL_DOUBLE(1, query("cascade"));
    TEST_ASSERT_EQUAL_DOUBLE(P08_ACT_RUINS, query("act"));
    TEST_ASSERT_TRUE(query("enemies") >= 4);
    TEST_ASSERT_TRUE(cadv_nav_walkable(&st->nav, 39, 10)); /* the breach */
}

static void test_ruins_guard_power_and_radiology(void)
{
    int i;
    start(1);
    clear_enemies();
    TEST_ASSERT_EQUAL_DOUBLE(P08_ACT_RUINS, query("act"));
    /* Brandt joins when spoken to, but only when looked at */
    cadv_p08_teleport(st, st->npc[P08_NPC_BRANDT].f.pos.x + 2.0f, 0, st->npc[P08_NPC_BRANDT].f.pos.z, 0.0f);
    st->body.yaw = -0.5f * CADV_PI; /* facing away */
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("following"));
    face(st->npc[P08_NPC_BRANDT].f.pos);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("following"));
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "sidearm"));
    press(CADV_ACT_USE); /* wait here */
    TEST_ASSERT_EQUAL_DOUBLE(0, query("following"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "WAIT"));
    /* the generator needs two cells */
    stand(46, 6, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "2 POWER CELLS"));
    /* radiology: the nameplate lights the scanner and shows the hidden ones */
    stand(42, 5, 0.0f);
    tick(1);
    TEST_ASSERT_TRUE(st->marks_found & 4u);
    for (i = 0; i < P08_ENEMIES; ++i) {
        TEST_ASSERT_FALSE(st->e[i].hidden_until_scan);
    }
    clear_enemies();
    cadv_p08_teleport(st, st->pk[13].pos.x, 0, st->pk[13].pos.z, 0.0f);
    tick(1);
    cadv_p08_teleport(st, st->pk[14].pos.x, 0, st->pk[14].pos.z, 0.0f);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("cells"));
    stand(46, 6, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("generator"));
    TEST_ASSERT_TRUE(cadv_nav_walkable(&st->nav, 49, 10));
    /* the crate slides when walked into */
    {
        float x = st->crate_x;
        cadv_p08_teleport(st, st->crate_x - 1.5f, 0, st->crate_z, -0.5f * CADV_PI);
        in.move_y = 1.0f;
        tick(40);
        in.move_y = 0.0f;
        TEST_ASSERT_TRUE(st->crate_x > x + 0.2f);
        cadv_p08_teleport(st, st->crate_x, 0, st->crate_z - 1.5f, CADV_PI);
        in.move_y = 1.0f;
        tick(40);
        in.move_y = 0.0f;
    }
}

static void test_weapons_and_pickups(void)
{
    int e;
    start(2);
    clear_enemies();
    TEST_ASSERT_TRUE(st->owned[P08_W_SHOTGUN]);
    press(CADV_ACT_WEAPON_3);
    TEST_ASSERT_EQUAL_DOUBLE(P08_W_SHOTGUN, query("weapon"));
    press(CADV_ACT_WEAPON_4); /* not owned yet */
    TEST_ASSERT_EQUAL_DOUBLE(P08_W_SHOTGUN, query("weapon"));
    press(CADV_ACT_NEXT_WEAPON);
    TEST_ASSERT_EQUAL_DOUBLE(P08_W_AXE, query("weapon"));
    press(CADV_ACT_PREV_WEAPON);
    TEST_ASSERT_EQUAL_DOUBLE(P08_W_SHOTGUN, query("weapon"));
    /* a soldier in front takes a shotgun blast and an axe */
    stand(55, 10, 0.0f);
    st->body.yaw = -0.5f * CADV_PI;
    e = 0;
    st->e[e].active = true;
    st->e[e].kind = P08_E_SOLDIER;
    st->e[e].hp = 70;
    st->e[e].pos = cadv_v3(st->body.pos.x + 2.5f, 0, st->body.pos.z);
    st->e[e].cd = 99.0f;
    st->e[e].think = 99.0f;
    face(cadv_v3(st->e[e].pos.x, 1.0f, st->e[e].pos.z));
    press(CADV_ACT_FIRE);
    TEST_ASSERT_TRUE(st->e[e].hp < 70);
    tick(60);
    press(CADV_ACT_WEAPON_1);
    st->e[e].pos = cadv_v3(st->body.pos.x + 1.2f, 0, st->body.pos.z);
    face(cadv_v3(st->e[e].pos.x, 1.0f, st->e[e].pos.z));
    st->e[e].hp = 20;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(0, cadv_p08_enemies_alive(st));
    press(CADV_ACT_FIRE); /* a swing at the air */
    /* crossbow and grenades from the pickups */
    cadv_p08_teleport(st, st->pk[3].pos.x, 0, st->pk[3].pos.z, 0.0f);
    tick(1);
    TEST_ASSERT_TRUE(st->owned[P08_W_CROSSBOW]);
    press(CADV_ACT_WEAPON_4);
    in.held = CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
    tick(2);
    TEST_ASSERT_TRUE(st->zoom);
    in.held = 0;
    tick(2);
    cadv_p08_teleport(st, st->pk[4].pos.x, 0, st->pk[4].pos.z, 0.0f);
    tick(1);
    press(CADV_ACT_WEAPON_5);
    TEST_ASSERT_EQUAL_DOUBLE(P08_W_GRENADE, query("weapon"));
    st->body.pitch = 0.3f;
    TEST_ASSERT_EQUAL_INT(2, st->reserve[P08_W_GRENADE]);
    tick(60);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(1, st->reserve[P08_W_GRENADE]);
    tick(150);
    /* empty magazines reload from the reserve */
    press(CADV_ACT_WEAPON_2);
    st->mag[P08_W_PISTOL] = 1;
    st->body.pitch = -0.4f;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(17, st->mag[P08_W_PISTOL]);
    /* medkits and ammo */
    st->body.health = 50;
    cadv_p08_teleport(st, st->pk[11].pos.x, 0, st->pk[11].pos.z, 0.0f);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(75, query("player.health"));
    cadv_p08_teleport(st, st->pk[7].pos.x, 0, st->pk[7].pos.z, 0.0f);
    tick(1);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "AMMUNITION"));
}

static void test_portal_08_cascade__escort_to_the_scanner(void)
{
    int i;
    start(2);
    clear_enemies();
    st->squad_sent = 2; /* nobody shooting */
    /* the scanner refuses the technician */
    stand(61, 10, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "BRING A SCIENTIST"));
    /* the lab: all three follow */
    stand(53, 15, 0.0f);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("lab"));
    TEST_ASSERT_TRUE(query("following") >= 3);
    clear_enemies();
    /* lead them to the scanner */
    for (i = 0; i < 6; ++i) {
        stand(57 + i / 2, 12, 0.0f);
        tick(120);
        clear_enemies();
    }
    stand(61, 10, 0.0f);
    tick(240);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("scanner"));
    /* a scientist under fire can fall */
    st->npc[P08_NPC_OKAFOR].hp = 3;
    st->e[0].active = true;
    st->e[0].kind = P08_E_SKITTER;
    st->e[0].hp = 14;
    st->e[0].pos = st->npc[P08_NPC_OKAFOR].f.pos;
    st->e[0].target = P08_NPC_OKAFOR;
    st->e[0].cd = 0.0f;
    tick(30);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("scientists"));
}

static void test_soldiers_think_in_squads(void)
{
    int i;
    int states = 0;
    start(2);
    clear_enemies();
    st->t_blackice.fired = 0;
    st->t_blackice.inside = false;
    stand(51, 11, -0.5f * CADV_PI);
    tick(60);
    TEST_ASSERT_TRUE(query("enemies") >= 3); /* the Blackice Detail arrives */
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "BLACKICE"));
    st->body.health = 100000; /* watch them work */
    for (i = 0; i < 60 * 12; ++i) {
        int k;
        tick(1);
        for (k = 0; k < P08_ENEMIES; ++k) {
            if (st->e[k].active && st->e[k].kind == P08_E_SOLDIER) {
                states |= 1 << st->e[k].state;
            }
        }
    }
    TEST_ASSERT_TRUE(states & (1 << CADV_SQUAD_ENGAGE));
    TEST_ASSERT_TRUE(__builtin_popcount((unsigned)states) >= 2);
    TEST_ASSERT_TRUE(st->body.health < 100000);
}

static void test_suit_chargers(void)
{
    start(2);
    clear_enemies();
    st->squad_sent = 2;
    st->body.armor = 0;
    cadv_p08_teleport(st, st->charger_pos[0].x, 0, st->charger_pos[0].z + 0.8f, 0.0f);
    in.held = CADV_ACT_BIT(CADV_ACT_USE);
    tick(60 * 10);
    in.held = 0;
    TEST_ASSERT_TRUE(query("suit") >= 70.0);
    TEST_ASSERT_TRUE(st->chargers[0].reserve < 5.0f);
}

static void test_portal_08_cascade__reverse_the_beam(void)
{
    static const int k_breakers[3][2] = {{65, 5}, {77, 11}, {74, 17}};
    int i;
    start(3);
    clear_enemies();
    TEST_ASSERT_EQUAL_DOUBLE(P08_ACT_ARRAY, query("act"));
    /* the thing under the ice strikes where you stand */
    st->slam_cd = 1;
    tick(2);
    TEST_ASSERT_TRUE(st->slam.active);
    {
        int hp = st->body.health + st->body.armor;
        tick(80);
        TEST_ASSERT_TRUE(st->body.health + st->body.armor < hp);
    }
    for (i = 0; i < 3; ++i) {
        cadv_vec3 b = cadv_p08_cell(k_breakers[i][0], k_breakers[i][1]);
        cadv_p08_teleport(st, b.x, i == 2 ? 2.7f : 0.0f, b.z + 1.0f, 0.0f);
        tick(2);
        press(CADV_ACT_USE);
        clear_enemies();
    }
    TEST_ASSERT_EQUAL_DOUBLE(7, query("breakers"));
    TEST_ASSERT_TRUE(st->star_active);
    tick(120);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("star.taken"));
    tick(150);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_EQUAL_INT(3, cadv_session_result(&s).snowflakes);
}

static void test_losing_and_queries(void)
{
    const char *keys[] = {"player.x", "player.y", "player.z", "player.yaw", "player.health", "suit", "act", "weapon", "cells", "generator",
                          "lab", "scanner", "breakers", "scientists", "following", "enemies", "cascade", "star.taken", "outcome",
                          "marks", "ticks"};
    size_t i;
    double v;
    start(1);
    for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        TEST_ASSERT_TRUE(cadv_p08_query(st, keys[i], &v));
    }
    TEST_ASSERT_FALSE(cadv_p08_query(st, "bogus", &v));
    st->body.health = 1;
    st->body.armor = 0;
    st->e[0].active = true;
    st->e[0].kind = P08_E_SKITTER;
    st->e[0].hidden_until_scan = false;
    st->e[0].pos = st->body.pos;
    st->e[0].cd = 0.0f;
    tick(240);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
    cadv_session_end(&s);
    start(0);
    press(CADV_ACT_PAUSE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    tick(1);
}

static void play_bot(int max)
{
    int i;
    for (i = 0; i < max && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_input_frame bot;
        cadv_p08_bot(st, &bot);
        cadv_session_tick(&s, &bot);
    }
}

static void test_bot_finishes_on_every_difficulty(void)
{
    start_with(0, CADV_DIFF_DEFAULT);
    play_bot(60000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    TEST_ASSERT_EQUAL_UINT32(7u, cadv_session_result(&s).marks_found);
    cadv_session_end(&s);
    start_with(0, CADV_DIFF_STORY);
    play_bot(60000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    cadv_session_end(&s);
    start_with(1, CADV_DIFF_HARD);
    play_bot(60000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    cadv_session_end(&s);
    start_with(2, CADV_DIFF_HARD);
    play_bot(60000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_catalog__portal_08_cascade);
    RUN_TEST(test_portal_08_cascade__no_cutscene_ever_takes_control);
    RUN_TEST(test_ruins_guard_power_and_radiology);
    RUN_TEST(test_weapons_and_pickups);
    RUN_TEST(test_portal_08_cascade__escort_to_the_scanner);
    RUN_TEST(test_soldiers_think_in_squads);
    RUN_TEST(test_suit_chargers);
    RUN_TEST(test_portal_08_cascade__reverse_the_beam);
    RUN_TEST(test_losing_and_queries);
    RUN_TEST(test_bot_finishes_on_every_difficulty);
    return UNITY_END();
}
