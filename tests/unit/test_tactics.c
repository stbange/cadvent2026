/* SPDX-License-Identifier: MIT */
/* FPS kit tactics: recoil, spread, economy, smoke and the walk grid. */
#include <string.h>

#include "kits/fps/tactics.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_fps_tactics__recoil_pattern_determinism(void)
{
    cadv_recoil r = {0.02f, 0.03f, 8};
    float y0;
    float p0;
    float y1;
    float p1;
    int k;
    cadv_recoil_offset(&r, 0, &y0, &p0);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, y0); /* the first round goes where you aim */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, p0);
    for (k = 1; k < 30; ++k) {
        cadv_recoil_offset(&r, k, &y0, &p0);
        cadv_recoil_offset(&r, k, &y1, &p1);
        TEST_ASSERT_EQUAL_FLOAT(y0, y1); /* the same every time */
        TEST_ASSERT_EQUAL_FLOAT(p0, p1);
    }
    /* it climbs for the first rounds, then drifts sideways */
    cadv_recoil_offset(&r, 4, &y0, &p0);
    cadv_recoil_offset(&r, 8, &y1, &p1);
    TEST_ASSERT_TRUE(p1 > p0 && p0 > 0.05f);
    cadv_recoil_offset(&r, 14, &y0, &p0);
    cadv_recoil_offset(&r, 22, &y1, &p1);
    TEST_ASSERT_TRUE(cadv_absf(y0 - y1) > 0.01f);
    TEST_ASSERT_TRUE(cadv_absf(p1 - p0) < 0.1f);
}

static void test_fps_tactics__movement_inaccuracy(void)
{
    float still = cadv_spread(0.01f, 0.05f, 0.0f, 5.0f, false, false);
    float moving = cadv_spread(0.01f, 0.05f, 5.0f, 5.0f, false, false);
    float air = cadv_spread(0.01f, 0.05f, 5.0f, 5.0f, true, false);
    TEST_ASSERT_EQUAL_FLOAT(0.01f, still);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.06f, moving);
    TEST_ASSERT_TRUE(air > moving);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.005f, cadv_spread(0.01f, 0.05f, 0.0f, 5.0f, false, true));
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.06f, cadv_spread(0.01f, 0.05f, 50.0f, 5.0f, false, false)); /* clamped */
    TEST_ASSERT_EQUAL_FLOAT(0.01f, cadv_spread(0.01f, 0.05f, 3.0f, 0.0f, false, false));
}

static void test_fps_tactics__economy_rules_table(void)
{
    cadv_econ_rules r = cadv_econ_default();
    TEST_ASSERT_EQUAL_INT(800, r.start);
    TEST_ASSERT_EQUAL_INT(3250, cadv_econ_round_reward(&r, true, CADV_WIN_ELIMINATION, 0, false, true));
    TEST_ASSERT_EQUAL_INT(3500, cadv_econ_round_reward(&r, true, CADV_WIN_BOMB, 0, true, true));
    TEST_ASSERT_EQUAL_INT(3500, cadv_econ_round_reward(&r, true, CADV_WIN_DEFUSE, 0, true, false));
    TEST_ASSERT_EQUAL_INT(3250, cadv_econ_round_reward(&r, true, CADV_WIN_TIME, 0, false, false));
    TEST_ASSERT_EQUAL_INT(1400, cadv_econ_round_reward(&r, false, CADV_WIN_ELIMINATION, 1, false, false));
    TEST_ASSERT_EQUAL_INT(1400, cadv_econ_round_reward(&r, false, CADV_WIN_ELIMINATION, 0, false, false));
    TEST_ASSERT_EQUAL_INT(1900, cadv_econ_round_reward(&r, false, CADV_WIN_ELIMINATION, 2, false, false));
    TEST_ASSERT_EQUAL_INT(3400, cadv_econ_round_reward(&r, false, CADV_WIN_ELIMINATION, 9, false, false)); /* capped */
    /* attackers who planted get the bonus on a loss; defenders do not */
    TEST_ASSERT_EQUAL_INT(2200, cadv_econ_round_reward(&r, false, CADV_WIN_DEFUSE, 1, true, true));
    TEST_ASSERT_EQUAL_INT(1400, cadv_econ_round_reward(&r, false, CADV_WIN_BOMB, 1, true, false));
    TEST_ASSERT_EQUAL_INT(16000, cadv_econ_add(&r, 15000, 3500));
    TEST_ASSERT_EQUAL_INT(0, cadv_econ_add(&r, 100, -400));
    TEST_ASSERT_EQUAL_INT(1100, cadv_econ_add(&r, 800, r.kill));
}

static void test_fps_tactics__smoke_blocks_line_of_sight(void)
{
    cadv_vec3 smoke = cadv_v3(5, 1, 0);
    TEST_ASSERT_TRUE(cadv_segment_sphere(cadv_v3(0, 1, 0), cadv_v3(10, 1, 0), smoke, 2.0f));
    TEST_ASSERT_FALSE(cadv_segment_sphere(cadv_v3(0, 1, 5), cadv_v3(10, 1, 5), smoke, 2.0f));
    TEST_ASSERT_FALSE(cadv_segment_sphere(cadv_v3(0, 1, 0), cadv_v3(2, 1, 0), smoke, 2.0f)); /* stops short */
    TEST_ASSERT_TRUE(cadv_segment_sphere(cadv_v3(5, 1, 0), cadv_v3(5, 1, 0), smoke, 2.0f));  /* degenerate, inside */
}

static void test_fps_tactics__walk_grid_paths_and_cover(void)
{
    cadv_nav n;
    cadv_nav_cell path[64];
    cadv_nav_cell a = {1, 1};
    cadv_nav_cell b = {8, 1};
    cadv_nav_cell c;
    int x;
    int z;
    TEST_ASSERT_FALSE(cadv_nav_init(&n, 1, 5, 2.0f));
    TEST_ASSERT_FALSE(cadv_nav_init(&n, 99, 5, 2.0f));
    TEST_ASSERT_FALSE(cadv_nav_init(&n, 10, 10, 0.0f));
    TEST_ASSERT_TRUE(cadv_nav_init(&n, 10, 10, 2.0f));
    for (z = 0; z < 10; ++z) {
        for (x = 0; x < 10; ++x) {
            cadv_nav_set(&n, x, z, true);
        }
    }
    /* a wall with a gap at the bottom */
    for (z = 0; z < 9; ++z) {
        cadv_nav_set(&n, 5, z, false);
    }
    cadv_nav_set(&n, 99, 99, true); /* ignored */
    TEST_ASSERT_FALSE(cadv_nav_walkable(&n, 5, 3));
    TEST_ASSERT_FALSE(cadv_nav_walkable(&n, -1, 3));
    TEST_ASSERT_EQUAL_INT(0, cadv_nav_path(&n, a, a, path, 64));
    x = cadv_nav_path(&n, a, b, path, 64);
    TEST_ASSERT_TRUE(x > 7);
    TEST_ASSERT_EQUAL_INT(8, path[x - 1].x);
    TEST_ASSERT_EQUAL_INT(9, path[x / 2].z); /* through the gap */
    TEST_ASSERT_EQUAL_INT(-1, cadv_nav_path(&n, a, b, path, 3)); /* too long */
    cadv_nav_set(&n, 5, 9, false);
    TEST_ASSERT_EQUAL_INT(-1, cadv_nav_path(&n, a, b, path, 64)); /* sealed */
    c.x = 5;
    c.z = 4;
    TEST_ASSERT_EQUAL_INT(-1, cadv_nav_path(&n, a, c, path, 64)); /* into a wall */
    /* cover hugs the wall */
    cadv_nav_mark_cover(&n);
    TEST_ASSERT_TRUE(cadv_nav_nearest_cover(&n, a, 4, &c));
    TEST_ASSERT_TRUE(c.x == 4 || c.x == 0 || c.z == 0);
    TEST_ASSERT_FALSE(cadv_nav_nearest_cover(&n, (cadv_nav_cell){2, 4}, 0, &c));
    /* world <-> cells */
    c = cadv_nav_cell_at(&n, 7.0f, 3.0f);
    TEST_ASSERT_EQUAL_INT(3, c.x);
    TEST_ASSERT_EQUAL_INT(1, c.z);
    c = cadv_nav_cell_at(&n, -4.0f, 99.0f);
    TEST_ASSERT_EQUAL_INT(0, c.x);
    TEST_ASSERT_EQUAL_INT(9, c.z);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, cadv_nav_centre(&n, (cadv_nav_cell){3, 1}).x);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fps_tactics__recoil_pattern_determinism);
    RUN_TEST(test_fps_tactics__movement_inaccuracy);
    RUN_TEST(test_fps_tactics__economy_rules_table);
    RUN_TEST(test_fps_tactics__smoke_blocks_line_of_sight);
    RUN_TEST(test_fps_tactics__walk_grid_paths_and_cover);
    return UNITY_END();
}
