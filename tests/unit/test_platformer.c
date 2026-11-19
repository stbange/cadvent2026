/* SPDX-License-Identifier: MIT */
/* Platformer kit: tiles and grid-aware traversal (jumps, ledges, blocks, water). */
#include <string.h>

#include "kits/platformer/climber.h"
#include "kits/platformer/tiles.h"
#include "unity.h"

static cadv_tiles t;
static cadv_climber c;
static cadv_climber_def d;
static cadv_input_frame in;

void setUp(void)
{
    TEST_ASSERT_TRUE(cadv_tiles_init(&t, 20, 10, 24, CADV_TILE_AIR));
    cadv_tiles_fill(&t, 0, 0, 0, 19, 1, 23, CADV_TILE_ROCK); /* floor, top at y = 2 */
    cadv_climber_default_def(&d);
    memset(&in, 0, sizeof in);
}
void tearDown(void) {}

static uint32_t step(int n)
{
    uint32_t ev = 0;
    int i;
    for (i = 0; i < n; ++i) {
        ev |= cadv_climber_tick(&c, &d, &in, &t, 1.0f / 60.0f);
        in.pressed = 0;
        in.look_x = in.look_y = 0.0f;
    }
    return ev;
}

static void test_tiles_basics(void)
{
    cadv_tiles big;
    TEST_ASSERT_FALSE(cadv_tiles_init(&big, 0, 1, 1, CADV_TILE_AIR));
    TEST_ASSERT_FALSE(cadv_tiles_init(&big, 1000, 100, 100, CADV_TILE_AIR));
    TEST_ASSERT_EQUAL_INT(CADV_TILE_ROCK, cadv_tiles_get(&t, -1, 5, 5)); /* sides are rock */
    TEST_ASSERT_EQUAL_INT(CADV_TILE_ROCK, cadv_tiles_get(&t, 5, 10, 5)); /* so is the sky */
    TEST_ASSERT_EQUAL_INT(CADV_TILE_AIR, cadv_tiles_get(&t, 5, -1, 5));  /* the abyss */
    cadv_tiles_set(&t, 99, 0, 0, CADV_TILE_ICE);                         /* ignored */
    {
        uint32_t v = t.version;
        cadv_tiles_set(&t, 3, 4, 3, CADV_TILE_ICE);
        TEST_ASSERT_TRUE(t.version > v);
        v = t.version;
        cadv_tiles_set(&t, 3, 4, 3, CADV_TILE_ICE); /* unchanged: no new version */
        TEST_ASSERT_EQUAL_UINT32(v, t.version);
    }
    TEST_ASSERT_TRUE(cadv_tile_solid(CADV_TILE_DOOR));
    TEST_ASSERT_FALSE(cadv_tile_solid(CADV_TILE_WATER));
    TEST_ASSERT_EQUAL_INT(CADV_TILE_ICE, cadv_tiles_at(&t, cadv_v3(3.5f, 4.2f, 3.9f)));
    TEST_ASSERT_EQUAL_INT(CADV_TILE_ROCK, cadv_tiles_at(&t, cadv_v3(-0.5f, 4.2f, 3.9f)));
    TEST_ASSERT_FALSE(cadv_tiles_line_clear(&t, cadv_v3(1, 4.5f, 3.5f), cadv_v3(6, 4.5f, 3.5f)));
    TEST_ASSERT_TRUE(cadv_tiles_line_clear(&t, cadv_v3(1, 6.5f, 3.5f), cadv_v3(6, 6.5f, 3.5f)));
    TEST_ASSERT_TRUE(cadv_tiles_line_clear(&t, cadv_v3(1, 6.5f, 3.5f), cadv_v3(1, 6.5f, 3.5f)));
    TEST_ASSERT_FALSE(cadv_tiles_line_clear(&t, cadv_v3(3.5f, 4.5f, 3.5f), cadv_v3(3.5f, 4.5f, 3.5f)));
    TEST_ASSERT_EQUAL_FLOAT(5.0f, cadv_tiles_floor(&t, 3.5f, 7.0f, 3.5f));
    TEST_ASSERT_EQUAL_FLOAT(2.0f, cadv_tiles_floor(&t, 5.5f, 7.0f, 3.5f));
    cadv_tiles_fill(&t, 8, 0, 8, 8, 1, 8, CADV_TILE_AIR);
    TEST_ASSERT_EQUAL_FLOAT(-1000.0f, cadv_tiles_floor(&t, 8.5f, 3.0f, 8.5f));
}

/* Runs at a gap of 'n' tiles (cells z = 11-n .. 10 removed) and reports
 * whether the player lands on the far side. */
static bool jump_gap(int n, bool run)
{
    int i;
    cadv_tiles_fill(&t, 0, 0, 11 - n, 19, 1, 10, CADV_TILE_AIR);
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 16.5f), 0.0f);
    in.move_y = 1.0f;
    in.held = run ? CADV_ACT_BIT(CADV_ACT_RUN) : 0;
    for (i = 0; i < 600 && c.pos.z > 11.05f; ++i) {
        step(1);
    }
    in.pressed = CADV_ACT_BIT(CADV_ACT_JUMP);
    in.held |= CADV_ACT_BIT(CADV_ACT_JUMP);
    step(1);
    in.held &= ~CADV_ACT_BIT(CADV_ACT_JUMP);
    for (i = 0; i < 240 && c.state == CADV_CLIMB_AIR; ++i) {
        step(1);
    }
    in.move_y = 0.0f;
    in.held = 0;
    return c.state == CADV_CLIMB_GROUND && c.pos.y == 2.0f && c.pos.z < (float)(11 - n);
}

static void test_platformer_kit__jump_distance_per_tile_class(void)
{
    TEST_ASSERT_TRUE(jump_gap(1, false));  /* standing jump: one tile */
    setUp();
    TEST_ASSERT_FALSE(jump_gap(2, false)); /* ... but not two */
    setUp();
    TEST_ASSERT_TRUE(jump_gap(2, true));   /* running jump: two tiles */
    setUp();
    TEST_ASSERT_FALSE(jump_gap(3, true));  /* nothing clears three */
}

static void test_jumps_snap_to_the_grid(void)
{
    uint32_t ev;
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 10.5f), 0.3f); /* ~17 degrees off -z */
    in.move_y = 1.0f;
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    ev = step(1);
    TEST_ASSERT_TRUE(ev & CADV_CLIMB_EV_JUMP);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.vel.x);
    TEST_ASSERT_TRUE(c.vel.z < 0.0f);
    in.held = 0;
    ev = step(120);
    TEST_ASSERT_TRUE(ev & CADV_CLIMB_EV_LAND);
    TEST_ASSERT_TRUE(c.last_fall > 0.5f && c.last_fall < 1.2f);
    /* standing still: straight up */
    in.move_y = 0.0f;
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    step(1);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.vel.z);
    in.held = 0;
    step(120);
}

static void wall(int height)
{
    cadv_tiles_fill(&t, 0, 2, 5, 19, 1 + height, 5, CADV_TILE_ROCK); /* a wall at z = 5 */
}

static bool climb_wall(int height)
{
    int i;
    wall(height);
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 8.5f), 0.0f);
    in.move_y = 1.0f;
    step(90); /* walk into the wall */
    in.move_y = 0.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_USE) | CADV_ACT_BIT(CADV_ACT_JUMP);
    in.pressed = CADV_ACT_BIT(CADV_ACT_JUMP);
    for (i = 0; i < 60 && c.state != CADV_CLIMB_HANG && c.state != CADV_CLIMB_PULLUP; ++i) {
        step(1);
        in.held = CADV_ACT_BIT(CADV_ACT_USE);
    }
    if (c.state == CADV_CLIMB_HANG) {
        in.move_y = 1.0f;
        step(1);
        in.move_y = 0.0f;
    }
    step(60);
    in.held = 0;
    return c.state == CADV_CLIMB_GROUND && c.pos.y == (float)(2 + height) && c.pos.z < 6.0f;
}

static void test_platformer_kit__ledge_grab_tolerance_window(void)
{
    TEST_ASSERT_TRUE(climb_wall(1)); /* a step: climbed straight away */
    setUp();
    TEST_ASSERT_TRUE(climb_wall(2)); /* jump, grab, pull up */
    setUp();
    TEST_ASSERT_FALSE(climb_wall(3)); /* out of reach */
    setUp();
    /* without holding USE nothing is caught */
    wall(2);
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 6.31f), 0.0f);
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    step(1);
    in.held = 0;
    step(60);
    TEST_ASSERT_EQUAL_INT(CADV_CLIMB_GROUND, c.state);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, c.pos.y);
}

static void test_hanging_shimmy_and_drop(void)
{
    uint32_t ev;
    float x0;
    wall(2);
    cadv_tiles_fill(&t, 13, 4, 5, 13, 5, 5, CADV_TILE_ROCK); /* a pillar on top ends the ledge at x = 13 */
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 6.31f), 0.0f);
    in.held = CADV_ACT_BIT(CADV_ACT_USE) | CADV_ACT_BIT(CADV_ACT_JUMP);
    in.pressed = CADV_ACT_BIT(CADV_ACT_JUMP);
    ev = step(1);
    in.held = CADV_ACT_BIT(CADV_ACT_USE);
    ev |= step(30);
    TEST_ASSERT_TRUE(ev & CADV_CLIMB_EV_GRAB);
    TEST_ASSERT_EQUAL_INT(CADV_CLIMB_HANG, c.state);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f - 1.5f, c.pos.y);
    x0 = c.pos.x;
    in.move_x = 1.0f; /* shimmy right (+x) until the pillar blocks the ledge */
    step(240);
    TEST_ASSERT_TRUE(c.pos.x > x0 + 1.0f);
    TEST_ASSERT_TRUE(c.pos.x < 13.0f);
    in.move_x = -1.0f;
    step(30);
    TEST_ASSERT_TRUE(c.pos.x < 12.9f);
    in.move_x = 0.0f;
    in.held = 0; /* let go */
    step(60);
    TEST_ASSERT_EQUAL_INT(CADV_CLIMB_GROUND, c.state);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, c.pos.y);
}

static void test_platformer_kit__block_push_collision(void)
{
    uint32_t ev;
    cadv_tiles_set(&t, 10, 2, 8, CADV_TILE_BLOCK);
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 10.5f), 0.0f);
    in.move_y = 1.0f;
    step(60); /* up against the block */
    in.held = CADV_ACT_BIT(CADV_ACT_USE);
    ev = step(40); /* one push takes 0.7 s; holding on keeps pushing */
    TEST_ASSERT_TRUE(ev & CADV_CLIMB_EV_PUSHED);
    TEST_ASSERT_EQUAL_INT(CADV_TILE_BLOCK, cadv_tiles_get(&t, 10, 2, 7));
    TEST_ASSERT_EQUAL_INT(CADV_TILE_AIR, cadv_tiles_get(&t, 10, 2, 8));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 8.32f, c.pos.z);
    /* a wall behind the block stops it */
    cadv_tiles_set(&t, 10, 2, 5, CADV_TILE_ROCK);
    step(200);
    TEST_ASSERT_EQUAL_INT(CADV_TILE_BLOCK, cadv_tiles_get(&t, 10, 2, 6));
    TEST_ASSERT_EQUAL_INT(CADV_TILE_ROCK, cadv_tiles_get(&t, 10, 2, 5));
    /* no floor beyond: it does not slide over the edge */
    cadv_tiles_set(&t, 10, 2, 5, CADV_TILE_AIR);
    cadv_tiles_fill(&t, 10, 0, 5, 10, 1, 5, CADV_TILE_AIR);
    step(200);
    TEST_ASSERT_EQUAL_INT(CADV_TILE_BLOCK, cadv_tiles_get(&t, 10, 2, 6));
    /* pulling brings it back one tile */
    in.move_y = -1.0f;
    ev = step(50);
    TEST_ASSERT_TRUE(ev & CADV_CLIMB_EV_PUSHED);
    TEST_ASSERT_EQUAL_INT(CADV_TILE_BLOCK, cadv_tiles_get(&t, 10, 2, 7));
    TEST_ASSERT_EQUAL_INT(CADV_TILE_AIR, cadv_tiles_get(&t, 10, 2, 6));
    /* pulling into a wall behind the player does nothing */
    cadv_tiles_fill(&t, 10, 2, 9, 10, 3, 9, CADV_TILE_ROCK);
    step(100);
    TEST_ASSERT_EQUAL_INT(CADV_TILE_BLOCK, cadv_tiles_get(&t, 10, 2, 8));
    step(100);
    TEST_ASSERT_EQUAL_INT(CADV_TILE_BLOCK, cadv_tiles_get(&t, 10, 2, 8));
}

static void test_platformer_kit__air_meter_drowning(void)
{
    uint32_t ev = 0;
    int i;
    cadv_tiles_fill(&t, 0, 2, 0, 19, 6, 23, CADV_TILE_WATER);
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 4.5f, 10.5f), 0.0f);
    ev = step(2);
    TEST_ASSERT_TRUE(ev & CADV_CLIMB_EV_SPLASH);
    TEST_ASSERT_EQUAL_INT(CADV_CLIMB_SWIM, c.state);
    in.held = CADV_ACT_BIT(CADV_ACT_CROUCH);
    step(120);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 2.0f, c.pos.y); /* on the bottom */
    in.held = 0;
    for (i = 0; i < 31 * 60 && !(ev & CADV_CLIMB_EV_DROWNING); ++i) {
        ev = step(1);
    }
    TEST_ASSERT_TRUE(ev & CADV_CLIMB_EV_DROWNING);
    TEST_ASSERT_TRUE(i > 27 * 60); /* about 30 s of breath */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.air);
    /* swim up: breath comes back at the surface */
    in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    step(240);
    TEST_ASSERT_TRUE(c.pos.y + d.eye > 7.0f);
    TEST_ASSERT_TRUE(c.air > 10.0f);
    /* swim forward along the view */
    {
        float z0 = c.pos.z;
        in.held = 0;
        in.move_y = 1.0f;
        in.move_x = 0.5f;
        step(60);
        TEST_ASSERT_TRUE(c.pos.z < z0 - 1.5f);
    }
}

static void test_climbing_out_of_water(void)
{
    cadv_tiles_fill(&t, 0, 2, 8, 19, 5, 23, CADV_TILE_WATER); /* pool z >= 8, surface at y = 6 */
    cadv_tiles_fill(&t, 0, 2, 0, 19, 5, 7, CADV_TILE_ROCK);   /* bank at z < 8, top at y = 6 */
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 4.5f, 9.0f), 0.0f);
    step(30);
    TEST_ASSERT_EQUAL_INT(CADV_CLIMB_SWIM, c.state);
    in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    step(90); /* rise to the surface */
    in.move_y = 1.0f;
    step(30);
    in.move_y = 0.0f;
    in.pressed = CADV_ACT_BIT(CADV_ACT_JUMP);
    step(60);
    in.held = 0;
    TEST_ASSERT_EQUAL_INT(CADV_CLIMB_GROUND, c.state);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, c.pos.y);
}

static void test_walking_off_an_edge_and_into_the_abyss(void)
{
    uint32_t ev;
    cadv_tiles_fill(&t, 0, 0, 0, 19, 1, 9, CADV_TILE_AIR);
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 11.0f), 0.0f);
    in.move_y = 1.0f;
    ev = step(400);
    TEST_ASSERT_TRUE(ev & CADV_CLIMB_EV_ABYSS);
    TEST_ASSERT_EQUAL_INT(CADV_CLIMB_AIR, c.state);
    {
        int dx;
        int dz;
        c.yaw = 1.5708f; /* facing -x */
        cadv_climber_facing(&c, &dx, &dz);
        TEST_ASSERT_EQUAL_INT(-1, dx);
        TEST_ASSERT_EQUAL_INT(0, dz);
        TEST_ASSERT_TRUE(cadv_climber_forward(&c).x < -0.9f);
        TEST_ASSERT_EQUAL_FLOAT(c.pos.y + d.eye, cadv_climber_eye(&c, &d).y);
    }
}

static void test_head_bump_and_wall_stop_in_flight(void)
{
    cadv_tiles_fill(&t, 0, 4, 0, 19, 4, 23, CADV_TILE_ROCK); /* low ceiling: 2 tiles of room */
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 10.5f), 0.0f);
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    step(1);
    in.held = 0;
    step(10);
    TEST_ASSERT_TRUE(c.vel.y <= 0.0f); /* bumped */
    step(60);
    setUp();
    wall(3);
    cadv_climber_spawn(&c, &d, cadv_v3(10.5f, 2.0f, 6.6f), 0.0f);
    in.move_y = 1.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    in.pressed = CADV_ACT_BIT(CADV_ACT_JUMP);
    in.held |= CADV_ACT_BIT(CADV_ACT_JUMP);
    step(12);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.vel.z); /* stopped by the wall */
    in.held = 0;
    in.move_y = 0.0f;
    step(60);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tiles_basics);
    RUN_TEST(test_platformer_kit__jump_distance_per_tile_class);
    RUN_TEST(test_jumps_snap_to_the_grid);
    RUN_TEST(test_platformer_kit__ledge_grab_tolerance_window);
    RUN_TEST(test_hanging_shimmy_and_drop);
    RUN_TEST(test_platformer_kit__block_push_collision);
    RUN_TEST(test_platformer_kit__air_meter_drowning);
    RUN_TEST(test_climbing_out_of_water);
    RUN_TEST(test_walking_off_an_edge_and_into_the_abyss);
    RUN_TEST(test_head_bump_and_wall_stop_in_flight);
    return UNITY_END();
}
