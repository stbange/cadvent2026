/* SPDX-License-Identifier: MIT */
#include "sim/collide.h"
#include "unity.h"

static cadv_world w;

void setUp(void)
{
    cadv_world_clear(&w);
    cadv_world_add(&w, cadv_v3(-10, -1, -10), cadv_v3(10, 0, 10), 0); /* floor */
    cadv_world_add(&w, cadv_v3(2, 0, -10), cadv_v3(3, 3, 10), 1);     /* wall at x=2 */
    cadv_world_add(&w, cadv_v3(-3, 0, -1), cadv_v3(-2, 0.3f, 1), 2);  /* low step */
    cadv_world_add(&w, cadv_v3(-6, 0, -1), cadv_v3(-5, 1.0f, 1), 3);  /* tall block */
}
void tearDown(void) {}

static void test_aabb_helpers(void)
{
    cadv_aabb a = cadv_aabb_make(cadv_v3(1, 1, 1), cadv_v3(0, 0, 0));
    cadv_aabb b = cadv_aabb_make(cadv_v3(0.5f, 0.5f, 0.5f), cadv_v3(2, 2, 2));
    cadv_aabb c = cadv_aabb_make(cadv_v3(5, 5, 5), cadv_v3(6, 6, 6));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, a.lo.x);
    TEST_ASSERT_TRUE(cadv_aabb_overlap(a, b));
    TEST_ASSERT_FALSE(cadv_aabb_overlap(a, c));
    TEST_ASSERT_TRUE(cadv_aabb_contains(a, cadv_v3(0.5f, 0.5f, 0.5f)));
    TEST_ASSERT_FALSE(cadv_aabb_contains(a, cadv_v3(1.5f, 0.5f, 0.5f)));
}

static void test_world_capacity(void)
{
    int i;
    cadv_world big;
    cadv_world_clear(&big);
    for (i = 0; i < CADV_MAX_SOLIDS; ++i) {
        TEST_ASSERT_EQUAL_INT(i, cadv_world_add(&big, cadv_v3(0, 0, 0), cadv_v3(1, 1, 1), 0));
    }
    TEST_ASSERT_EQUAL_INT(-1, cadv_world_add(&big, cadv_v3(0, 0, 0), cadv_v3(1, 1, 1), 0));
}

static void test_raycast(void)
{
    float t;
    cadv_vec3 n;
    int s;
    TEST_ASSERT_TRUE(cadv_world_raycast(&w, cadv_v3(0, 1, 0), cadv_v3(1, 0, 0), 50, &t, &n, &s));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.0f, t);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, n.x);
    TEST_ASSERT_EQUAL_INT(1, s);
    TEST_ASSERT_TRUE(cadv_world_raycast(&w, cadv_v3(0, 1, 0), cadv_v3(0, -1, 0), 50, &t, &n, NULL));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, n.y);
    TEST_ASSERT_TRUE(cadv_world_raycast(&w, cadv_v3(2.5f, 1, -20), cadv_v3(0, 0, 1), 50, NULL, &n, NULL));
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, n.z);
    TEST_ASSERT_FALSE(cadv_world_raycast(&w, cadv_v3(0, 1, 0), cadv_v3(1, 0, 0), 1.5f, &t, NULL, NULL));
    TEST_ASSERT_FALSE(cadv_world_raycast(&w, cadv_v3(0, 5, 0), cadv_v3(0, 1, 0), 50, &t, NULL, NULL));
    w.s[1].enabled = false;
    TEST_ASSERT_FALSE(cadv_world_raycast(&w, cadv_v3(0, 1, 0), cadv_v3(1, 0, 0), 50, &t, NULL, NULL));
}

static void test_ray_aabb_inside_and_parallel(void)
{
    cadv_aabb b = cadv_aabb_make(cadv_v3(0, 0, 0), cadv_v3(1, 1, 1));
    float t;
    cadv_vec3 n;
    TEST_ASSERT_TRUE(cadv_ray_aabb(cadv_v3(0.5f, 0.5f, 0.5f), cadv_v3(1, 0, 0), b, 10, &t, &n));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, t); /* starting inside */
    TEST_ASSERT_FALSE(cadv_ray_aabb(cadv_v3(0.5f, 2, 0.5f), cadv_v3(1, 0, 0), b, 10, &t, NULL));
    TEST_ASSERT_FALSE(cadv_ray_aabb(cadv_v3(-1, 0.5f, 0.5f), cadv_v3(-1, 0, 0), b, 10, &t, NULL));
}

static void test_ray_cylinder(void)
{
    float t;
    TEST_ASSERT_TRUE(cadv_ray_cylinder(cadv_v3(0, 1, 0), cadv_v3(0, 0, -1), cadv_v3(0, 0, -5), 0.5f, 2, 20, &t));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 4.5f, t);
    TEST_ASSERT_TRUE(cadv_ray_cylinder(cadv_v3(0, 1, -5), cadv_v3(0, 0, -1), cadv_v3(0, 0, -5), 0.5f, 2, 20, &t));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.5f, t); /* from inside: exit point */
    TEST_ASSERT_FALSE(cadv_ray_cylinder(cadv_v3(0, 3, 0), cadv_v3(0, 0, -1), cadv_v3(0, 0, -5), 0.5f, 2, 20, &t));
    TEST_ASSERT_FALSE(cadv_ray_cylinder(cadv_v3(2, 1, 0), cadv_v3(0, 0, -1), cadv_v3(0, 0, -5), 0.5f, 2, 20, &t));
    TEST_ASSERT_FALSE(cadv_ray_cylinder(cadv_v3(0, 1, 0), cadv_v3(0, 0, -1), cadv_v3(0, 0, -5), 0.5f, 2, 3, &t));
    TEST_ASSERT_FALSE(cadv_ray_cylinder(cadv_v3(0, 1, 0), cadv_v3(0, 0, 1), cadv_v3(0, 0, -5), 0.5f, 2, 20, &t));
    TEST_ASSERT_FALSE(cadv_ray_cylinder(cadv_v3(0, 1, 0), cadv_v3(0, 1, 0), cadv_v3(0, 0, -5), 0.5f, 2, 20, &t));
}

static void test_line_of_sight(void)
{
    TEST_ASSERT_TRUE(cadv_world_line_of_sight(&w, cadv_v3(0, 1, 0), cadv_v3(1, 1, 0)));
    TEST_ASSERT_FALSE(cadv_world_line_of_sight(&w, cadv_v3(0, 1, 0), cadv_v3(5, 1, 0)));
    TEST_ASSERT_TRUE(cadv_world_line_of_sight(&w, cadv_v3(0, 1, 0), cadv_v3(0, 1, 0)));
}

static void test_move_blocks_slides_and_lands(void)
{
    cadv_vec3 p = cadv_v3(0, 0.5f, 0);
    cadv_move_result r = cadv_world_move(&w, &p, 0.3f, 1.7f, cadv_v3(5, -1, 1), 0.0f);
    TEST_ASSERT_TRUE(r.hit_x);
    TEST_ASSERT_FALSE(r.hit_z);
    TEST_ASSERT_TRUE(r.on_ground);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.7f, p.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, p.z);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, p.y);
    p = cadv_v3(0, 0, 0);
    r = cadv_world_move(&w, &p, 0.3f, 1.7f, cadv_v3(0, 0, -5), 0.0f); /* free move, still on floor */
    TEST_ASSERT_TRUE(r.on_ground);
}

static void test_move_steps_up_low_obstacles_only(void)
{
    cadv_vec3 p = cadv_v3(-1.5f, 0.001f, 0);
    cadv_move_result r = cadv_world_move(&w, &p, 0.3f, 1.7f, cadv_v3(-0.5f, -0.01f, 0), 0.45f);
    TEST_ASSERT_TRUE(r.stepped);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.3f, p.y);
    p = cadv_v3(-4.5f, 0.001f, 0);
    r = cadv_world_move(&w, &p, 0.3f, 1.7f, cadv_v3(-0.5f, -0.01f, 0), 0.45f);
    TEST_ASSERT_FALSE(r.stepped);
    TEST_ASSERT_TRUE(r.hit_x);
}

static void test_move_hits_ceiling(void)
{
    cadv_vec3 p;
    cadv_move_result r;
    cadv_world_add(&w, cadv_v3(-10, 2, -10), cadv_v3(10, 3, 10), 0);
    p = cadv_v3(0, 0, 0);
    r = cadv_world_move(&w, &p, 0.3f, 1.7f, cadv_v3(0, 1, 0), 0.0f);
    TEST_ASSERT_TRUE(r.hit_ceiling);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.3f, p.y);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_aabb_helpers);
    RUN_TEST(test_world_capacity);
    RUN_TEST(test_raycast);
    RUN_TEST(test_ray_aabb_inside_and_parallel);
    RUN_TEST(test_ray_cylinder);
    RUN_TEST(test_line_of_sight);
    RUN_TEST(test_move_blocks_slides_and_lands);
    RUN_TEST(test_move_steps_up_low_obstacles_only);
    RUN_TEST(test_move_hits_ceiling);
    return UNITY_END();
}
