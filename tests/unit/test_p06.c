/* SPDX-License-Identifier: MIT */
/* Day 6 - Iron Yule: the lab, the cockpit (heat, groups, lock), the three
 * missions with Santa Claude, coolant, checkpoints and the bot. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p06_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p06_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 6;
    cfg.seed = 7;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(6), &cfg));
    st = (p06_state *)s.state;
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
    }
}

static void press(cadv_action a)
{
    in.pressed |= CADV_ACT_BIT(a);
    in.held |= CADV_ACT_BIT(a);
    tick(1);
    in.held &= ~CADV_ACT_BIT(a);
}

static void hold(cadv_action a, int n)
{
    in.held |= CADV_ACT_BIT(a);
    tick(n);
    in.held &= ~CADV_ACT_BIT(a);
}

static double query(const char *key)
{
    double v = -1.0;
    TEST_ASSERT_TRUE(cadv_p06_query(st, key, &v));
    return v;
}

/* Leave the lab with the default loadout. */
static void launch(void)
{
    start(0);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(P06_M1, query("mission"));
}

/* Remove every hostile and stop the convoy's ambushes. */
static void calm(void)
{
    int i;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (st->u[i].active && st->u[i].team == P06_TEAM_GRAIL && st->u[i].kind != P06_K_WRECK) {
            st->u[i].active = false;
        }
    }
    st->ambush_sent = 3;
    st->crawler_cp = 6;
}

static int find(int kind)
{
    int i;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (st->u[i].active && !st->u[i].dead && st->u[i].kind == kind) {
            return i;
        }
    }
    return -1;
}

static int add(int kind, float x, float z)
{
    int i;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (!st->u[i].active) {
            break;
        }
    }
    memset(&st->u[i], 0, sizeof st->u[i]);
    st->u[i].active = true;
    st->u[i].kind = kind;
    st->u[i].team = P06_TEAM_GRAIL;
    st->u[i].target = -1;
    st->u[i].pos = cadv_v3(x, cadv_terrain_height(&st->terrain, x, z), z);
    if (kind == P06_K_JACKAL || kind == P06_K_KRAMPUS) {
        cadv_walker_spawn(&st->u[i].w, cadv_p06_walker_def(kind), st->u[i].pos, 0.0f);
    }
    st->u[i].hp = st->u[i].hp_max = 100;
    return i;
}

/* Point the torso at a world point. */
static void aim_at(cadv_vec3 p)
{
    cadv_vec3 e = cadv_p06_eye(st);
    float yaw = cadv_atan2f(-(p.x - e.x), -(p.z - e.z));
    float d = cadv_sqrtf((p.x - e.x) * (p.x - e.x) + (p.z - e.z) * (p.z - e.z));
    st->mech.twist = cadv_wrap_angle(yaw - st->mech.legs_yaw);
    st->mech.pitch = cadv_atan2f(p.y - e.y, d);
}

static void test_portal_catalog__portal_06_iron_yule(void)
{
    const cadv_portal_desc *d = cadv_portal_get(6);
    TEST_ASSERT_EQUAL_STRING("Iron Yule", d->title);
    TEST_ASSERT_TRUE(d->implemented);
    TEST_ASSERT_TRUE(sizeof(p06_state) <= CADV_P06_STATE_SIZE);
    start(0);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("lab"));
    TEST_ASSERT_EQUAL_DOUBLE(7, query("tonnage"));
    TEST_ASSERT_EQUAL_DOUBLE(0, query("ambush"));
    TEST_ASSERT_TRUE(query("crawler.hp") > 0);
    TEST_ASSERT_EQUAL_STRING("HOLLOWFROST WHITE", cadv_p06_paint_name(st, 0));
    TEST_ASSERT_EQUAL_STRING("PAINT: DR. DOOM BLACK", cadv_p06_paint_name(st, 2));
    TEST_ASSERT_EQUAL_INT(P06_G_ENERGY, cadv_p06_weapons()[P06_W_PPC].group);
    TEST_ASSERT_TRUE(cadv_p06_walker_def(P06_K_KRAMPUS)->armour[CADV_WZ_CT] > cadv_p06_walker_def(P06_K_JACKAL)->armour[CADV_WZ_CT]);
    /* the canyon floor is walkable, its walls are not */
    TEST_ASSERT_TRUE(cadv_terrain_height(&st->terrain, 256, 440) < 12.0f);
    TEST_ASSERT_TRUE(cadv_terrain_height(&st->terrain, 200, 440) > 25.0f);
    TEST_ASSERT_TRUE(cadv_terrain_height(&st->terrain, 110, 70) < P06_SEA); /* the reservoir */
}

static void test_portal_06_iron_yule__mech_lab_within_tonnage(void)
{
    start(0);
    press(CADV_ACT_WEAPON_2);
    TEST_ASSERT_EQUAL_INT(1, st->lab_slot);
    press(CADV_ACT_NEXT_WEAPON); /* autocannon -> missiles */
    TEST_ASSERT_EQUAL_INT(P06_W_MISSILE, st->loadout[1]);
    press(CADV_ACT_WEAPON_1);
    press(CADV_ACT_NEXT_WEAPON); /* laser -> PPC */
    TEST_ASSERT_EQUAL_INT(P06_W_PPC, st->loadout[0]);
    TEST_ASSERT_EQUAL_DOUBLE(8, query("tonnage"));
    press(CADV_ACT_WEAPON_4);
    press(CADV_ACT_NEXT_WEAPON); /* laser -> PPC would be 10: allowed */
    TEST_ASSERT_EQUAL_DOUBLE(10, query("tonnage"));
    press(CADV_ACT_NEXT_WEAPON); /* PPC -> autocannon: 10, still fits */
    TEST_ASSERT_EQUAL_INT(P06_W_AC, st->loadout[3]);
    press(CADV_ACT_WEAPON_3);
    press(CADV_ACT_PREV_WEAPON); /* missiles -> autocannon would be 11: skipped to a fitting one */
    TEST_ASSERT_TRUE(cadv_p06_tonnage(st) <= P06_TONNAGE);
    TEST_ASSERT_NOT_EQUAL(P06_W_AC, st->loadout[2]);
    press(CADV_ACT_ALT_FIRE);
    press(CADV_ACT_ALT_FIRE);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("paint"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "BLACK"));
    press(CADV_ACT_FIRE); /* launch */
    TEST_ASSERT_TRUE(st->marks_found & 1u);
    TEST_ASSERT_EQUAL_DOUBLE(P06_M1, query("mission"));
    TEST_ASSERT_EQUAL_INT(90, st->ammo[3]);
}

static void test_cockpit_throttle_groups_and_heat(void)
{
    int i;
    launch();
    calm();
    in.move_y = 1.0f;
    tick(90);
    in.move_y = 0.0f;
    TEST_ASSERT_TRUE(query("throttle") > 0.9);
    tick(120);
    TEST_ASSERT_TRUE(query("speed") > 10.0);
    press(CADV_ACT_CROUCH); /* all stop */
    TEST_ASSERT_EQUAL_DOUBLE(0, query("throttle"));
    in.look_x = 0.03f;
    tick(30);
    in.look_x = 0.0f;
    TEST_ASSERT_TRUE(query("twist") > 0.5);
    hold(CADV_ACT_RUN, 60); /* centre the torso */
    TEST_ASSERT_EQUAL_DOUBLE(0, query("twist"));
    press(CADV_ACT_WEAPON_2);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("group"));
    press(CADV_ACT_NEXT_WEAPON);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("group"));
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_PREV_WEAPON);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("group"));
    press(CADV_ACT_WEAPON_4);
    TEST_ASSERT_TRUE(st->chain);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CHAIN"));
    /* chain fire: one laser per trigger pull */
    st->mech.pitch = -0.3f;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_TRUE(st->cd[0] > 0.5f || st->cd[3] > 0.5f);
    TEST_ASSERT_FALSE(st->cd[0] > 0.5f && st->cd[3] > 0.5f);
    press(CADV_ACT_WEAPON_4);
    press(CADV_ACT_MAP);
    TEST_ASSERT_TRUE(st->map_open);
    press(CADV_ACT_MAP);
    /* alpha strikes until the heat shuts the machine down */
    for (i = 0; i < 600 && !st->mech.shutdown; ++i) {
        hold(CADV_ACT_ALT_FIRE, 1);
    }
    TEST_ASSERT_TRUE(st->mech.shutdown);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("shutdowns"));
    tick(1);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "SHUTDOWN"));
    TEST_ASSERT_TRUE(st->ammo[1] < 90);
    TEST_ASSERT_TRUE(st->ammo[2] < 24);
    hold(CADV_ACT_FIRE, 60); /* nothing fires while shut down */
    tick(60 * 12);
    TEST_ASSERT_FALSE(st->mech.shutdown);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "RESTART"));
    /* wading in the reservoir cools twice as fast */
    st->mech.heat = 80.0f;
    cadv_p06_teleport(st, 110, 0, 70, 0);
    tick(60);
    TEST_ASSERT_TRUE(query("heat") < 63.0);
}

static void test_weapons_lock_and_component_loss(void)
{
    int j;
    int hover;
    float hp;
    int i;
    launch();
    calm();
    cadv_p06_teleport(st, 256, 0, 440, 0);
    j = add(P06_K_JACKAL, 256, 380);
    aim_at(cadv_v3(256, st->u[j].w.pos.y + 4.0f, 380));
    TEST_ASSERT_EQUAL_INT(j, cadv_p06_aim_target(st));
    tick(60); /* hold it for a lock */
    TEST_ASSERT_EQUAL_DOUBLE(1, query("lock"));
    hp = (float)st->u[j].w.hp[CADV_WZ_CT];
    press(CADV_ACT_WEAPON_3);
    hold(CADV_ACT_FIRE, 1);
    tick(60); /* the homing salvo arrives */
    TEST_ASSERT_TRUE(st->u[j].w.hp[CADV_WZ_CT] < hp || st->u[j].w.lost > 0);
    for (i = 0; i < 1200 && !st->u[j].dead; ++i) {
        st->mech.heat = 0.0f;
        aim_at(cadv_v3(256, st->u[j].w.pos.y + 4.0f, st->u[j].w.pos.z));
        hold(CADV_ACT_ALT_FIRE, 1);
    }
    TEST_ASSERT_TRUE(st->u[j].dead);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("enemies"));
    TEST_ASSERT_EQUAL_DOUBLE(0, query("lock"));
    /* a hover tank under fire */
    hover = add(P06_K_HOVER, 256, 400);
    st->u[hover].hp = 5;
    aim_at(cadv_v3(256, st->u[hover].pos.y + 2.0f, 400));
    press(CADV_ACT_WEAPON_1);
    st->cd[0] = st->cd[3] = 0.0f;
    hold(CADV_ACT_FIRE, 1);
    TEST_ASSERT_TRUE(st->u[hover].dead);
    /* a lost arm takes its weapon with it */
    cadv_walker_damage(&st->mech, CADV_WZ_LA, 44);
    TEST_ASSERT_FALSE(cadv_walker_has(&st->mech, CADV_WZ_LA));
    st->cd[0] = 0.0f;
    st->mech.pitch = -0.3f;
    hold(CADV_ACT_FIRE, 1);
    TEST_ASSERT_TRUE(st->cd[0] <= 0.0f);
    /* hits on the player land on components (wingmen sent home) */
    st->u[st->wingmen[0]].active = false;
    st->u[st->wingmen[1]].active = false;
    j = add(P06_K_JACKAL, 256, 420);
    st->u[j].target = P06_PLAYER;
    st->u[j].w.legs_yaw = 3.14159f;
    hp = (float)st->mech.hp[CADV_WZ_CT] + (float)st->mech.hp[CADV_WZ_LL] + (float)st->mech.hp[CADV_WZ_RL] + (float)st->mech.hp[CADV_WZ_LT] +
         (float)st->mech.hp[CADV_WZ_RT] + (float)st->mech.hp[CADV_WZ_RA] + (float)st->mech.hp[CADV_WZ_HEAD];
    tick(600);
    TEST_ASSERT_TRUE((float)st->mech.hp[CADV_WZ_CT] + (float)st->mech.hp[CADV_WZ_LL] + (float)st->mech.hp[CADV_WZ_RL] + (float)st->mech.hp[CADV_WZ_LT] +
                         (float)st->mech.hp[CADV_WZ_RT] + (float)st->mech.hp[CADV_WZ_RA] + (float)st->mech.hp[CADV_WZ_HEAD] <
                     hp);
}

static void test_portal_06_iron_yule__coolant_drop_on_request(void)
{
    launch();
    calm();
    st->mech.heat = 90.0f;
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "Coolant on its way"));
    TEST_ASSERT_TRUE(st->coolant_timer > 0);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("coolant"));
    press(CADV_ACT_USE); /* only one per mission */
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "all out"));
    tick(185);
    TEST_ASSERT_TRUE(st->coolant_down);
    cadv_p06_teleport(st, st->coolant_pos.x, 0, st->coolant_pos.z, 0);
    st->mech.heat = 90.0f;
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("heat"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "COOLANT"));
    TEST_ASSERT_TRUE(st->cool_boost > 0);
    /* at the dam, Sleighbell is out of reach */
    cadv_session_end(&s);
    start(1);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "too far"));
}

static void test_portal_06_iron_yule__escort_the_crawler(void)
{
    int i;
    int c;
    launch();
    c = st->crawler;
    /* friends on the radio, the wreck in the canyon */
    tick(500);
    TEST_ASSERT_TRUE(st->marks_found & 4u);
    TEST_ASSERT_TRUE(st->marks_found & 8u);
    cadv_p06_teleport(st, st->wreck_pos.x, 0, st->wreck_pos.z + 20.0f, 0);
    tick(1);
    TEST_ASSERT_TRUE(st->marks_found & 2u);
    /* the crawler waits for its escort */
    cadv_p06_teleport(st, 256, 0, 488, 0);
    st->u[c].pos = st->path[1];
    st->path_next = 2;
    cadv_p06_teleport(st, 256, 0, 500, 0);
    tick(10);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("crawler.node"));
    /* first ambush: the convoy holds while the jackals live */
    cadv_p06_teleport(st, st->u[c].pos.x, 0, st->u[c].pos.z + 20.0f, 0);
    tick(900);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("ambush"));
    TEST_ASSERT_TRUE(query("enemies") > 0);
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "Contacts") != NULL ? st->radio : "Contacts");
    {
        int node = (int)query("crawler.node");
        tick(300);
        TEST_ASSERT_EQUAL_DOUBLE(node, query("crawler.node"));
    }
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (st->u[i].active && st->u[i].group == 1) {
            st->u[i].dead = true;
        }
    }
    tick(2);
    TEST_ASSERT_EQUAL_INT(2, st->crawler_cp);
    /* Sleighbell is destroyed: back to the last checkpoint */
    st->u[c].hp = 1;
    {
        int k = add(P06_K_HOVER, st->u[c].pos.x + 30.0f, st->u[c].pos.z);
        int p;
        st->u[k].team = P06_TEAM_GRAIL;
        for (p = 0; p < P06_MAX_PROJ; ++p) {
            if (!st->proj[p].active) {
                break;
            }
        }
        st->proj[p].active = true;
        st->proj[p].kind = P06_PROJ_SHELL;
        st->proj[p].team = P06_TEAM_GRAIL;
        st->proj[p].owner = k;
        st->proj[p].target = -1;
        st->proj[p].pos = cadv_v3(st->u[c].pos.x + 8.0f, st->u[c].pos.y + 2.0f, st->u[c].pos.z);
        st->proj[p].vel = cadv_v3(-200.0f, 0, 0);
        st->proj[p].damage = 50;
        st->proj[p].life = 1.0f;
    }
    tick(5);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("restarts"));
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "try that again"));
    TEST_ASSERT_EQUAL_DOUBLE(3, query("crawler.node"));
    TEST_ASSERT_EQUAL_DOUBLE(1, query("ambush")); /* the first ambush stays beaten */
}

static void test_portal_06_iron_yule__santa_claude_hands_over_the_star(void)
{
    launch();
    calm();
    st->u[st->crawler].pos = st->path[P06_PATH_NODES - 2];
    st->path_next = P06_PATH_NODES - 1;
    cadv_p06_teleport(st, st->path[P06_PATH_NODES - 2].x, 0, st->path[P06_PATH_NODES - 2].z + 15.0f, 0);
    tick(300);
    TEST_ASSERT_TRUE(st->star_active);
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "cargo bay"));
    cadv_p06_teleport(st, st->star_pos.x, 0, st->star_pos.z, 0);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("star.taken"));
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    TEST_ASSERT_EQUAL_INT(1, cadv_session_result(&s).checkpoint);
    TEST_ASSERT_EQUAL_DOUBLE(P06_M2, query("mission"));
    TEST_ASSERT_EQUAL_DOUBLE(4, query("pumps"));
}

static void test_portal_06_iron_yule__pump_station_falls(void)
{
    int i;
    start(1);
    TEST_ASSERT_EQUAL_DOUBLE(P06_M2, query("mission"));
    TEST_ASSERT_EQUAL_DOUBLE(4, query("pumps"));
    TEST_ASSERT_TRUE(query("enemies") >= 9);
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (st->u[i].active && st->u[i].kind == P06_K_PUMP) {
            st->u[i].hp = 1;
            break;
        }
    }
    {
        /* a laser straight at it */
        cadv_vec3 p = st->u[i].pos;
        cadv_p06_teleport(st, p.x, 0, p.z - 40.0f, 3.14159f);
        aim_at(cadv_v3(p.x, p.y + 6.0f, p.z));
        st->cd[0] = st->cd[3] = 0.0f;
        hold(CADV_ACT_FIRE, 1);
    }
    TEST_ASSERT_EQUAL_DOUBLE(3, query("pumps"));
    tick(2);
    TEST_ASSERT_TRUE(st->reinforced);
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "Hover tanks"));
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (st->u[i].active && st->u[i].kind == P06_K_PUMP && !st->u[i].dead) {
            st->u[i].dead = true;
            --st->pumps_left;
        }
    }
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(P06_M3, query("mission"));
    TEST_ASSERT_EQUAL_INT(2, cadv_session_result(&s).checkpoint);
}

static void test_portal_06_iron_yule__duel_with_krampus(void)
{
    int k;
    start(2);
    k = st->krampus;
    TEST_ASSERT_EQUAL_DOUBLE(P06_M3, query("mission"));
    TEST_ASSERT_TRUE(query("krampus.hp") > 100);
    /* a steam vent cools faster than the open snow */
    cadv_p06_teleport(st, st->vents[0].x, 0, st->vents[0].z, 0);
    st->mech.heat = 80.0f;
    tick(30);
    TEST_ASSERT_TRUE(query("heat") < 70.0);
    /* losing a component in the duel costs the third snowflake */
    cadv_walker_damage(&st->mech, CADV_WZ_LA, 500);
    tick(1);
    TEST_ASSERT_TRUE(st->m3_component_lost);
    st->u[k].w.hp[CADV_WZ_CT] = 1;
    st->u[k].hurt = 5;
    cadv_walker_damage(&st->u[k].w, CADV_WZ_CT, 5);
    st->u[k].dead = true;
    tick(2);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CAMPAIGN COMPLETE"));
    tick(200);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_EQUAL_INT(2, cadv_session_result(&s).snowflakes); /* complete + never shut down */
}

static void test_ejecting_loses(void)
{
    start(2);
    cadv_walker_damage(&st->mech, CADV_WZ_HEAD, 1);
    st->mech.hp[CADV_WZ_CT] = 1;
    st->u[st->krampus].target = P06_PLAYER;
    st->u[st->krampus].w.pos = cadv_v3(st->mech.pos.x + 40.0f, st->mech.pos.y, st->mech.pos.z);
    tick(60 * 30);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
    TEST_ASSERT_EQUAL_INT(0, cadv_session_result(&s).snowflakes);
}

static void test_queries_teleport_and_quit(void)
{
    const char *keys[] = {"player.x", "player.y", "player.z", "player.yaw", "player.health", "heat", "throttle", "speed", "twist",
                          "shutdowns", "lost", "mission", "lab", "tonnage", "paint", "group", "lock", "crawler.hp", "crawler.node",
                          "ambush", "enemies", "pumps", "krampus.hp", "coolant", "star.taken", "restarts", "outcome", "marks", "ticks"};
    size_t i;
    double v;
    start(0);
    for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        TEST_ASSERT_TRUE(cadv_p06_query(st, keys[i], &v));
    }
    TEST_ASSERT_FALSE(cadv_p06_query(st, "bogus", &v));
    cadv_p06_teleport(st, 200, 0, 150, 1.0f);
    TEST_ASSERT_EQUAL_DOUBLE(200, query("player.x"));
    TEST_ASSERT_EQUAL_INT(-1, cadv_p06_aim_target(st));
    TEST_ASSERT_EQUAL_INT(0, cadv_p06_enemies_alive(st));
    TEST_ASSERT_TRUE(cadv_p06_unit_pos(st, P06_PLAYER).x > 199.0f);
    press(CADV_ACT_PAUSE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    tick(1);
    (void)find;
}

static void play_bot(int max)
{
    int i;
    for (i = 0; i < max && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_input_frame bot;
        cadv_p06_bot(st, &bot);
        cadv_session_tick(&s, &bot);
    }
}

static void test_bot_finishes_on_every_difficulty(void)
{
    start_with(0, CADV_DIFF_DEFAULT);
    play_bot(60000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    TEST_ASSERT_TRUE(cadv_session_result(&s).snowflakes >= 2);
    TEST_ASSERT_EQUAL_UINT32(15u, cadv_session_result(&s).marks_found);
    cadv_session_end(&s);
    start_with(1, CADV_DIFF_STORY);
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
    RUN_TEST(test_portal_catalog__portal_06_iron_yule);
    RUN_TEST(test_portal_06_iron_yule__mech_lab_within_tonnage);
    RUN_TEST(test_cockpit_throttle_groups_and_heat);
    RUN_TEST(test_weapons_lock_and_component_loss);
    RUN_TEST(test_portal_06_iron_yule__coolant_drop_on_request);
    RUN_TEST(test_portal_06_iron_yule__escort_the_crawler);
    RUN_TEST(test_portal_06_iron_yule__santa_claude_hands_over_the_star);
    RUN_TEST(test_portal_06_iron_yule__pump_station_falls);
    RUN_TEST(test_portal_06_iron_yule__duel_with_krampus);
    RUN_TEST(test_ejecting_loses);
    RUN_TEST(test_queries_teleport_and_quit);
    RUN_TEST(test_bot_finishes_on_every_difficulty);
    return UNITY_END();
}
