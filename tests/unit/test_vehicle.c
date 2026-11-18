/* SPDX-License-Identifier: MIT */
/* Vehicle kit: deformable heightmap terrain and the hovering flyer. */
#include <string.h>

#include "kits/vehicle/flyer.h"
#include "kits/vehicle/terrain.h"
#include "unity.h"

static cadv_terrain t;

void setUp(void) { TEST_ASSERT_TRUE(cadv_terrain_init(&t, 33, 2.0f, 0.0f, 40.0f, 5.0f)); }
void tearDown(void) {}

static void test_init_rejects_bad_sizes(void)
{
    cadv_terrain u;
    TEST_ASSERT_FALSE(cadv_terrain_init(&u, 1, 2.0f, 0, 10, 1));
    TEST_ASSERT_FALSE(cadv_terrain_init(&u, CADV_TERRAIN_MAX_N + 16, 2.0f, 0, 10, 1));
    TEST_ASSERT_FALSE(cadv_terrain_init(&u, 30, 2.0f, 0, 10, 1)); /* not chunk aligned */
    TEST_ASSERT_FALSE(cadv_terrain_init(&u, 33, 0.0f, 0, 10, 1));
    TEST_ASSERT_FALSE(cadv_terrain_init(&u, 33, 2.0f, 10, 10, 1));
    TEST_ASSERT_EQUAL_FLOAT(64.0f, cadv_terrain_size(&t));
    TEST_ASSERT_EQUAL_INT(2, cadv_terrain_chunks_per_side(&t));
}

static void test_islands_and_sampling(void)
{
    cadv_island isl = {32.0f, 32.0f, 20.0f, 12.0f, 0.4f};
    cadv_vec3 n;
    cadv_terrain_islands(&t, &isl, 1, 3u);
    TEST_ASSERT_TRUE(cadv_terrain_height(&t, 32, 32) > 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, cadv_terrain_height(&t, 1, 1)); /* sea floor */
    TEST_ASSERT_EQUAL_FLOAT(5.0f, cadv_terrain_surface(&t, 1, 1));  /* water on top */
    TEST_ASSERT_EQUAL_FLOAT(cadv_terrain_sample(&t, 0, 0), cadv_terrain_sample(&t, -5, -9));
    TEST_ASSERT_EQUAL_FLOAT(cadv_terrain_sample(&t, 32, 32), cadv_terrain_sample(&t, 99, 99));
    TEST_ASSERT_EQUAL_FLOAT(cadv_terrain_sample(&t, 32, 32), cadv_terrain_height(&t, 500, 500));
    n = cadv_terrain_normal(&t, 1, 1);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, n.y);
    n = cadv_terrain_normal(&t, 32 + 16, 32); /* east shoulder leans east */
    TEST_ASSERT_TRUE(n.x > 0.05f);
}

static void test_vehicle_kit__deformation_stays_in_bounds(void)
{
    uint32_t v0 = t.chunk_ver[0];
    uint32_t v3 = t.chunk_ver[3];
    float moved;
    int i;
    for (i = 0; i < 30; ++i) {
        cadv_terrain_deform(&t, 10, 10, 6.0f, 5.0f);
    }
    TEST_ASSERT_EQUAL_FLOAT(40.0f, cadv_terrain_height(&t, 10, 10));
    for (i = 0; i < 30; ++i) {
        cadv_terrain_deform(&t, 10, 10, 6.0f, -5.0f);
    }
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_terrain_height(&t, 10, 10));
    TEST_ASSERT_TRUE(t.chunk_ver[0] > v0);
    TEST_ASSERT_EQUAL_UINT32(v3, t.chunk_ver[3]); /* far chunk untouched */
    moved = cadv_terrain_deform(&t, 10, 10, 6.0f, -5.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, moved); /* already at the floor */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_terrain_deform(&t, 10, 10, 0.0f, 5.0f));
    TEST_ASSERT_TRUE(cadv_terrain_deform(&t, 10, 10, 6.0f, 2.0f) > 0.0f);
    for (i = 0; i < 33 * 33; ++i) {
        TEST_ASSERT_TRUE(t.h[i] >= 0.0f && t.h[i] <= 40.0f);
    }
}

static void test_raise_flatten_roughness(void)
{
    float r;
    cadv_terrain_raise_to(&t, 32, 32, 6.0f, 20.0f);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, cadv_terrain_height(&t, 32, 32));
    cadv_terrain_raise_to(&t, 32, 32, 6.0f, 20.0f); /* idempotent */
    TEST_ASSERT_EQUAL_FLOAT(20.0f, cadv_terrain_height(&t, 32, 32));
    cadv_terrain_raise_to(&t, 32, 32, 6.0f, 5.0f); /* never lowers */
    TEST_ASSERT_EQUAL_FLOAT(20.0f, cadv_terrain_height(&t, 32, 32));
    r = cadv_terrain_roughness(&t, 32, 32, 8.0f);
    TEST_ASSERT_TRUE(r > 15.0f);
    cadv_terrain_flatten(&t, 32, 32, 10.0f, 3.0f, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, cadv_terrain_height(&t, 32, 32));
    cadv_terrain_flatten(&t, 32, 32, 10.0f, 3.0f, 100.0f); /* nothing left to move */
    TEST_ASSERT_TRUE(cadv_terrain_roughness(&t, 32, 32, 8.0f) < r);
    cadv_terrain_flatten(&t, 60, 60, 4.0f, 10.0f, 1.0f); /* rate-limited */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, cadv_terrain_height(&t, 60, 60));
}

static void test_raycast_and_line_of_sight(void)
{
    cadv_vec3 hit;
    cadv_terrain_raise_to(&t, 32, 32, 8.0f, 20.0f);
    TEST_ASSERT_TRUE(cadv_terrain_raycast(&t, cadv_v3(32, 30, 32), cadv_v3(0, -1, 0), 50.0f, &hit));
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 20.0f, hit.y);
    TEST_ASSERT_FALSE(cadv_terrain_raycast(&t, cadv_v3(32, 30, 32), cadv_v3(0, 1, 0), 50.0f, &hit));
    TEST_ASSERT_FALSE(cadv_terrain_raycast(&t, cadv_v3(32, 30, 32), cadv_v3(0, -1, 0), 5.0f, &hit));
    TEST_ASSERT_TRUE(cadv_terrain_raycast(&t, cadv_v3(32, 10, 32), cadv_v3(1, 0, 0), 5.0f, &hit)); /* starts inside */
    TEST_ASSERT_EQUAL_FLOAT(10.0f, hit.y);
    TEST_ASSERT_FALSE(cadv_terrain_line_clear(&t, cadv_v3(10, 8, 32), cadv_v3(54, 8, 32)));
    TEST_ASSERT_TRUE(cadv_terrain_line_clear(&t, cadv_v3(10, 30, 32), cadv_v3(54, 30, 32)));
    TEST_ASSERT_TRUE(cadv_terrain_line_clear(&t, cadv_v3(5, 30, 5), cadv_v3(5, 30, 5)));
    TEST_ASSERT_FALSE(cadv_terrain_line_clear(&t, cadv_v3(32, 1, 32), cadv_v3(32, 1, 32)));
}

static cadv_flyer_def def(void)
{
    cadv_flyer_def d = {10.0f, 40.0f, 1.0f, 6.0f, 2.0f, 30.0f, 2.0f};
    return d;
}

static void test_vehicle_kit__flyer_keeps_clearance(void)
{
    cadv_flyer f;
    cadv_flyer_def d = def();
    cadv_input_frame in;
    int i;
    memset(&in, 0, sizeof in);
    cadv_flyer_spawn(&f, cadv_v3(20, 8, 20), 0.0f);
    in.held = CADV_ACT_BIT(CADV_ACT_CROUCH);
    for (i = 0; i < 120; ++i) {
        cadv_flyer_tick(&f, &d, &in, &t, 1.0f / 60.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 7.0f, f.pos.y); /* water 5 + clearance 2 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 2.0f, cadv_flyer_altitude(&f, &t));
    in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    for (i = 0; i < 600; ++i) {
        cadv_flyer_tick(&f, &d, &in, &t, 1.0f / 60.0f);
    }
    TEST_ASSERT_EQUAL_FLOAT(30.0f, f.pos.y); /* ceiling */
    /* thrust forward (-z) with boost, then hit the world edge */
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    in.move_y = 1.0f;
    in.move_x = 1.0f;
    in.look_y = 2.0f;
    for (i = 0; i < 600; ++i) {
        cadv_flyer_tick(&f, &d, &in, &t, 1.0f / 60.0f);
        in.look_y = 0.0f;
    }
    TEST_ASSERT_EQUAL_FLOAT(1.0f, f.pos.z);
    TEST_ASSERT_EQUAL_FLOAT(63.0f, f.pos.x);
    TEST_ASSERT_TRUE(cadv_v3_len(f.vel) <= 20.0f + 1e-3f);
    TEST_ASSERT_EQUAL_FLOAT(1.2f, f.pitch);
    TEST_ASSERT_TRUE(cadv_flyer_forward(&f).y > 0.9f);
}

static void test_vehicle_kit__cliffs_stop_the_flyer(void)
{
    cadv_flyer f;
    cadv_flyer_def d = def();
    cadv_input_frame in;
    int i;
    bool bumped = false;
    memset(&in, 0, sizeof in);
    for (i = 0; i < 33; ++i) {
        cadv_terrain_raise_to(&t, (float)i * 2.0f, 20.0f, 3.0f, 25.0f); /* a wall along x */
    }
    cadv_flyer_spawn(&f, cadv_v3(32, 8, 40), 0.0f);
    in.move_y = 1.0f;
    for (i = 0; i < 300; ++i) {
        cadv_flyer_tick(&f, &d, &in, &t, 1.0f / 60.0f);
        bumped = bumped || f.bumped;
    }
    TEST_ASSERT_TRUE(bumped);
    TEST_ASSERT_GREATER_THAN_FLOAT(21.0f, f.pos.z); /* still south of the wall */
    TEST_ASSERT_LESS_THAN_FLOAT(12.0f, f.pos.y);
    /* a gentle slope is climbed instead */
    cadv_terrain_init(&t, 33, 2.0f, 0.0f, 40.0f, 5.0f);
    for (i = 0; i < 33; ++i) {
        cadv_terrain_raise_to(&t, (float)i * 2.0f, 10.0f, 30.0f, 9.0f);
    }
    cadv_flyer_spawn(&f, cadv_v3(32, 7, 60), 0.0f);
    for (i = 0; i < 300; ++i) {
        cadv_flyer_tick(&f, &d, &in, &t, 1.0f / 60.0f);
    }
    TEST_ASSERT_LESS_THAN_FLOAT(20.0f, f.pos.z);
    TEST_ASSERT_TRUE(cadv_flyer_altitude(&f, &t) >= 2.0f - 1e-3f);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_bad_sizes);
    RUN_TEST(test_islands_and_sampling);
    RUN_TEST(test_vehicle_kit__deformation_stays_in_bounds);
    RUN_TEST(test_raise_flatten_roughness);
    RUN_TEST(test_raycast_and_line_of_sight);
    RUN_TEST(test_vehicle_kit__flyer_keeps_clearance);
    RUN_TEST(test_vehicle_kit__cliffs_stop_the_flyer);
    return UNITY_END();
}
