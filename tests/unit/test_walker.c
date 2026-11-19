/* SPDX-License-Identifier: MIT */
/* Vehicle kit: the walker (legs and torso, throttle, heat, components, jets). */
#include <string.h>

#include "kits/vehicle/walker.h"
#include "unity.h"

static cadv_terrain t;
static cadv_walker_def d;
static cadv_walker w;
static cadv_walker_ctl c;

void setUp(void)
{
    int i;
    TEST_ASSERT_TRUE(cadv_terrain_init(&t, 33, 4.0f, 0.0f, 60.0f, -5.0f));
    for (i = 0; i < 33 * 33; ++i) {
        t.h[i] = 10.0f;
    }
    memset(&d, 0, sizeof d);
    d.max_speed = 12.0f;
    d.reverse = 0.5f;
    d.accel = 6.0f;
    d.throttle_rate = 1.0f;
    d.turn_rate = 1.0f;
    d.twist_limit = 1.5f;
    d.twist_rate = 2.0f;
    d.height = 9.0f;
    d.max_climb = 0.8f;
    d.jump_thrust = 20.0f;
    d.jump_fuel = 2.0f;
    d.jump_heat = 30.0f;
    d.gravity = 9.8f;
    d.heat_cap = 100.0f;
    d.heat_shed = 10.0f;
    d.restart_heat = 40.0f;
    for (i = 0; i < CADV_WZ_COUNT; ++i) {
        d.armour[i] = 40;
    }
    cadv_walker_spawn(&w, &d, cadv_v3(64, 10, 64), 0.0f);
    memset(&c, 0, sizeof c);
}
void tearDown(void) {}

static void run(float seconds, float cooling)
{
    int i;
    for (i = 0; i < (int)(seconds * 60.0f); ++i) {
        cadv_walker_tick(&w, &d, &c, &t, cooling, 1.0f / 60.0f);
    }
}

static void test_vehicle_kit__throttle_and_reverse(void)
{
    c.throttle = 1.0f;
    run(1.0f, 1.0f);
    c.throttle = 0.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 1.0f, w.throttle);
    run(4.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, w.speed);
    TEST_ASSERT_TRUE(w.pos.z < 30.0f); /* walked north */
    c.stop = true;
    run(3.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, w.speed);
    c.stop = false;
    c.throttle = -1.0f;
    run(2.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -6.0f, w.speed); /* reverse is half speed */
}

static void test_vehicle_kit__torso_twist_is_independent(void)
{
    c.turn = 1.0f;
    run(0.5f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, -0.5f, w.legs_yaw); /* legs turned right */
    c.turn = 0.0f;
    c.twist = 0.5f;
    run(2.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(1.5f, w.twist); /* clamped at the limit */
    TEST_ASSERT_FLOAT_WITHIN(0.02f, -0.5f, w.legs_yaw);
    TEST_ASSERT_FLOAT_WITHIN(0.03f, 1.0f, cadv_walker_torso_yaw(&w));
    c.twist = 0.0f;
    c.centre = true;
    run(1.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, w.twist);
    c.centre = false;
    c.pitch = 1.0f;
    run(0.1f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, w.pitch);
    TEST_ASSERT_TRUE(cadv_walker_aim(&w).y > 0.4f);
    TEST_ASSERT_EQUAL_FLOAT(19.0f, cadv_walker_eye(&w, &d).y);
}

static void test_vehicle_kit__overheating_shuts_down(void)
{
    cadv_walker_add_heat(&w, &d, 60.0f);
    TEST_ASSERT_FALSE(w.shutdown);
    cadv_walker_add_heat(&w, &d, 45.0f);
    TEST_ASSERT_TRUE(w.shutdown);
    TEST_ASSERT_EQUAL_INT(1, w.shutdowns);
    c.throttle = 1.0f;
    run(1.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, w.throttle); /* no control while shut down */
    TEST_ASSERT_TRUE(w.shutdown);
    run(7.0f, 1.0f);
    TEST_ASSERT_FALSE(w.shutdown); /* cooled below the restart heat */
    TEST_ASSERT_TRUE(w.heat < 40.0f);
    /* cooling helps: water, vents, coolant */
    w.heat = 50.0f;
    c.throttle = 0.0f;
    run(1.0f, 3.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.6f, 20.0f, w.heat);
    cadv_walker_add_heat(&w, &d, 1000.0f);
    TEST_ASSERT_EQUAL_FLOAT(150.0f, w.heat);
}

static void test_vehicle_kit__component_damage(void)
{
    cadv_vec3 front;
    TEST_ASSERT_FALSE(cadv_walker_damage(&w, CADV_WZ_LA, 0));
    TEST_ASSERT_FALSE(cadv_walker_damage(&w, CADV_WZ_LA, 30));
    TEST_ASSERT_TRUE(cadv_walker_has(&w, CADV_WZ_LA));
    TEST_ASSERT_FALSE(cadv_walker_damage(&w, CADV_WZ_LA, 30)); /* arm gone, 20 to the left torso */
    TEST_ASSERT_FALSE(cadv_walker_has(&w, CADV_WZ_LA));
    TEST_ASSERT_EQUAL_INT(20, w.hp[CADV_WZ_LT]);
    TEST_ASSERT_EQUAL_INT(1, w.lost);
    cadv_walker_damage(&w, CADV_WZ_LA, 5); /* hits on a lost arm land on its torso */
    TEST_ASSERT_EQUAL_INT(15, w.hp[CADV_WZ_LT]);
    /* the right torso takes its arm with it */
    cadv_walker_damage(&w, CADV_WZ_RT, 50);
    TEST_ASSERT_FALSE(cadv_walker_has(&w, CADV_WZ_RA));
    TEST_ASSERT_EQUAL_INT(3, w.lost);
    cadv_walker_damage(&w, CADV_WZ_LT, 20); /* 5 more pass on to the centre torso */
    TEST_ASSERT_EQUAL_INT(4, w.lost);
    TEST_ASSERT_EQUAL_INT(25, w.hp[CADV_WZ_CT]);
    TEST_ASSERT_TRUE(cadv_walker_alive(&w));
    /* a hurt leg slows the walk, a lost one more */
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cadv_walker_speed_factor(&w, &d));
    cadv_walker_damage(&w, CADV_WZ_LL, 20);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, cadv_walker_speed_factor(&w, &d));
    cadv_walker_damage(&w, CADV_WZ_LL, 20);
    TEST_ASSERT_EQUAL_FLOAT(0.35f, cadv_walker_speed_factor(&w, &d));
    TEST_ASSERT_TRUE(cadv_walker_alive(&w));
    TEST_ASSERT_TRUE(cadv_walker_damage(&w, CADV_WZ_CT, 999));
    TEST_ASSERT_FALSE(cadv_walker_alive(&w));
    TEST_ASSERT_FALSE(cadv_walker_damage(&w, CADV_WZ_HEAD, 5)); /* already destroyed */
    c.throttle = 1.0f;
    run(1.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, w.speed);
    /* hit zones, seen from the front (north of a walker facing north) */
    cadv_walker_spawn(&w, &d, cadv_v3(64, 10, 64), 0.0f);
    front = cadv_v3(64, 10 + 8.8f, 60);
    TEST_ASSERT_EQUAL_INT(CADV_WZ_HEAD, cadv_walker_hit_zone(&w, &d, front));
    TEST_ASSERT_EQUAL_INT(CADV_WZ_CT, cadv_walker_hit_zone(&w, &d, cadv_v3(64, 16, 60)));
    TEST_ASSERT_EQUAL_INT(CADV_WZ_RT, cadv_walker_hit_zone(&w, &d, cadv_v3(65, 16, 60)));
    TEST_ASSERT_EQUAL_INT(CADV_WZ_LT, cadv_walker_hit_zone(&w, &d, cadv_v3(63, 16, 60)));
    TEST_ASSERT_EQUAL_INT(CADV_WZ_RA, cadv_walker_hit_zone(&w, &d, cadv_v3(66.5f, 16, 60)));
    TEST_ASSERT_EQUAL_INT(CADV_WZ_LA, cadv_walker_hit_zone(&w, &d, cadv_v3(61.5f, 16, 60)));
    TEST_ASSERT_EQUAL_INT(CADV_WZ_RL, cadv_walker_hit_zone(&w, &d, cadv_v3(65, 12, 60)));
    TEST_ASSERT_EQUAL_INT(CADV_WZ_LL, cadv_walker_hit_zone(&w, &d, cadv_v3(63, 12, 60)));
    TEST_ASSERT_EQUAL_STRING("LEFT ARM", cadv_walker_zone_name(CADV_WZ_LA));
    TEST_ASSERT_EQUAL_STRING("?", cadv_walker_zone_name(CADV_WZ_COUNT));
    /* both legs gone */
    cadv_walker_damage(&w, CADV_WZ_LL, 40);
    TEST_ASSERT_TRUE(cadv_walker_damage(&w, CADV_WZ_RL, 40));
}

static void test_vehicle_kit__slopes_and_jump_jets(void)
{
    int x;
    int z;
    /* a cliff north of the walker */
    for (z = 0; z < 13; ++z) {
        for (x = 0; x < 33; ++x) {
            t.h[z * 33 + x] = 40.0f;
        }
    }
    c.throttle = 1.0f;
    run(8.0f, 1.0f);
    TEST_ASSERT_TRUE(w.blocked || w.speed < 5.0f);
    TEST_ASSERT_TRUE(w.pos.z > 48.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, w.pos.y);
    /* jets lift it, heat it and burn fuel; landing on the shelf */
    c.jump = true;
    run(1.5f, 1.0f);
    c.jump = false;
    TEST_ASSERT_TRUE(w.pos.y > 20.0f);
    TEST_ASSERT_TRUE(w.heat > 5.0f);
    TEST_ASSERT_TRUE(w.fuel < 1.0f);
    run(6.0f, 1.0f);
    TEST_ASSERT_TRUE(w.on_ground);
    run(4.0f, 1.0f);
    TEST_ASSERT_TRUE(w.fuel > 1.5f); /* refuelled on the ground */
    /* walking down a gentle slope keeps the feet on it */
    for (z = 0; z < 33; ++z) {
        for (x = 0; x < 33; ++x) {
            t.h[z * 33 + x] = 10.0f + (float)z * 0.5f;
        }
    }
    cadv_walker_spawn(&w, &d, cadv_v3(64, cadv_terrain_height(&t, 64, 20), 20), 3.14159f);
    c.throttle = 1.0f;
    run(3.0f, 1.0f);
    TEST_ASSERT_TRUE(w.on_ground);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, cadv_terrain_height(&t, w.pos.x, w.pos.z), w.pos.y);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_vehicle_kit__throttle_and_reverse);
    RUN_TEST(test_vehicle_kit__torso_twist_is_independent);
    RUN_TEST(test_vehicle_kit__overheating_shuts_down);
    RUN_TEST(test_vehicle_kit__component_damage);
    RUN_TEST(test_vehicle_kit__slopes_and_jump_jets);
    return UNITY_END();
}
