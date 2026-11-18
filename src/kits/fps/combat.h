/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_FPS_COMBAT_H
#define CADV_KITS_FPS_COMBAT_H

#include <stdbool.h>
#include <stdint.h>

#include "core/rng.h"
#include "kits/fps/player.h"
#include "sim/collide.h"

/* Weapons, enemies, projectiles, pickups and doors: the reusable FPS kit. */

#define CADV_FPS_MAX_ENEMIES 64
#define CADV_FPS_MAX_PROJECTILES 64
#define CADV_FPS_MAX_PICKUPS 64
#define CADV_FPS_MAX_DOORS 16
#define CADV_FPS_WEAPONS 4

typedef struct cadv_weapon_def {
    const char *name;
    int damage;        /* per pellet */
    int pellets;
    float spread;      /* radians */
    float cooldown;    /* seconds between shots */
    float range;
    int ammo_per_shot; /* 0 = infinite (melee / basic) */
    bool projectile;   /* slow projectile instead of hitscan */
    float proj_speed;
    float splash;      /* splash radius for projectiles */
} cadv_weapon_def;

const cadv_weapon_def *cadv_weapon_def_get(int index);

typedef enum cadv_enemy_state {
    CADV_ENEMY_IDLE = 0,
    CADV_ENEMY_CHASE,
    CADV_ENEMY_ATTACK,
    CADV_ENEMY_PAIN,
    CADV_ENEMY_DEAD
} cadv_enemy_state;

typedef struct cadv_enemy_def {
    const char *name;
    int health;
    float speed;
    float radius, height;
    float sight;        /* metres */
    float attack_range; /* melee if <= 1.6 */
    float attack_cooldown;
    int damage;
    bool ranged_projectile;
} cadv_enemy_def;

const cadv_enemy_def *cadv_enemy_def_get(int index);
#define CADV_ENEMY_TYPES 3

typedef struct cadv_enemy {
    int type;
    cadv_vec3 pos;
    float yaw;
    int health;
    cadv_enemy_state state;
    float timer;
    bool active;
} cadv_enemy;

typedef struct cadv_projectile {
    cadv_vec3 pos, vel;
    int damage;
    float splash;
    bool from_player;
    bool active;
    float life;
} cadv_projectile;

typedef enum cadv_pickup_kind {
    CADV_PICKUP_HEALTH = 0,
    CADV_PICKUP_ARMOR,
    CADV_PICKUP_AMMO,
    CADV_PICKUP_KEY,
    CADV_PICKUP_WEAPON,
    CADV_PICKUP_STAR /* the portal's Shining Star */
} cadv_pickup_kind;

typedef struct cadv_pickup {
    cadv_pickup_kind kind;
    int value;       /* amount, key bit or weapon index */
    cadv_vec3 pos;
    bool active;
} cadv_pickup;

typedef struct cadv_door {
    int solid;       /* index into the world */
    uint32_t key;    /* required key bits, 0 = none */
    float open;      /* 0 closed .. 1 open */
    bool opening;
    cadv_vec3 closed_lo, closed_hi;
} cadv_door;

enum {
    CADV_FPS_EV_SHOT = 1 << 0,
    CADV_FPS_EV_ENEMY_HIT = 1 << 1,
    CADV_FPS_EV_ENEMY_KILLED = 1 << 2,
    CADV_FPS_EV_PLAYER_HURT = 1 << 3,
    CADV_FPS_EV_PLAYER_DIED = 1 << 4,
    CADV_FPS_EV_PICKUP = 1 << 5,
    CADV_FPS_EV_STAR = 1 << 6,
    CADV_FPS_EV_DOOR = 1 << 7,
    CADV_FPS_EV_DOOR_LOCKED = 1 << 8,
    CADV_FPS_EV_EXPLOSION = 1 << 9
};

typedef struct cadv_fps {
    cadv_world world;
    cadv_player player;
    cadv_rng rng;
    int weapon;                    /* current weapon index */
    bool owned[CADV_FPS_WEAPONS];
    int ammo;
    float cooldown;
    cadv_enemy enemies[CADV_FPS_MAX_ENEMIES];
    int enemy_count;
    cadv_projectile proj[CADV_FPS_MAX_PROJECTILES];
    cadv_pickup pickups[CADV_FPS_MAX_PICKUPS];
    int pickup_count;
    cadv_door doors[CADV_FPS_MAX_DOORS];
    int door_count;
    int kills;
    bool star_taken;
    float damage_scale;            /* difficulty: damage dealt to the player */
    float muzzle_flash;            /* seconds of muzzle light left */
} cadv_fps;

void cadv_fps_init(cadv_fps *f, uint64_t seed);
int cadv_fps_add_enemy(cadv_fps *f, int type, cadv_vec3 pos, float yaw);
int cadv_fps_add_pickup(cadv_fps *f, cadv_pickup_kind kind, int value, cadv_vec3 pos);
int cadv_fps_add_door(cadv_fps *f, cadv_vec3 lo, cadv_vec3 hi, uint32_t key);
int cadv_fps_alive_enemies(const cadv_fps *f);
/* Runs one tick of the whole FPS world; returns CADV_FPS_EV_* flags. */
uint32_t cadv_fps_tick(cadv_fps *f, const cadv_input_frame *in, float dt);
/* Fires the current weapon now (ignores cooldown); exposed for tests. */
uint32_t cadv_fps_fire(cadv_fps *f);
/* Tries to open the door the player looks at; returns event flags. */
uint32_t cadv_fps_use(cadv_fps *f);

#endif
