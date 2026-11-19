/* SPDX-License-Identifier: MIT */
/* Day 5 - Hearthguard: the Captain and the stave, raids, the thaw, the ferry,
 * the Longhall and the star. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p05_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p05_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 5;
    cfg.seed = 7;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(5), &cfg));
    st = (p05_state *)s.state;
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

static double query(const char *key)
{
    double v = -1.0;
    TEST_ASSERT_TRUE(cadv_p05_query(st, key, &v));
    return v;
}

static float cell(int c) { return ((float)c + 0.5f) * P05_CELL; }

static void aim(float x, float y, float z)
{
    cadv_vec3 e = cadv_p05_eye(st);
    cadv_vec3 to = cadv_v3_sub(cadv_v3(x, y, z), e);
    st->yaw = cadv_atan2f(-to.x, -to.z);
    st->pitch = cadv_atan2f(to.y, cadv_sqrtf(to.x * to.x + to.z * to.z));
}

static void aim_cell(int cx, int cz) { aim(cell(cx), 0.0f, cell(cz)); }

static void stand(int cx, int cz) { cadv_p05_teleport(st, cell(cx), 0, cell(cz), 0); }

/* No raids, no troll army: a quiet valley for the rules under test. */
static void calm(void)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (st->rts.u[i].alive && st->rts.u[i].owner == P05_TROLLS) {
            st->rts.u[i].alive = false;
        }
    }
    st->raid_timer = 1 << 30;
    st->ai.ntrain = 0;
    st->ai.nplan = 0;
}

static void rich(void)
{
    st->rts.side[P05_GNOMES].credits = 5000;
    st->rts.side[P05_GNOMES].lumber = 5000;
    st->rts.side[P05_GNOMES].tallow = 5000;
}

static void pick_stave(int item)
{
    st->stave_item = item;
}

static int first_unit(int def)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (st->rts.u[i].alive && st->rts.u[i].def == def) {
            return i;
        }
    }
    return -1;
}

static int add_unit(int side, int def, int cx, int cz) { return cadv_rts_add_unit(&st->rts, side, def, cell(cx), cell(cz)); }

static void test_portal_catalog__portal_05_hearthguard(void)
{
    const cadv_portal_desc *d = cadv_portal_get(5);
    TEST_ASSERT_EQUAL_STRING("Hearthguard", d->title);
    TEST_ASSERT_TRUE(d->implemented);
    TEST_ASSERT_EQUAL_INT(4, d->mark_count);
    TEST_ASSERT_TRUE(sizeof(p05_state) <= CADV_P05_STATE_SIZE);
    start(0);
    TEST_ASSERT_EQUAL_DOUBLE(3, query("workers"));
    TEST_ASSERT_EQUAL_DOUBLE(1, query("buildings"));
    TEST_ASSERT_EQUAL_DOUBLE(450, query("gold"));
    TEST_ASSERT_EQUAL_DOUBLE(250, query("lumber"));
    TEST_ASSERT_EQUAL_DOUBLE(0, query("tallow"));
    TEST_ASSERT_EQUAL_DOUBLE(3, query("supply"));
    TEST_ASSERT_EQUAL_DOUBLE(5, query("supply.cap"));
    TEST_ASSERT_EQUAL_DOUBLE(0, query("raid"));
    TEST_ASSERT_EQUAL_DOUBLE(0, query("thawed"));
    TEST_ASSERT_EQUAL_DOUBLE(3000, query("longhall.hp"));
    TEST_ASSERT_EQUAL_STRING("Farm", cadv_p05_stave_name(0));
    TEST_ASSERT_EQUAL_STRING("Keep Upgrade", cadv_p05_stave_name(P05_STAVE_KEEP));
    cadv_session_end(&s);
    start_with(0, CADV_DIFF_STORY);
    TEST_ASSERT_EQUAL_DOUBLE(700, query("gold"));
    TEST_ASSERT_TRUE(cadv_p05_raid_ticks(st, 0) > 60 * 210);
}

static void test_portal_05_hearthguard__three_resource_costs_on_the_stave(void)
{
    int g;
    int l;
    int t;
    cadv_p05_stave_cost(0, &g, &l, &t); /* farm */
    TEST_ASSERT_EQUAL_INT(80, g);
    TEST_ASSERT_EQUAL_INT(40, l);
    TEST_ASSERT_EQUAL_INT(0, t);
    cadv_p05_stave_cost(P05_STAVE_STRUCTURES + 5, &g, &l, &t); /* ferry */
    TEST_ASSERT_EQUAL_INT(100, g);
    TEST_ASSERT_EQUAL_INT(80, l);
    TEST_ASSERT_EQUAL_INT(60, t);
    cadv_p05_stave_cost(P05_STAVE_KEEP, &g, &l, &t);
    TEST_ASSERT_EQUAL_INT(300, g);
    TEST_ASSERT_EQUAL_INT(200, l);
    TEST_ASSERT_EQUAL_INT(0, t);
}

static void test_stave_cycles_and_map(void)
{
    start(0);
    press(CADV_ACT_NEXT_WEAPON);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("stave"));
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_PREV_WEAPON);
    TEST_ASSERT_EQUAL_DOUBLE(P05_STAVE_ITEMS - 1, query("stave"));
    press(CADV_ACT_RADIAL);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("stave"));
    press(CADV_ACT_MAP);
    TEST_ASSERT_TRUE(st->map_open);
    press(CADV_ACT_MAP);
    TEST_ASSERT_FALSE(st->map_open);
}

static void test_build_and_place_a_farm(void)
{
    start(0);
    calm();
    pick_stave(0);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "BUILDING"));
    tick(cadv_p05_buildings()[P05_B_FARM].build_ticks + 5);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("placing"));
    /* nowhere to put it: the sky, then the frozen lake */
    st->pitch = 0.5f;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "POINT THE STAVE"));
    stand(31, 36);
    aim_cell(31, 30);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "ICE"));
    /* in the forest, far from the village */
    st->marks_found |= 8u; /* the tavern sign is not the point here */
    stand(22, 56);
    aim_cell(15, 56);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CANNOT BUILD"));
    stand(44, 46);
    aim_cell(44, 41);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(st->message, "TOO FAR"), st->message);
    /* on top of the hall */
    stand(31, 56);
    aim_cell(31, 51);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CANNOT BUILD"));
    /* pause placing with ALT-FIRE, resume with E */
    press(CADV_ACT_ALT_FIRE);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("placing"));
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("placing"));
    stand(28, 58);
    aim_cell(28, 54);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(st->message, "BUILT"), st->message);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("buildings"));
    TEST_ASSERT_EQUAL_DOUBLE(10, query("supply.cap"));
    /* broke: the next farm is refused */
    st->rts.side[P05_GNOMES].credits = 0;
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "NOT ENOUGH"));
}

static void test_shipyard_needs_the_shore(void)
{
    start(0);
    calm();
    rich();
    pick_stave(4);
    press(CADV_ACT_USE);
    tick(cadv_p05_buildings()[P05_B_SHIPYARD].build_ticks + 5);
    stand(31, 48);
    aim_cell(27, 44);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "SHORE"));
    stand(37, 40);
    aim_cell(37, 35);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "BUILT"));
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_buildings(&st->rts, P05_GNOMES, P05_B_SHIPYARD));
    /* now a ferry can be trained (it needs tallow) */
    pick_stave(P05_STAVE_STRUCTURES + 5);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "TRAINING"));
}

static void test_training_refusals_and_the_keep(void)
{
    int i;
    start(0);
    calm();
    pick_stave(P05_STAVE_STRUCTURES + 1); /* footman: no barracks yet */
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "BARRACKS"));
    pick_stave(P05_STAVE_STRUCTURES + 4);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "WORKSHOP"));
    pick_stave(P05_STAVE_STRUCTURES + 5);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "SHIPYARD"));
    pick_stave(P05_STAVE_STRUCTURES + 3);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "KEEP"));
    /* food: two workers fit, the third does not */
    rich();
    pick_stave(P05_STAVE_STRUCTURES);
    press(CADV_ACT_USE);
    press(CADV_ACT_USE);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "FOOD"));
    for (i = 0; i < 5; ++i) {
        cadv_rts_add_building(&st->rts, P05_GNOMES, P05_B_FARM, 36 + (i % 2) * 2, 52 + (i / 2) * 2);
    }
    for (i = 0; i < 8; ++i) {
        press(CADV_ACT_USE);
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(st->message, "QUEUE FULL"), st->message);
    /* the keep */
    pick_stave(P05_STAVE_KEEP);
    st->rts.side[P05_GNOMES].credits = 10;
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "NOT ENOUGH"));
    rich();
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "RAISING"));
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "GOING UP"));
    tick(905);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("keep"));
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "ALREADY"));
    TEST_ASSERT_EQUAL_DOUBLE(33, query("supply.cap")); /* five farms, and the keep feeds more */
    /* a second structure while one waits to be placed resumes placing */
    pick_stave(0);
    press(CADV_ACT_USE);
    tick(cadv_p05_buildings()[P05_B_FARM].build_ticks + 5);
    press(CADV_ACT_ALT_FIRE);
    pick_stave(1);
    press(CADV_ACT_USE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "POINT AND FIRE"));
}

static void test_selection_groups_and_barks(void)
{
    int w;
    int i;
    start(0);
    calm();
    w = first_unit(P05_U_WORKER);
    st->rts.u[w].order = CADV_ORDER_IDLE;
    stand(31, 58);
    for (i = 0; i < 4; ++i) {
        aim(st->rts.u[w].x, 0.8f, st->rts.u[w].z);
        press(CADV_ACT_FIRE);
    }
    TEST_ASSERT_TRUE(st->marks_found & 4u); /* the third click */
    TEST_ASSERT_NOT_NULL(strstr(st->message, "POKING"));
    TEST_ASSERT_EQUAL_DOUBLE(1, query("selected"));
    /* shift + 1 sets a group, C clears, 1 recalls */
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    press(CADV_ACT_WEAPON_1);
    in.held = 0;
    TEST_ASSERT_NOT_NULL(strstr(st->message, "GROUP SET"));
    press(CADV_ACT_CROUCH);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("selected"));
    press(CADV_ACT_WEAPON_1);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("selected"));
    press(CADV_ACT_WEAPON_6);
    TEST_ASSERT_EQUAL_DOUBLE(3, query("selected"));
    add_unit(P05_GNOMES, P05_U_FOOTMAN, 31, 56);
    press(CADV_ACT_WEAPON_5);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("selected"));
    /* shift + fire adds a unit to the squad */
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    aim(st->rts.u[w].x, 0.8f, st->rts.u[w].z);
    press(CADV_ACT_FIRE);
    in.held = 0;
    TEST_ASSERT_EQUAL_DOUBLE(2, query("selected"));
    /* sweep select around the aim point */
    aim_cell(31, 55);
    press(CADV_ACT_ALT_FIRE);
    TEST_ASSERT_TRUE(query("selected") >= 3);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "WITH ME"));
    /* naming a soldier */
    press(CADV_ACT_CROUCH);
    i = first_unit(P05_U_FOOTMAN);
    aim(st->rts.u[i].x, 0.8f, st->rts.u[i].z);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_STRING("Footman", st->message);
}

static void test_orders_harvest_move_attack(void)
{
    int w;
    int f;
    int troll;
    start(0);
    calm();
    w = first_unit(P05_U_WORKER);
    stand(22, 56);
    press(CADV_ACT_WEAPON_6);
    aim_cell(19, 56); /* a tree */
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "TO WORK"));
    TEST_ASSERT_EQUAL_INT(1, st->rts.u[w].res_kind);
    tick(60 * 40);
    TEST_ASSERT_TRUE(query("lumber") > 250);
    /* workers sent at an enemy go back to work instead */
    troll = add_unit(P05_TROLLS, P05_U_AXE, 34, 58);
    stand(31, 58);
    cadv_rts_update_fog(&st->rts, P05_GNOMES);
    st->rts.u[w].order = CADV_ORDER_IDLE;
    aim(st->rts.u[troll].x, 0.8f, st->rts.u[troll].z);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_HARVEST, st->rts.u[w].order);
    press(CADV_ACT_CROUCH);
    /* soldiers: move, plain move, attack */
    f = add_unit(P05_GNOMES, P05_U_FOOTMAN, 31, 57);
    press(CADV_ACT_WEAPON_5);
    aim_cell(28, 60);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "FORWARD"));
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_ATTACK_MOVE, st->rts.u[f].order);
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    press(CADV_ACT_FIRE);
    in.held = 0;
    TEST_ASSERT_NOT_NULL(strstr(st->message, "MARCHING"));
    aim(st->rts.u[troll].x, 0.8f, st->rts.u[troll].z);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "AT THEM"));
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_ATTACK, st->rts.u[f].order);
}

static void test_shield_bash(void)
{
    int troll;
    int hp;
    start(0);
    calm();
    stand(44, 50);
    st->yaw = 0.0f;
    st->pitch = 0.0f;
    troll = add_unit(P05_TROLLS, P05_U_AXE, 44, 49);
    st->rts.u[troll].z = st->rts.u[st->captain].z - 1.5f;
    st->rts.u[troll].x = st->rts.u[st->captain].x;
    hp = st->rts.u[troll].hp;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(hp - 30, st->rts.u[troll].hp);
    TEST_ASSERT_TRUE(st->rts.u[troll].z < st->rts.u[st->captain].z - 2.5f);
    press(CADV_ACT_FIRE); /* cooling down */
    TEST_ASSERT_EQUAL_INT(hp - 30, st->rts.u[troll].hp);
    tick(60);
    st->rts.u[troll].alive = false;
    press(CADV_ACT_FIRE); /* a swing at nothing */
    TEST_ASSERT_TRUE(st->bash_cd > 0.5f);
}

static void kill_raiders(void)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (st->raider[i]) {
            cadv_rts_damage(&st->rts, i, 10000);
        }
    }
}

static void test_portal_05_hearthguard__three_raids_then_the_thaw(void)
{
    int lh;
    int drown;
    start(0);
    calm();
    st->raid_timer = 1;
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("raid"));
    TEST_ASSERT_EQUAL_DOUBLE(4, query("raiders"));
    TEST_ASSERT_TRUE(st->marks_found & 2u); /* the owl scout */
    TEST_ASSERT_NOT_NULL(strstr(st->message, "RAID"));
    kill_raiders();
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("raiders"));
    TEST_ASSERT_EQUAL_INT(1, cadv_session_result(&s).checkpoint);
    /* the Longhall is warded before spring */
    lh = st->longhall;
    cadv_rts_damage(&st->rts, CADV_RTS_TARGET_BUILDING + lh, 500);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(3000, query("longhall.hp"));
    st->raid_timer = 1;
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(7, query("raiders"));
    kill_raiders();
    st->raid_timer = 1;
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(3, query("raid"));
    TEST_ASSERT_EQUAL_DOUBLE(9, query("raiders")); /* with the warlord */
    TEST_ASSERT_TRUE(st->marks_found & 1u);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("thawed"));
    /* someone left on the ice when it gives way */
    drown = add_unit(P05_GNOMES, P05_U_FOOTMAN, 20, 28);
    kill_raiders();
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("thawed"));
    TEST_ASSERT_FALSE(st->rts.u[drown].alive);
    TEST_ASSERT_EQUAL_INT(2, cadv_session_result(&s).checkpoint);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "THAW"));
    cadv_rts_damage(&st->rts, CADV_RTS_TARGET_BUILDING + lh, 500);
    tick(1);
    TEST_ASSERT_TRUE(query("longhall.hp") < 3000);
}

static void test_raid_sizes_follow_difficulty(void)
{
    start_with(0, CADV_DIFF_HARD);
    calm();
    st->raid_timer = 1;
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(6, query("raiders")); /* 5 axes and a thrower */
    cadv_session_end(&s);
    start_with(0, CADV_DIFF_STORY);
    calm();
    st->raid_timer = 1;
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(3, query("raiders"));
}

static void test_rts_kit__ferry_loading(void)
{
    int ferry;
    int f1;
    int f2;
    int north;
    start(0);
    calm();
    rich();
    st->raid = CADV_P05_RAIDS;
    tick(2); /* no raiders: the thaw */
    TEST_ASSERT_EQUAL_DOUBLE(1, query("thawed"));
    ferry = add_unit(P05_GNOMES, P05_U_FERRY, 31, 33);
    f1 = add_unit(P05_GNOMES, P05_U_FOOTMAN, 30, 36);
    f2 = add_unit(P05_GNOMES, P05_U_ARCHER, 32, 36);
    north = add_unit(P05_GNOMES, P05_U_FOOTMAN, 31, 20); /* across the lake: cannot board */
    stand(31, 37);
    press(CADV_ACT_WEAPON_5);
    TEST_ASSERT_EQUAL_DOUBLE(3, query("selected"));
    aim(st->rts.u[ferry].x, 1.0f, st->rts.u[ferry].z);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "ALL ABOARD"));
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_IDLE, st->rts.u[north].order);
    tick(240);
    TEST_ASSERT_EQUAL_INT(ferry, st->rts.u[f1].carrier);
    TEST_ASSERT_EQUAL_INT(ferry, st->rts.u[f2].carrier);
    TEST_ASSERT_EQUAL_INT(2, st->rts.u[ferry].passengers);
    /* the Captain takes the helm and sails north */
    press(CADV_ACT_CROUCH);
    stand(31, 34);
    press(CADV_ACT_JUMP);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("aboard"));
    pick_stave(0);
    press(CADV_ACT_USE); /* nothing to build from a boat */
    TEST_ASSERT_EQUAL_INT(0, st->rts.side[P05_GNOMES].queued[0]);
    in.move_y = 1.0f;
    tick(5); /* walking does nothing aboard */
    in.move_y = 0.0f;
    st->pitch = 0.6f;
    press(CADV_ACT_FIRE); /* at the sky: no order */
    aim_cell(31, 22);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "HELMSMAN"));
    tick(60 * 25);
    /* landed: everyone ashore, the Captain too */
    TEST_ASSERT_EQUAL_INT(0, st->rts.u[ferry].passengers);
    TEST_ASSERT_EQUAL_INT(-1, st->rts.u[f1].carrier);
    TEST_ASSERT_TRUE(st->rts.u[f1].z < cell(P05_LAKE_Z0));
    TEST_ASSERT_EQUAL_DOUBLE(0, query("aboard"));
    /* board again and step off by hand */
    press(CADV_ACT_JUMP);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("aboard"));
    press(CADV_ACT_JUMP);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("aboard"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "ASHORE"));
    /* mid-lake there is no shore to step onto */
    press(CADV_ACT_JUMP);
    st->rts.u[ferry].x = cell(31);
    st->rts.u[ferry].z = cell(29);
    press(CADV_ACT_JUMP);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "NO SHORE"));
    TEST_ASSERT_EQUAL_DOUBLE(1, query("aboard"));
}

static void test_portal_05_hearthguard__the_star_comes_home(void)
{
    int i;
    start(0);
    calm();
    st->raid = CADV_P05_RAIDS;
    tick(2);
    add_unit(P05_TROLLS, P05_U_AXE, 40, 10);
    cadv_rts_damage(&st->rts, CADV_RTS_TARGET_BUILDING + st->longhall, 100000);
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("star.state"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "LONGHALL BURNS"));
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (st->rts.u[i].alive && st->rts.u[i].owner == P05_TROLLS) {
            TEST_ASSERT_EQUAL_INT(CADV_ORDER_MOVE, st->rts.u[i].order); /* scattering */
        }
    }
    cadv_p05_teleport(st, st->star_x, 0, st->star_z, 0);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("star.state"));
    /* the Captain falls: the star drops, he returns to the hall */
    cadv_rts_damage(&st->rts, st->captain, 100000);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("star.state"));
    TEST_ASSERT_EQUAL_DOUBLE(0, query("player.health"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CAPTAIN IS DOWN"));
    tick(361);
    TEST_ASSERT_TRUE(st->captain >= 0);
    cadv_p05_teleport(st, st->star_x, 0, st->star_z, 0);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("star.state"));
    stand(31, 53);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(3, query("star.state"));
    TEST_ASSERT_TRUE(st->win_timer > 0);
    tick(130);
    press(CADV_ACT_USE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    TEST_ASSERT_EQUAL_INT(2, cadv_session_result(&s).snowflakes); /* the Captain fell */
}

static void test_losing_the_hall(void)
{
    start(0);
    calm();
    cadv_rts_damage(&st->rts, CADV_RTS_TARGET_BUILDING + st->hall, 100000);
    tick(2);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "HALL HAS FALLEN"));
    tick(200);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
    TEST_ASSERT_EQUAL_INT(0, cadv_session_result(&s).snowflakes);
}

static void test_building_lost_costs_a_snowflake(void)
{
    int farm;
    start(0);
    calm();
    st->star_state = 1; /* stills the troll AI, which would recall its axeman */
    st->star_x = 0.0f;
    farm = cadv_rts_add_building(&st->rts, P05_GNOMES, P05_B_FARM, 26, 54);
    st->rts.b[farm].hp = 5;
    cadv_rts_order_attack(&st->rts, add_unit(P05_TROLLS, P05_U_AXE, 25, 56), CADV_RTS_TARGET_BUILDING + farm);
    tick(900);
    TEST_ASSERT_FALSE(st->rts.b[farm].alive);
    TEST_ASSERT_TRUE(st->building_lost_in_raids);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "FALLEN"));
}

static void test_friend_marks_tavern(void)
{
    start(0);
    calm();
    TEST_ASSERT_FALSE(st->marks_found & 8u);
    stand(24, 60);
    tick(1);
    TEST_ASSERT_TRUE(st->marks_found & 8u);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "TAVERN SIGN"));
}

static void test_foreman_and_new_workers(void)
{
    int i;
    int tallow = 0;
    start(0);
    calm();
    rich();
    /* the mine runs dry: its workers move on */
    for (i = 0; i < P05_W * P05_H; ++i) {
        if (st->rts.grid.ore_kind[i] == 0) {
            st->rts.grid.ore[i] = 0;
        }
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (st->rts.u[i].alive && st->rts.u[i].def == P05_U_WORKER) {
            st->rts.u[i].res_kind = 0;
            st->rts.u[i].order = CADV_ORDER_HARVEST;
        }
    }
    tick(61);
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (st->rts.u[i].alive && st->rts.u[i].def == P05_U_WORKER) {
            TEST_ASSERT_EQUAL_INT(1, st->rts.u[i].res_kind);
        }
    }
    /* a shipyard wants tallow: the foreman sends workers, new ones go too */
    cadv_rts_add_building(&st->rts, P05_GNOMES, P05_B_SHIPYARD, 36, 34);
    st->rts.side[P05_GNOMES].tallow = 0;
    tick(130);
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        tallow += st->rts.u[i].alive && st->rts.u[i].def == P05_U_WORKER && st->rts.u[i].res_kind == 2;
    }
    TEST_ASSERT_EQUAL_INT(2, tallow);
    pick_stave(P05_STAVE_STRUCTURES);
    press(CADV_ACT_USE);
    tick(cadv_p05_units()[P05_U_WORKER].build_ticks + 5);
    i = first_unit(P05_U_WORKER);
    TEST_ASSERT_EQUAL_DOUBLE(4, query("workers"));
    /* an idle worker nobody is commanding is put back to work */
    st->rts.u[i].order = CADV_ORDER_IDLE;
    st->rts.u[i].path_len = 0;
    tick(61);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_HARVEST, st->rts.u[i].order);
}

static void test_portal_05_hearthguard__resume_from_checkpoints(void)
{
    start(1);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("raid"));
    TEST_ASSERT_EQUAL_DOUBLE(6, query("buildings"));
    TEST_ASSERT_EQUAL_DOUBLE(7, query("workers"));
    TEST_ASSERT_EQUAL_DOUBLE(5, query("army"));
    cadv_session_end(&s);
    start(2);
    TEST_ASSERT_EQUAL_DOUBLE(3, query("raid"));
    TEST_ASSERT_EQUAL_DOUBLE(1, query("thawed"));
    TEST_ASSERT_EQUAL_DOUBLE(150, query("tallow"));
    TEST_ASSERT_EQUAL_DOUBLE(9, query("army"));
}

static void test_aim_queries_and_quit(void)
{
    const char *keys[] = {"player.x", "player.y", "player.z", "player.yaw", "player.health", "gold", "lumber", "tallow",
                          "supply", "supply.cap", "buildings", "workers", "army", "ferries", "raid", "raiders",
                          "thawed", "keep", "longhall.hp", "star.state", "aboard", "selected", "placing", "stave",
                          "built", "outcome", "marks", "ticks"};
    size_t i;
    double v;
    float gx;
    float gz;
    start(0);
    for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        TEST_ASSERT_TRUE(cadv_p05_query(st, keys[i], &v));
    }
    TEST_ASSERT_FALSE(cadv_p05_query(st, "bogus", &v));
    cadv_p05_teleport(st, 60, 0, 150, 1.0f);
    TEST_ASSERT_EQUAL_DOUBLE(60, query("player.x"));
    st->pitch = 0.3f;
    TEST_ASSERT_FALSE(cadv_p05_aim_ground(st, &gx, &gz));
    st->pitch = -0.01f;
    st->yaw = 3.14159f; /* far over the southern edge */
    TEST_ASSERT_FALSE(cadv_p05_aim_ground(st, &gx, &gz));
    TEST_ASSERT_EQUAL_INT(-1, cadv_p05_aim_target(st, false));
    /* the owl circles over the lake */
    TEST_ASSERT_TRUE(cadv_p05_owl(st).y > 10.0f);
    /* walk into a building: blocked */
    stand(31, 54);
    st->yaw = 0.0f;
    in.move_y = 1.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    tick(120);
    in.move_y = 0.0f;
    in.held = 0;
    TEST_ASSERT_TRUE(query("player.z") > cell(52));
    press(CADV_ACT_PAUSE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    tick(1); /* ignored after the end */
}

static void play_bot(int max)
{
    int i;
    for (i = 0; i < max && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_input_frame bot;
        cadv_p05_bot(st, &bot);
        cadv_session_tick(&s, &bot);
    }
}

static void test_bot_finishes_on_every_difficulty(void)
{
    start_with(0, CADV_DIFF_DEFAULT);
    play_bot(120000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).snowflakes >= 2);
    cadv_session_end(&s);
    start_with(1, CADV_DIFF_HARD);
    play_bot(120000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    cadv_session_end(&s);
    start_with(2, CADV_DIFF_STORY);
    play_bot(120000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_catalog__portal_05_hearthguard);
    RUN_TEST(test_portal_05_hearthguard__three_resource_costs_on_the_stave);
    RUN_TEST(test_stave_cycles_and_map);
    RUN_TEST(test_build_and_place_a_farm);
    RUN_TEST(test_shipyard_needs_the_shore);
    RUN_TEST(test_training_refusals_and_the_keep);
    RUN_TEST(test_selection_groups_and_barks);
    RUN_TEST(test_orders_harvest_move_attack);
    RUN_TEST(test_shield_bash);
    RUN_TEST(test_portal_05_hearthguard__three_raids_then_the_thaw);
    RUN_TEST(test_raid_sizes_follow_difficulty);
    RUN_TEST(test_rts_kit__ferry_loading);
    RUN_TEST(test_portal_05_hearthguard__the_star_comes_home);
    RUN_TEST(test_losing_the_hall);
    RUN_TEST(test_building_lost_costs_a_snowflake);
    RUN_TEST(test_friend_marks_tavern);
    RUN_TEST(test_foreman_and_new_workers);
    RUN_TEST(test_portal_05_hearthguard__resume_from_checkpoints);
    RUN_TEST(test_aim_queries_and_quit);
    RUN_TEST(test_bot_finishes_on_every_difficulty);
    return UNITY_END();
}
