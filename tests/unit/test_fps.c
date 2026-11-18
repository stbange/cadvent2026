/* SPDX-License-Identifier: MIT */
#include <string.h>

#include "kits/fps/combat.h"
#include "unity.h"

#define DT (1.0f / 60.0f)

static cadv_fps f;
static cadv_input_frame in;

static void arena(void)
{
    cadv_fps_init(&f, 1);
    cadv_world_add(&f.world, cadv_v3(-20, -1, -20), cadv_v3(20, 0, 20), 0);
    cadv_world_add(&f.world, cadv_v3(-20, 0, -21), cadv_v3(20, 4, -20), 0);
    cadv_player_spawn(&f.player, cadv_v3(0, 0, 0), 0.0f);
}

void setUp(void)
{
    memset(&in, 0, sizeof in);
    arena();
}
void tearDown(void) {}

static void run(int ticks)
{
    int i;
    for (i = 0; i < ticks; ++i) {
        cadv_fps_tick(&f, &in, DT);
        in.pressed = 0;
    }
}

static void test_player_walks_runs_jumps_and_lands(void)
{
    uint32_t ev = 0;
    int i;
    in.move_y = 1.0f;
    run(60);
    TEST_ASSERT_TRUE(f.player.pos.z < -3.5f); /* walked forward (-Z) */
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    {
        float z = f.player.pos.z;
        run(30);
        TEST_ASSERT_TRUE(z - f.player.pos.z > 2.5f);
    }
    in.move_y = 0.0f;
    in.held = 0;
    run(60);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, f.player.vel.z); /* friction stops */
    in.pressed = CADV_ACT_BIT(CADV_ACT_JUMP);
    in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    ev = cadv_player_tick(&f.player, &f.world, &in, DT);
    TEST_ASSERT_TRUE(ev & CADV_PEV_JUMP);
    in.pressed = 0;
    for (i = 0; i < 120 && !(ev & CADV_PEV_LAND); ++i) {
        ev = cadv_player_tick(&f.player, &f.world, &in, DT);
    }
    TEST_ASSERT_TRUE(ev & CADV_PEV_LAND);
    TEST_ASSERT_TRUE(f.player.on_ground);
}

static void test_footsteps_and_look_limits(void)
{
    int i;
    int steps = 0;
    in.move_x = 1.0f;
    in.look_y = 10.0f;
    for (i = 0; i < 120; ++i) {
        steps += (cadv_player_tick(&f.player, &f.world, &in, DT) & CADV_PEV_STEP) != 0;
    }
    TEST_ASSERT_TRUE(steps >= 2);
    TEST_ASSERT_EQUAL_FLOAT(f.player.tune.max_pitch, f.player.pitch);
    TEST_ASSERT_TRUE(f.player.pos.x > 3.0f); /* strafed right (+X when facing -Z) */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, cadv_player_forward(&f.player).x);
}

static void test_crouch_needs_headroom(void)
{
    in.held = CADV_ACT_BIT(CADV_ACT_CROUCH);
    run(1);
    TEST_ASSERT_TRUE(f.player.crouching);
    TEST_ASSERT_TRUE(cadv_player_eye(&f.player).y < 1.0f);
    cadv_world_add(&f.world, cadv_v3(-1, 1.2f, -1), cadv_v3(1, 2, 1), 0); /* low ceiling */
    in.held = 0;
    run(1);
    TEST_ASSERT_TRUE(f.player.crouching);
    f.world.count--; /* remove ceiling */
    run(1);
    TEST_ASSERT_FALSE(f.player.crouching);
    in.held = CADV_ACT_BIT(CADV_ACT_CROUCH);
    in.pressed = CADV_ACT_BIT(CADV_ACT_JUMP);
    cadv_player_tick(&f.player, &f.world, &in, DT);
    TEST_ASSERT_TRUE(f.player.vel.y <= 0.0f); /* no jumping while crouched */
}

static void test_damage_and_armor(void)
{
    f.player.armor = 10;
    TEST_ASSERT_FALSE(cadv_player_damage(&f.player, 30)); /* armor takes 10 */
    TEST_ASSERT_EQUAL_INT(80, f.player.health);
    TEST_ASSERT_EQUAL_INT(0, f.player.armor);
    TEST_ASSERT_FALSE(cadv_player_damage(&f.player, 0));
    TEST_ASSERT_TRUE(cadv_player_damage(&f.player, 500));
    TEST_ASSERT_EQUAL_INT(0, f.player.health);
    TEST_ASSERT_TRUE(cadv_player_damage(&f.player, 5));
}

static void test_defs(void)
{
    TEST_ASSERT_NOT_NULL(cadv_weapon_def_get(0));
    TEST_ASSERT_NULL(cadv_weapon_def_get(CADV_FPS_WEAPONS));
    TEST_ASSERT_NULL(cadv_weapon_def_get(-1));
    TEST_ASSERT_NOT_NULL(cadv_enemy_def_get(2));
    TEST_ASSERT_NULL(cadv_enemy_def_get(3));
    TEST_ASSERT_NULL(cadv_enemy_def_get(-1));
    TEST_ASSERT_EQUAL_INT(-1, cadv_fps_add_enemy(&f, 9, cadv_v3(0, 0, 0), 0));
}

static void test_hitscan_kills_enemy(void)
{
    int e = cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, -8), 0);
    int shots = 0;
    TEST_ASSERT_EQUAL_INT(1, cadv_fps_alive_enemies(&f));
    while (f.enemies[e].state != CADV_ENEMY_DEAD && shots < 20) {
        cadv_fps_fire(&f);
        ++shots;
    }
    TEST_ASSERT_EQUAL_INT(CADV_ENEMY_DEAD, f.enemies[e].state);
    TEST_ASSERT_EQUAL_INT(1, f.kills);
    TEST_ASSERT_EQUAL_INT(0, cadv_fps_alive_enemies(&f));
    TEST_ASSERT_TRUE(shots >= 3);
}

static void test_walls_block_shots(void)
{
    int e = cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, -8), 0);
    cadv_world_add(&f.world, cadv_v3(-2, 0, -5), cadv_v3(2, 3, -4), 0);
    cadv_fps_fire(&f);
    TEST_ASSERT_EQUAL_INT(cadv_enemy_def_get(0)->health, f.enemies[e].health);
}

static void test_scattershot_needs_ammo_and_melee_range(void)
{
    int e = cadv_fps_add_enemy(&f, 1, cadv_v3(0, 0, -1.5f), 0);
    f.weapon = 2;
    TEST_ASSERT_EQUAL_UINT32(0, cadv_fps_fire(&f));
    f.ammo[1] = 2;
    TEST_ASSERT_TRUE(cadv_fps_fire(&f) & CADV_FPS_EV_ENEMY_HIT);
    TEST_ASSERT_EQUAL_INT(1, f.ammo[1]);
    f.weapon = 0; /* melee */
    {
        int guard = 0;
        while (f.enemies[e].state != CADV_ENEMY_DEAD && guard++ < 50) {
            cadv_fps_fire(&f);
        }
    }
    TEST_ASSERT_EQUAL_INT(1, f.kills);
}

static void test_weapon_selection(void)
{
    in.pressed = CADV_ACT_BIT(CADV_ACT_WEAPON_1);
    run(1);
    TEST_ASSERT_EQUAL_INT(0, f.weapon);
    in.pressed = CADV_ACT_BIT(CADV_ACT_WEAPON_3); /* not owned */
    run(1);
    TEST_ASSERT_EQUAL_INT(0, f.weapon);
    in.pressed = CADV_ACT_BIT(CADV_ACT_NEXT_WEAPON);
    run(1);
    TEST_ASSERT_EQUAL_INT(1, f.weapon);
    in.pressed = CADV_ACT_BIT(CADV_ACT_PREV_WEAPON);
    run(1);
    TEST_ASSERT_EQUAL_INT(0, f.weapon);
}

static void test_fire_input_and_cooldown(void)
{
    int e = cadv_fps_add_enemy(&f, 2, cadv_v3(0, 0, -6), 0);
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    run(1);
    TEST_ASSERT_TRUE(f.enemies[e].health < cadv_enemy_def_get(2)->health);
    TEST_ASSERT_TRUE(f.cooldown > 0.0f);
    TEST_ASSERT_TRUE(f.muzzle_flash > 0.0f);
}

static void test_enemy_sees_chases_and_attacks(void)
{
    int e = cadv_fps_add_enemy(&f, 1, cadv_v3(0, 0, -10), 0);
    int i;
    int hurt = 0;
    for (i = 0; i < 600 && !hurt; ++i) {
        hurt = (cadv_fps_tick(&f, &in, DT) & CADV_FPS_EV_PLAYER_HURT) != 0;
    }
    TEST_ASSERT_TRUE(hurt);
    TEST_ASSERT_TRUE(f.enemies[e].pos.z > -3.0f);
    TEST_ASSERT_TRUE(f.player.health < 100);
}

static void test_enemy_stays_idle_without_sight(void)
{
    int e;
    cadv_world_add(&f.world, cadv_v3(-20, 0, -6), cadv_v3(20, 4, -5), 0);
    e = cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, -10), 0);
    run(120);
    TEST_ASSERT_EQUAL_INT(CADV_ENEMY_IDLE, f.enemies[e].state);
}

static void test_projectiles_hurt_and_explode(void)
{
    int i;
    uint32_t ev = 0;
    cadv_fps_add_enemy(&f, 2, cadv_v3(0, 0, -12), 0); /* lobber throws projectiles */
    for (i = 0; i < 900 && !(ev & CADV_FPS_EV_PLAYER_HURT); ++i) {
        ev = cadv_fps_tick(&f, &in, DT);
    }
    TEST_ASSERT_TRUE(ev & CADV_FPS_EV_PLAYER_HURT);
    TEST_ASSERT_TRUE(ev & CADV_FPS_EV_EXPLOSION);
}

static void test_player_launcher_splash(void)
{
    int e1 = cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, -10), 0);
    int e2 = cadv_fps_add_enemy(&f, 0, cadv_v3(1.0f, 0, -10), 0);
    int i;
    uint32_t ev = 0;
    f.owned[3] = true;
    f.weapon = 3;
    f.ammo[2] = 5;
    cadv_fps_fire(&f);
    for (i = 0; i < 120 && !(ev & CADV_FPS_EV_EXPLOSION); ++i) {
        ev |= cadv_fps_tick(&f, &in, DT);
    }
    TEST_ASSERT_TRUE(ev & CADV_FPS_EV_EXPLOSION);
    TEST_ASSERT_TRUE(f.enemies[e1].health < 40);
    TEST_ASSERT_TRUE(f.enemies[e2].health < 40);
}

static void test_projectile_hits_wall_and_expires(void)
{
    int i;
    f.owned[3] = true;
    f.weapon = 3;
    f.ammo[2] = 2;
    f.player.yaw = CADV_PI; /* face +Z: open range, projectile expires */
    cadv_fps_fire(&f);
    f.player.yaw = 0.0f;    /* face the wall at z=-20 */
    cadv_fps_fire(&f);
    for (i = 0; i < 400; ++i) {
        cadv_fps_tick(&f, &in, DT);
    }
    for (i = 0; i < CADV_FPS_MAX_PROJECTILES; ++i) {
        TEST_ASSERT_FALSE(f.proj[i].active);
    }
}

static void test_pickups(void)
{
    f.player.health = 50;
    cadv_fps_add_pickup(&f, CADV_PICKUP_HEALTH, 25, cadv_v3(0, 0.5f, -0.5f));
    cadv_fps_add_pickup(&f, CADV_PICKUP_ARMOR, 50, cadv_v3(0, 0.5f, 0.5f));
    cadv_fps_add_pickup(&f, CADV_PICKUP_AMMO, 20, cadv_v3(0.5f, 0.5f, 0));
    cadv_fps_add_pickup(&f, CADV_PICKUP_KEY, 2, cadv_v3(-0.5f, 0.5f, 0));
    cadv_fps_add_pickup(&f, CADV_PICKUP_WEAPON, 3, cadv_v3(0, 1, 0));
    cadv_fps_add_pickup(&f, CADV_PICKUP_WEAPON, 99, cadv_v3(0, 1, 0.1f));
    cadv_fps_add_pickup(&f, CADV_PICKUP_STAR, 0, cadv_v3(0, 1.2f, 0));
    cadv_fps_add_pickup(&f, CADV_PICKUP_HEALTH, 25, cadv_v3(10, 1, 10)); /* far away */
    TEST_ASSERT_TRUE(cadv_fps_tick(&f, &in, DT) & CADV_FPS_EV_STAR);
    TEST_ASSERT_EQUAL_INT(75, f.player.health);
    TEST_ASSERT_EQUAL_INT(50, f.player.armor);
    TEST_ASSERT_EQUAL_INT(20, f.ammo[1]);
    TEST_ASSERT_EQUAL_INT(10, f.ammo[2]);
    TEST_ASSERT_EQUAL_UINT32(2, f.player.keys);
    TEST_ASSERT_TRUE(f.owned[3]);
    TEST_ASSERT_TRUE(f.star_taken);
    TEST_ASSERT_TRUE(f.pickups[7].active);
    f.player.health = 100;
    cadv_fps_add_pickup(&f, CADV_PICKUP_HEALTH, 25, cadv_v3(0, 0.5f, 0));
    cadv_fps_tick(&f, &in, DT);
    TEST_ASSERT_TRUE(f.pickups[8].active); /* not needed at full health */
}

static void test_pickup_capacity(void)
{
    int i;
    for (i = 0; i < CADV_FPS_MAX_PICKUPS; ++i) {
        cadv_fps_add_pickup(&f, CADV_PICKUP_AMMO, 1, cadv_v3(15, 0, 15));
    }
    TEST_ASSERT_EQUAL_INT(-1, cadv_fps_add_pickup(&f, CADV_PICKUP_AMMO, 1, cadv_v3(0, 0, 0)));
    for (i = f.enemy_count; i < CADV_FPS_MAX_ENEMIES; ++i) {
        cadv_fps_add_enemy(&f, 0, cadv_v3(15, 0, 15), 0);
    }
    TEST_ASSERT_EQUAL_INT(-1, cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, 0), 0));
}

static void test_doors_and_keys(void)
{
    int d = cadv_fps_add_door(&f, cadv_v3(-1, 0, -2.5f), cadv_v3(1, 2.5f, -2), 4);
    TEST_ASSERT_EQUAL_INT(0, d);
    TEST_ASSERT_EQUAL_UINT32(CADV_FPS_EV_DOOR_LOCKED, cadv_fps_use(&f));
    f.player.keys = 4;
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    TEST_ASSERT_TRUE(cadv_fps_tick(&f, &in, DT) & CADV_FPS_EV_DOOR);
    in.pressed = 0;
    TEST_ASSERT_EQUAL_UINT32(0, cadv_fps_use(&f)); /* already opening */
    run(60);
    TEST_ASSERT_FALSE(f.world.s[f.doors[d].solid].enabled);
    TEST_ASSERT_EQUAL_UINT32(0, cadv_fps_use(&f)); /* nothing to use */
    f.player.yaw = CADV_PI;
    TEST_ASSERT_EQUAL_UINT32(0, cadv_fps_use(&f));
}

static void test_door_capacity_and_non_door_use(void)
{
    int i;
    int wall = cadv_world_add(&f.world, cadv_v3(-1, 0, -2.5f), cadv_v3(1, 2.5f, -2), 0); /* plain wall */
    TEST_ASSERT_EQUAL_UINT32(CADV_FPS_EV_USE_SOLID, cadv_fps_use(&f));
    TEST_ASSERT_EQUAL_INT(wall, f.use_solid);
    for (i = 0; i < CADV_FPS_MAX_DOORS; ++i) {
        TEST_ASSERT_EQUAL_INT(i, cadv_fps_add_door(&f, cadv_v3(10, 0, 10), cadv_v3(11, 1, 11), 0));
    }
    TEST_ASSERT_EQUAL_INT(-1, cadv_fps_add_door(&f, cadv_v3(10, 0, 10), cadv_v3(11, 1, 11), 0));
    f.door_count = 0;
    f.world.count = CADV_MAX_SOLIDS;
    TEST_ASSERT_EQUAL_INT(-1, cadv_fps_add_door(&f, cadv_v3(10, 0, 10), cadv_v3(11, 1, 11), 0));
}

static void test_dead_player_stops_acting(void)
{
    int e = cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, -8), 0);
    f.player.health = 0;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    run(30);
    TEST_ASSERT_EQUAL_INT(cadv_enemy_def_get(0)->health, f.enemies[e].health);
    f.damage_scale = 0.0f;
    f.player.health = 10;
    TEST_ASSERT_TRUE(cadv_player_damage(&f.player, 10)); /* sanity for scale path */
}

static const cadv_weapon_def k_test_weapons[6] = {
    {"Cane", CADV_WK_MELEE, 25, 1, 0, 0.4f, 1.8f, 0, 0, 0, 0},
    {"Pistol", CADV_WK_HITSCAN, 14, 1, 0, 0.3f, 60, 0, 0, 0, 0},
    {"Bomb", CADV_WK_REMOTE, 80, 1, 0, 0.5f, 30, 2, 1, 8.0f, 3.0f},
    {"Shrinker", CADV_WK_SHRINK, 5, 1, 0, 0.5f, 40, 3, 1, 0, 0},
    {"Chaingun", CADV_WK_HITSCAN, 9, 1, 0.03f, 0.08f, 50, 0, 1, 0, 0},
    {"Launcher", CADV_WK_PROJECTILE, 60, 1, 0, 1.0f, 60, 2, 1, 18.0f, 2.5f},
};
static const cadv_enemy_def k_test_enemies[2] = {
    {"Hog", 40, 2.0f, 0.4f, 1.6f, 20, 12, 1.4f, 5, false, 0.0f, true},
    {"Drone", 30, 3.0f, 0.3f, 0.6f, 20, 10, 1.4f, 4, false, 2.0f, false},
};

static void custom_tables(void)
{
    cadv_fps_set_tables(&f, k_test_weapons, 6, k_test_enemies, 2);
    TEST_ASSERT_EQUAL_STRING("Bomb", cadv_fps_weapon(&f, 2)->name);
    TEST_ASSERT_NULL(cadv_fps_weapon(&f, 6));
    TEST_ASSERT_NULL(cadv_fps_enemy_def(&f, 2));
}

static void test_remote_bombs_stick_and_detonate(void)
{
    int e;
    int i;
    uint32_t ev = 0;
    custom_tables();
    e = cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, -6), 0);
    f.owned[2] = true;
    f.weapon = 2;
    f.ammo[2] = 2;
    f.player.pitch = -0.3f;
    TEST_ASSERT_TRUE(cadv_fps_fire(&f) & CADV_FPS_EV_SHOT);
    for (i = 0; i < 120; ++i) {
        cadv_fps_tick(&f, &in, DT);
    }
    TEST_ASSERT_TRUE(f.proj[0].active && f.proj[0].stuck);
    f.enemies[e].pos = cadv_v3(f.proj[0].pos.x, 0, f.proj[0].pos.z); /* enemy walks over the bomb */
    in.pressed = CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
    ev = cadv_fps_tick(&f, &in, DT);
    TEST_ASSERT_TRUE(ev & CADV_FPS_EV_EXPLOSION);
    TEST_ASSERT_EQUAL_INT(CADV_ENEMY_DEAD, f.enemies[e].state);
    TEST_ASSERT_EQUAL_INT(e, f.last_killed);
    TEST_ASSERT_EQUAL_UINT32(0, cadv_fps_detonate(&f)); /* nothing left */
    f.ammo[2] = 0;
    TEST_ASSERT_EQUAL_UINT32(0, cadv_fps_fire(&f));
}

static void test_remote_bomb_expires(void)
{
    int i;
    custom_tables();
    f.weapon = 2;
    f.ammo[2] = 1;
    cadv_fps_fire(&f);
    f.proj[0].life = 0.01f;
    for (i = 0; i < 5; ++i) {
        cadv_fps_tick(&f, &in, DT);
    }
    TEST_ASSERT_FALSE(f.proj[0].active);
}

static void test_shrink_and_stomp(void)
{
    int e;
    int d;
    int i;
    uint32_t ev = 0;
    custom_tables();
    e = cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, -4), 0);
    d = cadv_fps_add_enemy(&f, 1, cadv_v3(3, 0, -8), 0); /* not shrinkable, flies */
    TEST_ASSERT_EQUAL_FLOAT(2.0f, f.enemies[d].pos.y);
    f.weapon = 3;
    f.ammo[3] = 5;
    TEST_ASSERT_TRUE(cadv_fps_fire(&f) & CADV_FPS_EV_SHRINK);
    TEST_ASSERT_EQUAL_INT(CADV_ENEMY_SHRUNK, f.enemies[e].state);
    TEST_ASSERT_FALSE(cadv_fps_fire(&f) & CADV_FPS_EV_SHRINK); /* already shrunk: no effect */
    cadv_fps_damage_enemy(&f, e, 1);                          /* pain does not unshrink */
    TEST_ASSERT_EQUAL_INT(CADV_ENEMY_SHRUNK, f.enemies[e].state);
    in.move_y = 1.0f;
    for (i = 0; i < 120 && !(ev & CADV_FPS_EV_STOMP); ++i) {
        ev |= cadv_fps_tick(&f, &in, DT);
    }
    TEST_ASSERT_TRUE(ev & CADV_FPS_EV_STOMP);
    TEST_ASSERT_EQUAL_INT(CADV_ENEMY_DEAD, f.enemies[e].state);
    in.move_y = 0.0f;
    f.player.yaw = cadv_atan2f(-(3.0f - f.player.pos.x), -(-8.0f - f.player.pos.z));
    f.player.pitch = 0.2f;
    {
        int hp = f.enemies[d].health;
        cadv_fps_fire(&f); /* immune to shrinking: takes damage instead */
        TEST_ASSERT_TRUE(f.enemies[d].health < hp);
    }
}

static void test_shrink_wears_off(void)
{
    int e;
    int i;
    custom_tables();
    cadv_world_add(&f.world, cadv_v3(-20, 0, 10), cadv_v3(20, 4, 11), 0);
    e = cadv_fps_add_enemy(&f, 0, cadv_v3(0, 0, -6), 0);
    f.weapon = 3;
    f.ammo[3] = 1;
    cadv_fps_fire(&f);
    for (i = 0; i < 400; ++i) {
        cadv_fps_tick(&f, &in, DT);
    }
    TEST_ASSERT_TRUE(f.enemies[e].state != CADV_ENEMY_SHRUNK);
}

static void test_breakables_by_shot_and_blast(void)
{
    int glass;
    int target;
    custom_tables();
    glass = cadv_fps_add_breakable(&f, cadv_v3(-1, 0, -3), cadv_v3(1, 3, -2.9f), 10, 0);
    target = cadv_fps_add_breakable(&f, cadv_v3(4, 0, -10), cadv_v3(5, 2, -9), 50, 1);
    TEST_ASSERT_TRUE(glass >= 0 && target >= 0);
    TEST_ASSERT_TRUE(cadv_fps_fire(&f) & CADV_FPS_EV_BREAK);
    TEST_ASSERT_TRUE(f.breakables[glass].broken);
    TEST_ASSERT_FALSE(f.world.s[f.breakables[glass].solid].enabled);
    TEST_ASSERT_EQUAL_INT(1, cadv_fps_broken_count(&f, 0));
    TEST_ASSERT_TRUE(cadv_fps_explode(&f, cadv_v3(4.5f, 1, -8.5f), 3.0f, 200, false) & CADV_FPS_EV_BREAK);
    TEST_ASSERT_EQUAL_INT(1, cadv_fps_broken_count(&f, 1));
    TEST_ASSERT_EQUAL_INT(target, f.last_broken);
    TEST_ASSERT_EQUAL_UINT32(CADV_FPS_EV_EXPLOSION, cadv_fps_explode(&f, cadv_v3(4.5f, 1, -8.5f), 0.0f, 10, false));
}

static void test_breakable_capacity(void)
{
    int i;
    for (i = 0; i < CADV_FPS_MAX_BREAKABLES; ++i) {
        TEST_ASSERT_TRUE(cadv_fps_add_breakable(&f, cadv_v3(10, 0, 10), cadv_v3(11, 1, 11), 5, 0) >= 0);
    }
    TEST_ASSERT_EQUAL_INT(-1, cadv_fps_add_breakable(&f, cadv_v3(10, 0, 10), cadv_v3(11, 1, 11), 5, 0));
    f.breakable_count = 0;
    f.world.count = CADV_MAX_SOLIDS;
    TEST_ASSERT_EQUAL_INT(-1, cadv_fps_add_breakable(&f, cadv_v3(10, 0, 10), cadv_v3(11, 1, 11), 5, 0));
}

static void test_jetpack_and_fuel(void)
{
    int i;
    uint32_t ev = 0;
    custom_tables();
    cadv_fps_add_pickup(&f, CADV_PICKUP_FUEL, 2, cadv_v3(0, 0.8f, 0)); /* no jetpack yet: still taken */
    cadv_fps_add_pickup(&f, CADV_PICKUP_JETPACK, 0, cadv_v3(0, 0.8f, 0.2f));
    cadv_fps_tick(&f, &in, DT);
    TEST_ASSERT_TRUE(f.jetpack);
    TEST_ASSERT_EQUAL_FLOAT(CADV_JET_FUEL_MAX, f.jet_fuel);
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_JUMP);
    cadv_fps_tick(&f, &in, DT);
    in.pressed = 0;
    for (i = 0; i < 90; ++i) {
        ev |= cadv_fps_tick(&f, &in, DT);
    }
    TEST_ASSERT_TRUE(ev & CADV_FPS_EV_JET);
    TEST_ASSERT_TRUE(f.player.pos.y > 2.0f);
    TEST_ASSERT_TRUE(f.jet_fuel < CADV_JET_FUEL_MAX);
    cadv_fps_add_pickup(&f, CADV_PICKUP_FUEL, 10, f.player.pos);
    f.pickups[f.pickup_count - 1].pos = cadv_v3_add(f.player.pos, cadv_v3(0, 0.8f, 0));
    in.held = 0;
    cadv_fps_tick(&f, &in, DT);
    TEST_ASSERT_EQUAL_FLOAT(CADV_JET_FUEL_MAX, f.jet_fuel);
    cadv_fps_add_pickup(&f, CADV_PICKUP_FUEL, 10, cadv_v3_add(f.player.pos, cadv_v3(0, 0.8f, 0)));
    cadv_fps_tick(&f, &in, DT);
    TEST_ASSERT_TRUE(f.pickups[f.pickup_count - 1].active); /* tank full: left lying */
}

static void test_ammo_types_and_bad_pickups(void)
{
    custom_tables();
    cadv_fps_add_pickup_ex(&f, CADV_PICKUP_AMMO, 7, 3, cadv_v3(0, 0.8f, 0));
    cadv_fps_add_pickup_ex(&f, CADV_PICKUP_AMMO, 7, 9, cadv_v3(0, 0.8f, 0.1f)); /* bad type: ignored */
    cadv_fps_add_pickup(&f, CADV_PICKUP_WEAPON, 4, cadv_v3(0, 0.8f, -0.1f));
    cadv_fps_tick(&f, &in, DT);
    TEST_ASSERT_EQUAL_INT(7, f.ammo[3]);
    TEST_ASSERT_EQUAL_INT(10, f.ammo[0]); /* chaingun brings bullets */
    TEST_ASSERT_EQUAL_INT(4, f.weapon);
}

static void test_flying_enemy_keeps_altitude(void)
{
    int d;
    custom_tables();
    d = cadv_fps_add_enemy(&f, 1, cadv_v3(0, 0, -12), 0);
    run(120);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, f.enemies[d].pos.y);
    TEST_ASSERT_TRUE(f.enemies[d].pos.z > -10.0f);
    TEST_ASSERT_TRUE(f.enemies[d].awake);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_player_walks_runs_jumps_and_lands);
    RUN_TEST(test_footsteps_and_look_limits);
    RUN_TEST(test_crouch_needs_headroom);
    RUN_TEST(test_damage_and_armor);
    RUN_TEST(test_defs);
    RUN_TEST(test_hitscan_kills_enemy);
    RUN_TEST(test_walls_block_shots);
    RUN_TEST(test_scattershot_needs_ammo_and_melee_range);
    RUN_TEST(test_weapon_selection);
    RUN_TEST(test_fire_input_and_cooldown);
    RUN_TEST(test_enemy_sees_chases_and_attacks);
    RUN_TEST(test_enemy_stays_idle_without_sight);
    RUN_TEST(test_projectiles_hurt_and_explode);
    RUN_TEST(test_player_launcher_splash);
    RUN_TEST(test_projectile_hits_wall_and_expires);
    RUN_TEST(test_pickups);
    RUN_TEST(test_pickup_capacity);
    RUN_TEST(test_doors_and_keys);
    RUN_TEST(test_door_capacity_and_non_door_use);
    RUN_TEST(test_dead_player_stops_acting);
    RUN_TEST(test_remote_bombs_stick_and_detonate);
    RUN_TEST(test_remote_bomb_expires);
    RUN_TEST(test_shrink_and_stomp);
    RUN_TEST(test_shrink_wears_off);
    RUN_TEST(test_breakables_by_shot_and_blast);
    RUN_TEST(test_breakable_capacity);
    RUN_TEST(test_jetpack_and_fuel);
    RUN_TEST(test_ammo_types_and_bad_pickups);
    RUN_TEST(test_flying_enemy_keeps_altitude);
    return UNITY_END();
}
