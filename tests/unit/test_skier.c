/* SPDX-License-Identifier: MIT */
/* Vehicle kit skier: skiing, jetpack energy, armour classes. */
#include <string.h>

#include "kits/vehicle/skier.h"
#include "unity.h"

static cadv_terrain terrain;
static cadv_world world;
static const float DT = 1.0f / 60.0f;

/* flat at 10 m for x < 100, a 1:2 slope down along +x from 100 to 300, flat again */
static void slope(void)
{
    int x;
    int z;
    cadv_terrain_init(&terrain, 129, 4.0f, 0.0f, 200.0f, -10.0f);
    for (z = 0; z < 129; ++z) {
        for (x = 0; x < 129; ++x) {
            float wx = (float)x * 4.0f;
            float h = wx < 100.0f ? 110.0f : (wx < 300.0f ? 110.0f - (wx - 100.0f) * 0.5f : 10.0f);
            terrain.h[z * 129 + x] = h;
        }
    }
}

void setUp(void)
{
    slope();
    cadv_world_clear(&world);
}
void tearDown(void) {}

static void run(cadv_skier *k, const cadv_skier_input *in, int ticks)
{
    int i;
    for (i = 0; i < ticks; ++i) {
        cadv_skier_step(k, &terrain, &world, in, DT);
    }
}

static void test_vehicle_skier__ski_momentum_conservation(void)
{
    cadv_skier k;
    cadv_skier_input ski = {0, 0, true, false, false};
    cadv_skier_input walk = {0, 0, false, false, false};
    float before;
    float top;
    /* skiing on the flat keeps nearly all its speed */
    cadv_skier_init(&k, CADV_ARMOUR_LIGHT, cadv_v3(340, 10, 200), 0.0f);
    k.on_ground = true;
    k.vel = cadv_v3(0, 0, -30.0f);
    run(&k, &ski, 120);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 30.0f, cadv_skier_speed(&k));
    TEST_ASSERT_TRUE(k.skiing);
    /* letting go of ski on the flat: friction stops you */
    run(&k, &walk, 120);
    TEST_ASSERT_TRUE(cadv_skier_speed(&k) < 0.5f);
    /* down the slope the speed grows like a frictionless slide: v^2 ~ 2 g h */
    cadv_skier_init(&k, CADV_ARMOUR_LIGHT, cadv_v3(104, 108, 200), -CADV_PI * 0.5f);
    k.on_ground = true;
    before = k.pos.y;
    while (k.pos.x < 290.0f && k.pos.y > 12.0f) {
        cadv_skier_step(&k, &terrain, &world, &ski, DT);
    }
    top = cadv_skier_speed(&k);
    TEST_ASSERT_FLOAT_WITHIN(0.12f * cadv_sqrtf(2.0f * CADV_SKIER_GRAVITY * (before - k.pos.y)), cadv_sqrtf(2.0f * CADV_SKIER_GRAVITY * (before - k.pos.y)), top);
    TEST_ASSERT_TRUE(top > 41.7f); /* faster than 150 km/h */
    /* and onto the flat: the momentum carries on */
    run(&k, &ski, 30);
    TEST_ASSERT_TRUE(cadv_skier_speed(&k) > top * 0.85f);
    TEST_ASSERT_TRUE(k.pos.x > 300.0f);
}

static void test_vehicle_skier__energy_drain_and_regen(void)
{
    cadv_skier k;
    cadv_skier_input jet = {0, 0, false, true, false};
    cadv_skier_input none = {0, 0, false, false, false};
    const cadv_armour_def *a = cadv_armour_get(CADV_ARMOUR_LIGHT);
    float e0;
    cadv_skier_init(&k, CADV_ARMOUR_LIGHT, cadv_v3(340, 10, 200), 0.0f);
    k.on_ground = true;
    TEST_ASSERT_EQUAL_FLOAT(a->energy_max, k.energy);
    run(&k, &jet, 60);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, a->energy_max - a->jet_drain, k.energy);
    TEST_ASSERT_TRUE(k.pos.y > 14.0f); /* the jets lift you */
    TEST_ASSERT_FALSE(k.on_ground);
    /* empty: the jets cough out and stay off for a moment */
    {
        int i;
        for (i = 0; i < 60 * 5 && k.energy > 0.0f; ++i) {
            cadv_skier_step(&k, &terrain, &world, &jet, DT);
        }
    }
    TEST_ASSERT_EQUAL_FLOAT(0.0f, k.energy);
    TEST_ASSERT_TRUE(k.jet_lock > 0.5f);
    cadv_skier_step(&k, &terrain, &world, &jet, DT);
    TEST_ASSERT_FALSE(k.jetting);
    /* regenerating; the energy pack is faster */
    run(&k, &none, 60 * 3);
    e0 = k.energy;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, a->regen * 3.0f, e0);
    k.energy = 0.0f;
    k.energy_pack = true;
    run(&k, &none, 60);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, a->regen * 1.5f, k.energy);
    run(&k, &none, 60 * 20);
    TEST_ASSERT_EQUAL_FLOAT(a->energy_max, k.energy); /* never above full */
}

static void test_vehicle_skier__armour_classes(void)
{
    cadv_skier k;
    cadv_skier_input run_fwd = {0, 1, false, false, false};
    float light_speed;
    TEST_ASSERT_TRUE(cadv_armour_get(CADV_ARMOUR_LIGHT)->run_speed > cadv_armour_get(CADV_ARMOUR_HEAVY)->run_speed);
    TEST_ASSERT_TRUE(cadv_armour_get(CADV_ARMOUR_HEAVY)->hp > cadv_armour_get(CADV_ARMOUR_MEDIUM)->hp);
    TEST_ASSERT_EQUAL_INT(2, cadv_armour_get(CADV_ARMOUR_LIGHT)->slots);
    TEST_ASSERT_EQUAL_STRING("LIGHT", cadv_armour_get((cadv_armour_class)9)->name);
    cadv_skier_init(&k, CADV_ARMOUR_LIGHT, cadv_v3(340, 10, 200), 0.0f);
    k.on_ground = true;
    run(&k, &run_fwd, 60);
    light_speed = cadv_skier_speed(&k);
    TEST_ASSERT_FLOAT_WITHIN(0.3f, 11.0f, light_speed);
    /* swapping armour keeps the health fraction */
    k.hp = 35.0f;
    cadv_skier_set_armour(&k, CADV_ARMOUR_HEAVY);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, k.hp);
    run(&k, &run_fwd, 60);
    TEST_ASSERT_FLOAT_WITHIN(0.3f, 7.0f, cadv_skier_speed(&k));
    /* heavier armour is pushed less, and a jump leaves the ground */
    k.vel = cadv_v3(0, 0, 0);
    cadv_skier_push(&k, cadv_v3(22.0f, 4.4f, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, k.vel.x);
    TEST_ASSERT_FALSE(k.on_ground);
    run(&k, &run_fwd, 120);
    TEST_ASSERT_TRUE(k.on_ground);
    {
        cadv_skier_input jump = {0, 0, false, false, true};
        cadv_skier_step(&k, &terrain, &world, &jump, DT);
        TEST_ASSERT_FALSE(k.on_ground);
        TEST_ASSERT_TRUE(k.airtime > 0.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, k.pos.y + 1.7f, cadv_skier_eye(&k).y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, cadv_skier_forward(&k).z);
}

static void test_vehicle_skier__solids_block_and_carry(void)
{
    cadv_skier k;
    cadv_skier_input fwd = {0, 1, false, false, false};
    /* a bunker wall ahead and a platform to land on */
    cadv_world_add(&world, cadv_v3(330, 0, 180), cadv_v3(350, 20, 182), 0);
    cadv_world_add(&world, cadv_v3(360, 0, 200), cadv_v3(380, 14, 220), 0);
    cadv_skier_init(&k, CADV_ARMOUR_MEDIUM, cadv_v3(340, 10, 190), 0.0f);
    k.on_ground = true;
    run(&k, &fwd, 120);
    TEST_ASSERT_TRUE(k.pos.z > 182.0f); /* the wall stops the walk */
    k.pos = cadv_v3(370, 20, 210);
    k.vel = cadv_v3(0, -5, 0);
    k.on_ground = false;
    run(&k, &fwd, 60);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 14.0f, k.pos.y); /* standing on the platform */
    TEST_ASSERT_TRUE(k.on_ground);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_vehicle_skier__ski_momentum_conservation);
    RUN_TEST(test_vehicle_skier__energy_drain_and_regen);
    RUN_TEST(test_vehicle_skier__armour_classes);
    RUN_TEST(test_vehicle_skier__solids_block_and_carry);
    return UNITY_END();
}
