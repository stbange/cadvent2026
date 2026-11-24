/* SPDX-License-Identifier: MIT */
/* FPS kit destructible facades. */
#include "kits/fps/facade.h"
#include "unity.h"

static cadv_world world;
static cadv_facade f;

void setUp(void) { cadv_world_clear(&world); }
void tearDown(void) {}

static void test_fps_facade__voxel_destruction_chunking(void)
{
    cadv_vec3 c;
    int gone;
    int solids;
    TEST_ASSERT_FALSE(cadv_facade_build(&f, &world, cadv_v3(0, 0, 0), 0, 13, 2, 1.0f, 0.5f, 100));
    TEST_ASSERT_TRUE(cadv_facade_build(&f, &world, cadv_v3(10, 0, 5), 0, 6, 4, 1.5f, 0.6f, 100));
    solids = world.count;
    TEST_ASSERT_EQUAL_INT(24, solids);
    TEST_ASSERT_EQUAL_INT(24, f.alive);
    c = cadv_facade_centre(&f, 2, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 13.75f, c.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.3f, c.z);
    /* a ray hits the wall... */
    TEST_ASSERT_FALSE(cadv_world_line_of_sight(&world, cadv_v3(13.7f, 0.7f, 0), cadv_v3(13.7f, 0.7f, 10)));
    /* a weak blast only chips */
    TEST_ASSERT_EQUAL_INT(0, cadv_facade_blast(&f, &world, c, 1.0f, 40));
    TEST_ASSERT_EQUAL_INT(0u, f.ver);
    TEST_ASSERT_EQUAL_INT(60, f.hp[2]);
    /* a strong one knocks a hole: the chunk and its neighbours go, the ray passes */
    gone = cadv_facade_blast(&f, &world, c, 2.0f, 200);
    TEST_ASSERT_EQUAL_INT(4, gone); /* the chunk and its three neighbours */
    TEST_ASSERT_FALSE(cadv_facade_alive(&f, 2, 0));
    TEST_ASSERT_TRUE(cadv_facade_alive(&f, 5, 3));
    TEST_ASSERT_FALSE(cadv_facade_alive(&f, -1, 0));
    TEST_ASSERT_EQUAL_INT(1u, f.ver);
    TEST_ASSERT_TRUE(cadv_world_line_of_sight(&world, cadv_v3(13.7f, 0.7f, 0), cadv_v3(13.7f, 0.7f, 10)));
    TEST_ASSERT_EQUAL_INT(24 - gone, f.alive);
    /* cut a column out of the middle on a narrow wall: the loose top falls too */
    TEST_ASSERT_TRUE(cadv_facade_build(&f, &world, cadv_v3(0, 0, 20), 1, 1, 3, 1.0f, 0.5f, 50));
    gone = cadv_facade_blast(&f, &world, cadv_facade_centre(&f, 0, 0), 0.6f, 200);
    TEST_ASSERT_EQUAL_INT(3, gone); /* the bottom, then the two hanging above it */
    TEST_ASSERT_EQUAL_INT(0, f.alive);
    TEST_ASSERT_FALSE(world.s[f.solid[2]].enabled);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fps_facade__voxel_destruction_chunking);
    return UNITY_END();
}
