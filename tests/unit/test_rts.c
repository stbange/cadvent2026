/* SPDX-License-Identifier: MIT */
/* RTS kit: grid, placement, paths, power, queues, harvesters, orders, hash. */
#include <string.h>

#include "kits/rts/rts.h"
#include "kits/rts/rts_ai.h"
#include "unity.h"

enum { B_YARD, B_POWER, B_REFINERY, B_FACTORY, B_TOWER, B_COUNT };
enum { U_RIFLE, U_HARVESTER, U_TANK, U_ENGINEER, U_COUNT };

static const cadv_rts_bdef k_b[B_COUNT] = {
    /* name        fw fh cost power hp  ticks flags                      range dmg cool */
    {"Yard", 3, 3, 0, 0, 900, 1, CADV_BF_HQ, 0, 0, 0, 0},
    {"Power", 2, 2, 300, 100, 400, 100, 0, 0, 0, 0, 0},
    {"Refinery", 3, 2, 600, -40, 600, 200, CADV_BF_REFINERY, 0, 0, 0, 0},
    {"Factory", 3, 3, 800, -60, 700, 200, CADV_BF_FACTORY, 0, 0, 0, 0},
    {"Tower", 1, 1, 400, -30, 300, 100, CADV_BF_DEFENCE, 12.0f, 10, 30, 0},
};
static const cadv_rts_udef k_u[U_COUNT] = {
    /* name       cost hp  speed range dmg cool ticks harv  carry */
    {"Rifle", 100, 50, 0.05f, 8.0f, 6, 20, 60, false, 0, 0, 0, false},
    {"Harvester", 700, 400, 0.06f, 0.0f, 0, 0, 120, true, 100, 0, 0, false},
    {"Tank", 600, 300, 0.08f, 14.0f, 40, 60, 120, false, 0, 0, 0, false},
    {"Engineer", 300, 40, 0.05f, 0.0f, 0, 0, 60, false, 0, 0, 0, false},
};

static cadv_rts r;

void setUp(void) { TEST_ASSERT_TRUE(cadv_rts_init(&r, 40, 40, 2.0f, k_b, B_COUNT, k_u, U_COUNT)); }
void tearDown(void) {}

static void run(int n)
{
    int i;
    for (i = 0; i < n; ++i) {
        cadv_rts_tick(&r);
    }
}

static void test_grid_basics(void)
{
    cadv_rts_grid g;
    cadv_cell c;
    float x;
    float z;
    TEST_ASSERT_FALSE(cadv_rts_grid_init(&g, 0, 4, 1.0f));
    TEST_ASSERT_FALSE(cadv_rts_grid_init(&g, CADV_RTS_GRID_MAX + 1, 4, 1.0f));
    TEST_ASSERT_FALSE(cadv_rts_grid_init(&g, 4, 4, 0.0f));
    TEST_ASSERT_FALSE(cadv_rts_init(&r, 0, 4, 1.0f, k_b, B_COUNT, k_u, U_COUNT));
    TEST_ASSERT_TRUE(cadv_rts_grid_init(&g, 8, 8, 2.0f));
    c = cadv_rts_cell_at(&g, 5.0f, 0.5f);
    TEST_ASSERT_EQUAL_INT(2, c.x);
    TEST_ASSERT_EQUAL_INT(0, c.z);
    c = cadv_rts_cell_at(&g, -0.5f, -4.0f);
    TEST_ASSERT_EQUAL_INT(-1, c.x);
    TEST_ASSERT_EQUAL_INT(-2, c.z);
    c.x = 3;
    c.z = 1;
    cadv_rts_cell_centre(&g, c, &x, &z);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, x);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, z);
    cadv_rts_set_blocked(&g, -1, -1, 1, 1);
    TEST_ASSERT_FALSE(cadv_rts_passable(&g, 0, 0));
    TEST_ASSERT_FALSE(cadv_rts_passable(&g, 9, 0));
    cadv_rts_add_ore(&g, 5, 5, 1, 40000);
    cadv_rts_add_ore(&g, 5, 5, 1, 40000);
    TEST_ASSERT_EQUAL_UINT16(65535, g.ore[cadv_rts_index(&g, 5, 5)]);
    TEST_ASSERT_EQUAL_UINT16(0, g.ore[cadv_rts_index(&g, 6, 6)]); /* outside the circle */
    cadv_rts_add_ore(&g, 0, 0, 0, 10);                             /* blocked: no ore */
    TEST_ASSERT_EQUAL_UINT16(0, g.ore[0]);
    TEST_ASSERT_EQUAL_INT(0, cadv_rts_mine(&g, -1, 0, 5));
    TEST_ASSERT_EQUAL_INT(5, cadv_rts_mine(&g, 5, 5, 5));
    TEST_ASSERT_TRUE(cadv_rts_nearest_ore(&g, 0, 7, &c));
    TEST_ASSERT_EQUAL_INT(4, c.x);
    TEST_ASSERT_EQUAL_INT(5, c.z);
    memset(g.ore, 0, sizeof g.ore);
    TEST_ASSERT_FALSE(cadv_rts_nearest_ore(&g, 0, 7, &c));
}

static void test_rts_kit__placement_validation(void)
{
    cadv_rts_grid *g = &r.grid;
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_OUT_OF_BOUNDS, cadv_rts_can_place(g, 38, 5, 3, 3, 0, 3, true));
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_OUT_OF_BOUNDS, cadv_rts_can_place(g, 5, 5, 0, 3, 0, 3, true));
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_TOO_FAR, cadv_rts_can_place(g, 5, 5, 2, 2, 0, 3, false));
    TEST_ASSERT_TRUE(cadv_rts_add_building(&r, 0, B_YARD, 5, 5) >= 0);
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_add_building(&r, 0, B_POWER, 6, 6)); /* overlap */
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_BLOCKED, cadv_rts_can_place(g, 7, 7, 2, 2, 0, 3, false));
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_OK, cadv_rts_can_place(g, 9, 5, 2, 2, 0, 3, false));     /* adjacent */
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_OK, cadv_rts_can_place(g, 10, 5, 2, 2, 0, 3, false));    /* within reach */
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_TOO_FAR, cadv_rts_can_place(g, 12, 5, 2, 2, 0, 3, false));
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_TOO_FAR, cadv_rts_can_place(g, 9, 5, 2, 2, 1, 3, false)); /* not yours */
    cadv_rts_add_ore(g, 20, 20, 2, 500);
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_BLOCKED, cadv_rts_can_place(g, 19, 19, 2, 2, 0, 3, true));
    cadv_rts_set_blocked(g, 30, 30, 30, 30);
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_BLOCKED, cadv_rts_can_place(g, 29, 29, 2, 2, 0, 3, true));
}

static void test_rts_kit__paths_avoid_buildings(void)
{
    cadv_cell a = {2, 10};
    cadv_cell b = {20, 10};
    cadv_cell p[CADV_RTS_PATH_MAX];
    int n;
    int i;
    n = cadv_rts_path(&r.grid, a, b, p, CADV_RTS_PATH_MAX);
    TEST_ASSERT_EQUAL_INT(18, n);
    TEST_ASSERT_EQUAL_INT(0, cadv_rts_path(&r.grid, a, a, p, CADV_RTS_PATH_MAX));
    /* a wall with one gap */
    cadv_rts_set_blocked(&r.grid, 10, 0, 10, 39);
    r.grid.kind[cadv_rts_index(&r.grid, 10, 30)] = CADV_CELL_OPEN;
    n = cadv_rts_path(&r.grid, a, b, p, CADV_RTS_PATH_MAX);
    TEST_ASSERT_TRUE(n > 18);
    for (i = 0; i < n; ++i) {
        TEST_ASSERT_TRUE(cadv_rts_passable(&r.grid, p[i].x, p[i].z));
        if (p[i].x == 10) {
            TEST_ASSERT_EQUAL_INT(30, p[i].z);
        }
    }
    TEST_ASSERT_EQUAL_INT(20, p[n - 1].x);
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_path(&r.grid, a, b, p, 5)); /* too long for the buffer */
    r.grid.kind[cadv_rts_index(&r.grid, 10, 30)] = CADV_CELL_BLOCKED;
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_path(&r.grid, a, b, p, CADV_RTS_PATH_MAX)); /* sealed */
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_path(&r.grid, a, (cadv_cell){99, 0}, p, CADV_RTS_PATH_MAX));
    /* a building as the goal: the path ends beside it */
    TEST_ASSERT_TRUE(cadv_rts_add_building(&r, 0, B_YARD, 3, 20) >= 0);
    n = cadv_rts_path(&r.grid, (cadv_cell){1, 30}, (cadv_cell){4, 21}, p, CADV_RTS_PATH_MAX);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(cadv_rts_passable(&r.grid, p[n - 1].x, p[n - 1].z));
    /* corners are not cut */
    cadv_rts_grid_init(&r.grid, 4, 4, 1.0f);
    cadv_rts_set_blocked(&r.grid, 1, 0, 1, 0);
    cadv_rts_set_blocked(&r.grid, 0, 1, 0, 1);
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_path(&r.grid, (cadv_cell){0, 0}, (cadv_cell){1, 1}, p, 8));
}

static void test_rts_kit__power_math(void)
{
    int yard = cadv_rts_add_building(&r, 0, B_YARD, 2, 2);
    TEST_ASSERT_TRUE(yard >= 0);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cadv_rts_power_factor(&r, 0));
    cadv_rts_add_building(&r, 0, B_POWER, 5, 2);      /* +100 */
    cadv_rts_add_building(&r, 0, B_FACTORY, 2, 5);    /* -60 */
    TEST_ASSERT_EQUAL_INT(100, r.side[0].power_made);
    TEST_ASSERT_EQUAL_INT(60, r.side[0].power_used);
    TEST_ASSERT_TRUE(cadv_rts_defences_online(&r, 0));
    cadv_rts_add_building(&r, 0, B_REFINERY, 5, 5);   /* -40 */
    cadv_rts_add_building(&r, 0, B_TOWER, 8, 2);      /* -30: 100 made / 130 used */
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 100.0f / 130.0f, cadv_rts_power_factor(&r, 0));
    TEST_ASSERT_FALSE(cadv_rts_defences_online(&r, 0));
    /* low power: building takes longer */
    r.side[0].credits = 1000;
    TEST_ASSERT_TRUE(cadv_rts_queue(&r, 0, true, B_POWER));
    run(100);
    TEST_ASSERT_EQUAL_INT(-1, r.side[0].ready_building);
    run(32); /* 10000 / 76 per tick */
    TEST_ASSERT_EQUAL_INT(B_POWER, r.side[0].ready_building);
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_OK, cadv_rts_place_ready(&r, 0, 8, 4));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cadv_rts_power_factor(&r, 0));
    /* a destroyed power plant cuts power again; floor of 25 % */
    r.events[0] = 0;
    cadv_rts_destroy_building(&r, r.grid.building[cadv_rts_index(&r.grid, 5, 2)]);
    TEST_ASSERT_TRUE(r.events[0] & CADV_RTS_EV_LOW_POWER); /* the moment power drops */
    cadv_rts_destroy_building(&r, r.grid.building[cadv_rts_index(&r.grid, 8, 4)]);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, cadv_rts_power_factor(&r, 0));
    cadv_rts_destroy_building(&r, yard);
    cadv_rts_destroy_building(&r, yard); /* already gone */
    while (r.ticks % 300 != 0) {
        run(1);
    }
    run(1);
    TEST_ASSERT_TRUE(r.events[0] & CADV_RTS_EV_LOW_POWER); /* and as a reminder */
}

static void test_rts_kit__build_queue_pays_up_front(void)
{
    int i;
    cadv_rts_add_building(&r, 0, B_YARD, 2, 2);
    TEST_ASSERT_FALSE(cadv_rts_queue(&r, 0, true, B_POWER)); /* broke */
    r.side[0].credits = 5000;
    TEST_ASSERT_FALSE(cadv_rts_queue(&r, 0, false, U_RIFLE)); /* no factory */
    for (i = 0; i < CADV_RTS_QUEUE; ++i) {
        TEST_ASSERT_TRUE(cadv_rts_queue(&r, 0, true, B_POWER));
    }
    TEST_ASSERT_FALSE(cadv_rts_queue(&r, 0, true, B_POWER)); /* full */
    TEST_ASSERT_EQUAL_INT(5000 - 5 * 300, r.side[0].credits);
    TEST_ASSERT_TRUE(cadv_rts_cancel(&r, 0, true));
    TEST_ASSERT_EQUAL_INT(5000 - 4 * 300, r.side[0].credits);
    run(100);
    TEST_ASSERT_TRUE(r.events[0] & CADV_RTS_EV_BUILT);
    run(200); /* the next one waits for the first to be placed */
    TEST_ASSERT_EQUAL_INT(3, r.side[0].queued[0]);
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_TOO_FAR, cadv_rts_place_ready(&r, 0, 30, 30));
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_OK, cadv_rts_place_ready(&r, 0, 5, 2));
    TEST_ASSERT_EQUAL_INT(CADV_PLACE_BLOCKED, cadv_rts_place_ready(&r, 0, 7, 2)); /* nothing ready */
    run(1);
    TEST_ASSERT_EQUAL_INT(2, r.side[0].queued[0]);
    while (cadv_rts_cancel(&r, 0, true)) {
    }
    TEST_ASSERT_FALSE(cadv_rts_cancel(&r, 0, true));
}

static void test_units_spawn_at_the_factory_and_rally(void)
{
    int u;
    cadv_rts_add_building(&r, 0, B_FACTORY, 10, 10);
    cadv_rts_add_building(&r, 0, B_POWER, 14, 10);
    r.side[0].credits = 1000;
    r.side[0].rally_x = 20;
    r.side[0].rally_z = 20;
    TEST_ASSERT_TRUE(cadv_rts_queue(&r, 0, false, U_RIFLE));
    run(60);
    TEST_ASSERT_TRUE(r.events[0] & CADV_RTS_EV_UNIT_READY);
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_units(&r, 0, U_RIFLE));
    u = 0;
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_ATTACK_MOVE, r.u[u].order);
    run(2000);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_IDLE, r.u[u].order);
    TEST_ASSERT_EQUAL_INT(20, cadv_rts_cell_at(&r.grid, r.u[u].x, r.u[u].z).x);
    /* factory lost while a unit is queued: it waits */
    TEST_ASSERT_TRUE(cadv_rts_queue(&r, 0, false, U_RIFLE));
    cadv_rts_destroy_building(&r, 0);
    run(100);
    TEST_ASSERT_EQUAL_INT(1, r.side[0].queued[1]);
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_buildings(&r, 0, -1)); /* only the power plant */
}

static void test_rts_kit__harvester_cycle(void)
{
    int h;
    int i;
    int ref = cadv_rts_add_building(&r, 0, B_REFINERY, 4, 4);
    TEST_ASSERT_TRUE(ref >= 0);
    cadv_rts_add_ore(&r.grid, 20, 6, 1, 60);
    h = cadv_rts_add_unit(&r, 0, U_HARVESTER, 12.0f, 14.0f);
    TEST_ASSERT_EQUAL_INT(CADV_HARV_SEEK, r.u[h].hstate);
    run(1);
    TEST_ASSERT_EQUAL_INT(CADV_HARV_TO_ORE, r.u[h].hstate);
    for (i = 0; i < 3000 && r.u[h].hstate != CADV_HARV_MINING; ++i) {
        run(1);
    }
    TEST_ASSERT_EQUAL_INT(CADV_HARV_MINING, r.u[h].hstate);
    for (i = 0; i < 8000 && r.u[h].hstate != CADV_HARV_UNLOAD; ++i) {
        run(1);
    }
    TEST_ASSERT_EQUAL_INT(CADV_HARV_UNLOAD, r.u[h].hstate);
    TEST_ASSERT_EQUAL_INT(100, r.u[h].cargo);
    for (i = 0; i < 200 && r.u[h].cargo > 0; ++i) {
        run(1);
    }
    TEST_ASSERT_EQUAL_INT(100, r.side[0].credits);
    TEST_ASSERT_EQUAL_INT(100, r.side[0].harvested);
    /* the field runs dry: the last partial load still comes home */
    for (i = 0; i < 40000 && r.side[0].harvested < 5 * 60; ++i) {
        run(1);
    }
    TEST_ASSERT_EQUAL_INT(5 * 60, r.side[0].harvested); /* 5 cells of 60 */
    run(200);
    TEST_ASSERT_EQUAL_INT(CADV_HARV_SEEK, r.u[h].hstate);
    TEST_ASSERT_EQUAL_INT(0, r.u[h].cargo);
    /* no refinery: a loaded harvester waits */
    cadv_rts_add_ore(&r.grid, 20, 20, 0, 100);
    cadv_rts_destroy_building(&r, ref);
    for (i = 0; i < 6000 && r.u[h].hstate != CADV_HARV_RETURN; ++i) {
        run(1);
    }
    run(100);
    TEST_ASSERT_EQUAL_INT(CADV_HARV_RETURN, r.u[h].hstate);
    TEST_ASSERT_EQUAL_INT(100, r.u[h].cargo);
    cadv_rts_order_stop(&r, h);
    cadv_rts_order_harvest(&r, h);
    TEST_ASSERT_EQUAL_INT(CADV_HARV_SEEK, r.u[h].hstate);
}

static void test_rts_kit__control_groups(void)
{
    int a = cadv_rts_add_unit(&r, 0, U_RIFLE, 3, 3);
    int b = cadv_rts_add_unit(&r, 0, U_RIFLE, 5, 3);
    int e = cadv_rts_add_unit(&r, 1, U_RIFLE, 70, 70);
    int members[4];
    cadv_cell goal = {10, 10};
    members[0] = a;
    members[1] = b;
    members[2] = e;  /* not ours: ignored */
    members[3] = 999; /* invalid: ignored */
    cadv_rts_group_set(&r, 0, 1, members, 4);
    TEST_ASSERT_EQUAL_INT(2, cadv_rts_group_move(&r, 0, 1, goal, false));
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_MOVE, r.u[a].order);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_IDLE, r.u[e].order);
    run(1000);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_IDLE, r.u[a].order);
    TEST_ASSERT_EQUAL_INT(10, cadv_rts_cell_at(&r.grid, r.u[b].x, r.u[b].z).x);
    /* a dead unit leaves its groups */
    r.u[e].x = r.u[a].x + 1.0f;
    r.u[e].z = r.u[a].z;
    r.u[a].hp = 1;
    r.u[b].hp = 1;
    run(30);
    TEST_ASSERT_TRUE(r.side[0].units_lost >= 1);
    TEST_ASSERT_TRUE(r.events[0] & CADV_RTS_EV_UNIT_LOST || r.side[0].units_lost == 2);
    TEST_ASSERT_TRUE(cadv_rts_group_move(&r, 0, 1, goal, true) < 2);
}

static void test_combat_orders_and_defences(void)
{
    int tank = cadv_rts_add_unit(&r, 0, U_TANK, 4, 4);
    int eng = cadv_rts_add_unit(&r, 0, U_ENGINEER, 6, 4);
    int hq = cadv_rts_add_building(&r, 1, B_YARD, 30, 30);
    int tower = cadv_rts_add_building(&r, 1, B_POWER, 25, 30);
    int i;
    TEST_ASSERT_TRUE(hq >= 0 && tower >= 0);
    i = cadv_rts_add_building(&r, 1, B_TOWER, 28, 26);
    cadv_rts_order_attack(&r, tank, CADV_RTS_TARGET_BUILDING + i);
    cadv_rts_order_move(&r, eng, (cadv_cell){8, 4}, false);
    run(3000);
    TEST_ASSERT_FALSE(r.b[i].alive);
    cadv_rts_order_attack(&r, tank, CADV_RTS_TARGET_BUILDING + hq);
    for (i = 0; i < 20000 && r.b[hq].alive; ++i) {
        run(1);
    }
    TEST_ASSERT_FALSE(r.b[hq].alive);
    run(1);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_IDLE, r.u[tank].order); /* target gone */
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_IDLE, r.u[eng].order);
    /* idle units defend themselves; attacking your own side does nothing */
    cadv_rts_order_attack(&r, tank, eng);
    run(1);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_IDLE, r.u[tank].order);
    i = cadv_rts_add_unit(&r, 1, U_RIFLE, r.u[tank].x + 4.0f, r.u[tank].z);
    run(200);
    TEST_ASSERT_FALSE(r.u[i].alive);
    /* attack-move engages on the way */
    i = cadv_rts_add_unit(&r, 1, U_RIFLE, 40, 60);
    cadv_rts_order_move(&r, tank, (cadv_cell){20, 35}, true);
    run(3000);
    TEST_ASSERT_FALSE(r.u[i].alive);
    /* unreachable goals retry later */
    cadv_rts_set_blocked(&r.grid, 0, 38, 39, 38);
    cadv_rts_order_move(&r, eng, (cadv_cell){5, 39}, false);
    TEST_ASSERT_EQUAL_INT(30, r.u[eng].repath);
    run(30);
    TEST_ASSERT_EQUAL_INT(30, r.u[eng].repath); /* retried and failed again */
    /* building in the way: the path is recomputed */
    cadv_rts_order_move(&r, eng, (cadv_cell){15, 4}, false);
    cadv_rts_add_building(&r, 0, B_POWER, (int)(r.u[eng].x / 2.0f) + 2, 3);
    run(2000);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_IDLE, r.u[eng].order);
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_units(&r, 0, U_TANK));
    TEST_ASSERT_EQUAL_INT(2, cadv_rts_count_units(&r, 0, -1));
    /* a powered tower shoots intruders, an unpowered one does not */
    r.u[tank].x = 3.0f; /* keep the tank out of it */
    r.u[tank].z = 3.0f;
    cadv_rts_order_stop(&r, tank);
    tower = cadv_rts_add_building(&r, 1, B_POWER, 30, 34);
    i = cadv_rts_add_building(&r, 1, B_TOWER, 26, 34);
    TEST_ASSERT_TRUE(i >= 0 && tower >= 0);
    TEST_ASSERT_TRUE(cadv_rts_defences_online(&r, 1));
    r.u[eng].x = 53.0f;
    r.u[eng].z = 71.0f;
    cadv_rts_order_stop(&r, eng);
    run(2);
    TEST_ASSERT_LESS_THAN_INT(k_u[U_ENGINEER].hp, r.u[eng].hp);
    cadv_rts_destroy_building(&r, tower);
    r.u[eng].hp = k_u[U_ENGINEER].hp;
    run(100);
    TEST_ASSERT_EQUAL_INT(k_u[U_ENGINEER].hp, r.u[eng].hp);
}

static void test_capacity_limits(void)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        TEST_ASSERT_TRUE(cadv_rts_add_unit(&r, 0, U_RIFLE, 1, 1) >= 0);
    }
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_add_unit(&r, 0, U_RIFLE, 1, 1));
    cadv_rts_init(&r, 90, 90, 2.0f, k_b, B_COUNT, k_u, U_COUNT);
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        TEST_ASSERT_TRUE(cadv_rts_add_building(&r, 0, B_TOWER, (i % 30) * 3, (i / 30) * 3) >= 0);
    }
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_add_building(&r, 0, B_TOWER, 80, 80));
}

static void script(cadv_rts *w)
{
    int i;
    cadv_rts_init(w, 40, 40, 2.0f, k_b, B_COUNT, k_u, U_COUNT);
    cadv_rts_add_building(w, 0, B_YARD, 2, 2);
    cadv_rts_add_building(w, 0, B_REFINERY, 6, 2);
    cadv_rts_add_building(w, 0, B_FACTORY, 2, 6);
    cadv_rts_add_ore(&w->grid, 20, 8, 2, 300);
    cadv_rts_add_unit(w, 0, U_HARVESTER, 14, 14);
    cadv_rts_add_unit(w, 1, U_TANK, 60, 60);
    w->side[0].credits = 2000;
    cadv_rts_queue(w, 0, false, U_TANK);
    for (i = 0; i < 1500; ++i) {
        cadv_rts_tick(w);
        if (i == 400) {
            cadv_rts_order_move(w, 1, (cadv_cell){8, 12}, true);
        }
    }
}

static void test_rts_kit__lockstep_hash_equality(void)
{
    static cadv_rts a;
    static cadv_rts b;
    script(&a);
    script(&b);
    TEST_ASSERT_TRUE(cadv_rts_hash(&a) == cadv_rts_hash(&b));
    b.side[0].credits += 1;
    TEST_ASSERT_TRUE(cadv_rts_hash(&a) != cadv_rts_hash(&b));
    b.side[0].credits -= 1;
    b.u[0].x += 0.001f;
    TEST_ASSERT_TRUE(cadv_rts_hash(&a) != cadv_rts_hash(&b));
}

static void test_rts_kit__fog_of_war(void)
{
    int u = cadv_rts_add_unit(&r, 0, U_RIFLE, 11.0f, 11.0f);
    TEST_ASSERT_FALSE(cadv_rts_cell_visible(&r, 0, 11.0f, 11.0f));
    run(1);
    TEST_ASSERT_TRUE(cadv_rts_cell_visible(&r, 0, 11.0f, 11.0f));
    TEST_ASSERT_FALSE(cadv_rts_cell_visible(&r, 0, 70.0f, 70.0f));
    TEST_ASSERT_FALSE(cadv_rts_cell_visible(&r, 1, 11.0f, 11.0f)); /* the other side sees nothing */
    TEST_ASSERT_FALSE(cadv_rts_cell_visible(&r, 0, -5.0f, 11.0f));
    r.u[u].x = 60.0f;
    r.u[u].z = 60.0f;
    run(1);
    TEST_ASSERT_FALSE(cadv_rts_cell_visible(&r, 0, 11.0f, 11.0f)); /* no longer in sight ... */
    TEST_ASSERT_EQUAL_UINT8(1, r.explored[0][cadv_rts_index(&r.grid, 5, 5)]); /* ... but explored */
    cadv_rts_add_building(&r, 0, B_YARD, 30, 30);
    cadv_rts_reveal(&r, 0, 20.0f, 70.0f, 2);
    TEST_ASSERT_TRUE(cadv_rts_cell_visible(&r, 0, 20.0f, 70.0f));
    run(1);
    TEST_ASSERT_TRUE(cadv_rts_cell_visible(&r, 0, 62.0f, 62.0f)); /* the yard sees too */
}

static void test_rts_kit__neutral_buildings_and_capture(void)
{
    int lab = cadv_rts_add_building(&r, CADV_RTS_NEUTRAL, B_POWER, 10, 10);
    int tank = cadv_rts_add_unit(&r, 0, U_TANK, 30.0f, 22.0f);
    int hp = r.b[lab].hp;
    run(200);
    TEST_ASSERT_EQUAL_INT(hp, r.b[lab].hp); /* idle units leave neutral buildings alone */
    TEST_ASSERT_EQUAL_INT(CADV_RTS_NEUTRAL, r.b[lab].owner);
    TEST_ASSERT_EQUAL_INT(lab, cadv_rts_building_at(&r, 10, 10));
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_building_at(&r, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, cadv_rts_building_at(&r, -1, 0));
    cadv_rts_capture(&r, lab, 0);
    TEST_ASSERT_EQUAL_INT(0, r.b[lab].owner);
    TEST_ASSERT_EQUAL_INT(0, r.grid.owner[cadv_rts_index(&r.grid, 11, 11)]);
    TEST_ASSERT_EQUAL_INT(100, r.side[0].power_made);
    cadv_rts_capture(&r, lab, 0); /* already ours: nothing changes */
    cadv_rts_destroy_building(&r, lab);
    cadv_rts_capture(&r, lab, 1); /* gone: nothing changes */
    TEST_ASSERT_EQUAL_INT(0, r.side[0].power_made);
    (void)tank;
}

static void test_rts_kit__barracks_and_factory_queues_run_together(void)
{
    static const cadv_rts_udef u2[2] = {
        {"Trooper", 100, 50, 0.05f, 8.0f, 6, 20, 60, false, 0, CADV_BF_BARRACKS, 0, true},
        {"Tank", 100, 300, 0.08f, 14.0f, 40, 60, 60, false, 0, 0, 0, false},
    };
    static const cadv_rts_bdef b2[2] = {
        {"Barracks", 2, 2, 100, 0, 400, 50, CADV_BF_BARRACKS, 0, 0, 0, 0},
        {"Factory", 2, 2, 100, 0, 400, 50, CADV_BF_FACTORY, 0, 0, 0, 0},
    };
    cadv_rts_init(&r, 40, 40, 2.0f, b2, 2, u2, 2);
    r.side[0].credits = 1000;
    TEST_ASSERT_FALSE(cadv_rts_can_train(&r, 0, 0));
    cadv_rts_add_building(&r, 0, 0, 5, 5);
    TEST_ASSERT_TRUE(cadv_rts_can_train(&r, 0, 0));
    TEST_ASSERT_FALSE(cadv_rts_can_train(&r, 0, 1)); /* default: a factory */
    TEST_ASSERT_TRUE(cadv_rts_queue(&r, 0, false, 0));
    TEST_ASSERT_FALSE(cadv_rts_queue(&r, 0, false, 1));
    TEST_ASSERT_TRUE(cadv_rts_queue(&r, 0, true, 1));
    r.side[0].credits = 0;
    TEST_ASSERT_FALSE(cadv_rts_queue(&r, 0, false, 0));
    TEST_ASSERT_TRUE(r.events[0] & CADV_RTS_EV_BROKE);
    run(60);
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_units(&r, 0, 0));
    TEST_ASSERT_EQUAL_INT(1, r.side[0].ready_building); /* both queues advanced together */
    TEST_ASSERT_TRUE(cadv_rts_cancel(&r, 0, false) == false);
}

static void test_rts_kit__ai_opponent(void)
{
    static const cadv_rts_ai_build plan[] = {{B_POWER, 20, 4}, {B_FACTORY, 20, 8}, {B_REFINERY, 25, 4}, {B_POWER, 2, 38}};
    static const int train[] = {U_RIFLE, U_TANK};
    cadv_rts_ai ai;
    int i;
    int base;
    cadv_rts_ai_init(&ai, 1);
    TEST_ASSERT_EQUAL_INT(1, ai.side);
    TEST_ASSERT_EQUAL_INT(-1, ai.harvester_def);
    ai.plan = plan;
    ai.nplan = 4;
    ai.train = train;
    ai.ntrain = 2;
    ai.harvester_def = U_HARVESTER;
    ai.income = 50;
    ai.wave_every = 600;
    ai.wave_size = 2;
    ai.wave_max = 3;
    ai.army_cap = 4;
    ai.home.x = 22;
    ai.home.z = 6;
    ai.target.x = 5;
    ai.target.z = 35;
    base = cadv_rts_add_building(&r, 1, B_YARD, 22, 2);
    TEST_ASSERT_TRUE(base >= 0);
    r.side[1].credits = 4000;
    for (i = 0; i < 6000; ++i) {
        cadv_rts_ai_tick(&r, &ai);
        cadv_rts_tick(&r);
    }
    TEST_ASSERT_EQUAL_INT(2, cadv_rts_count_buildings(&r, 1, B_POWER) + (cadv_rts_building_at(&r, 2, 38) < 0 ? 1 : 0));
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_buildings(&r, 1, B_FACTORY));
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_units(&r, 1, U_HARVESTER));
    TEST_ASSERT_TRUE(ai.waves_sent >= 1);
    TEST_ASSERT_TRUE(cadv_rts_ai_army(&r, 1) <= 4 + 2); /* capped (queued units may still land) */
    TEST_ASSERT_TRUE(ai.wave_size <= 3);
    /* a lost structure is rebuilt */
    cadv_rts_destroy_building(&r, cadv_rts_building_at(&r, 20, 8));
    for (i = 0; i < 3000; ++i) {
        cadv_rts_ai_tick(&r, &ai);
        cadv_rts_tick(&r);
    }
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_buildings(&r, 1, B_FACTORY));
    /* intruders near home are attacked by idle units */
    i = cadv_rts_add_unit(&r, 0, U_RIFLE, 46.0f, 14.0f);
    for (base = 0; base < 4000 && r.u[i].alive; ++base) {
        cadv_rts_ai_tick(&r, &ai);
        cadv_rts_tick(&r);
    }
    TEST_ASSERT_FALSE(r.u[i].alive);
    TEST_ASSERT_TRUE(cadv_rts_ai_idle_army(&r, 1) >= 0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_grid_basics);
    RUN_TEST(test_rts_kit__placement_validation);
    RUN_TEST(test_rts_kit__paths_avoid_buildings);
    RUN_TEST(test_rts_kit__power_math);
    RUN_TEST(test_rts_kit__build_queue_pays_up_front);
    RUN_TEST(test_units_spawn_at_the_factory_and_rally);
    RUN_TEST(test_rts_kit__harvester_cycle);
    RUN_TEST(test_rts_kit__control_groups);
    RUN_TEST(test_combat_orders_and_defences);
    RUN_TEST(test_capacity_limits);
    RUN_TEST(test_rts_kit__lockstep_hash_equality);
    RUN_TEST(test_rts_kit__fog_of_war);
    RUN_TEST(test_rts_kit__neutral_buildings_and_capture);
    RUN_TEST(test_rts_kit__barracks_and_factory_queues_run_together);
    RUN_TEST(test_rts_kit__ai_opponent);
    return UNITY_END();
}
