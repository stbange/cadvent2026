/* SPDX-License-Identifier: MIT */
/* FPS kit CTF: flags, captures, generators and turrets. */
#include "kits/fps/ctf.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_fps_ctf__flag_return_timer(void)
{
    cadv_flag f;
    int i;
    cadv_flag_init(&f, 1, cadv_v3(10, 5, 10));
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_NONE, cadv_flag_touch(&f, 3, 1)); /* own team at home: nothing */
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_TAKEN, cadv_flag_touch(&f, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, f.carrier);
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_NONE, cadv_flag_touch(&f, 4, 0)); /* carried */
    cadv_flag_drop(&f, cadv_v3(50, 2, 60), cadv_v3(20, 0, 0));
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_DROPPED, f.state);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, f.vel.x);
    TEST_ASSERT_EQUAL_INT(-1, f.carrier);
    /* it lies there for 30 seconds, then goes home on its own */
    for (i = 0; i < 29 * 60; ++i) {
        TEST_ASSERT_FALSE(cadv_flag_tick(&f, 1.0f / 60.0f));
    }
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_DROPPED, f.state);
    for (i = 0; i < 61 && !cadv_flag_tick(&f, 1.0f / 60.0f); ++i) {
    }
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_HOME, f.state);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, f.pos.x);
    TEST_ASSERT_FALSE(cadv_flag_tick(&f, 1.0f));
    /* a team mate touching the dropped flag returns it at once; an enemy picks it up */
    cadv_flag_touch(&f, 0, 0);
    cadv_flag_drop(&f, cadv_v3(50, 2, 60), cadv_v3(0, 0, 0));
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_RETURNED, cadv_flag_touch(&f, 5, 1));
    cadv_flag_touch(&f, 0, 0);
    cadv_flag_drop(&f, cadv_v3(50, 2, 60), cadv_v3(0, 0, 0));
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_TAKEN, cadv_flag_touch(&f, 2, 0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, f.return_t);
}

static void test_fps_ctf__capture_needs_own_flag_home(void)
{
    cadv_flag theirs;
    cadv_flag ours;
    cadv_flag_init(&theirs, 1, cadv_v3(100, 20, 400));
    cadv_flag_init(&ours, 0, cadv_v3(100, 20, 100));
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_NONE, cadv_flag_try_capture(&theirs, &ours, cadv_v3(100, 20, 100))); /* not carried */
    cadv_flag_touch(&theirs, 0, 0);
    cadv_flag_touch(&ours, 7, 1); /* they have ours too */
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_NONE, cadv_flag_try_capture(&theirs, &ours, cadv_v3(100, 20, 100)));
    cadv_flag_return(&ours);
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_NONE, cadv_flag_try_capture(&theirs, &ours, cadv_v3(104, 20, 100))); /* too far */
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_NONE, cadv_flag_try_capture(&theirs, &ours, cadv_v3(100, 30, 100))); /* flying over */
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_CAPTURED, cadv_flag_try_capture(&theirs, &ours, cadv_v3(101, 21, 101)));
    TEST_ASSERT_EQUAL_INT(CADV_FLAG_HOME, theirs.state);
    TEST_ASSERT_EQUAL_FLOAT(400.0f, theirs.pos.z);
}

static void test_fps_ctf__generator_powers_turrets(void)
{
    cadv_generator g;
    cadv_turret t = {{0, 0, 0}, 80.0f, 2.0f, 0.0f};
    int i;
    int shots = 0;
    cadv_generator_init(&g, 300.0f, 40.0f);
    TEST_ASSERT_TRUE(cadv_generator_powered(&g));
    TEST_ASSERT_FALSE(cadv_turret_ready(&t, &g, 90.0f, 0.1f)); /* out of range */
    TEST_ASSERT_TRUE(cadv_turret_ready(&t, &g, 50.0f, 0.1f));
    TEST_ASSERT_FALSE(cadv_turret_ready(&t, &g, 50.0f, 0.1f)); /* reloading */
    for (i = 0; i < 100; ++i) {
        shots += cadv_turret_ready(&t, &g, 50.0f, 0.1f) ? 1 : 0;
    }
    TEST_ASSERT_EQUAL_INT(5, shots); /* one every two seconds */
    /* destroy the generator: the turret goes quiet */
    TEST_ASSERT_FALSE(cadv_generator_damage(&g, 200.0f));
    TEST_ASSERT_FALSE(cadv_generator_damage(&g, -5.0f));
    TEST_ASSERT_TRUE(cadv_generator_damage(&g, 200.0f));
    TEST_ASSERT_FALSE(cadv_generator_damage(&g, 10.0f)); /* already down */
    TEST_ASSERT_FALSE(cadv_generator_powered(&g));
    for (i = 0; i < 30; ++i) {
        TEST_ASSERT_FALSE(cadv_turret_ready(&t, &g, 10.0f, 1.0f));
        TEST_ASSERT_FALSE(cadv_generator_tick(&g, 1.0f));
    }
    /* repaired after its repair time: power and the turret are back */
    for (i = 0; i < 11 && !cadv_generator_tick(&g, 1.0f); ++i) {
    }
    TEST_ASSERT_TRUE(cadv_generator_powered(&g));
    TEST_ASSERT_EQUAL_FLOAT(300.0f, g.hp);
    TEST_ASSERT_FALSE(cadv_generator_tick(&g, 1.0f));
    TEST_ASSERT_TRUE(cadv_turret_ready(&t, &g, 10.0f, 1.0f));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fps_ctf__flag_return_timer);
    RUN_TEST(test_fps_ctf__capture_needs_own_flag_home);
    RUN_TEST(test_fps_ctf__generator_powers_turrets);
    return UNITY_END();
}
