/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_FPS_COMBAT_H
#define CADV_KITS_FPS_COMBAT_H

#include <stdbool.h>
#include <stdint.h>

#include "core/rng.h"
#include "kits/fps/player.h"
#include "sim/collide.h"

/* Weapons, enemies, projectiles, pickups, doors, breakables and the jetpack:
 * the reusable FPS kit. Portals may supply their own weapon/enemy tables. */

#define CADV_FPS_MAX_ENEMIES 64
#define CADV_FPS_MAX_PROJECTILES 64
#define CADV_FPS_MAX_PICKUPS 96
#define CADV_FPS_MAX_DOORS 16
#define CADV_FPS_MAX_BREAKABLES 48
#define CADV_FPS_MAX_WEAPONS 8
#define CADV_FPS_WEAPONS 4 /* entries in the default weapon table */
#define CADV_AMMO_TYPES 4
#define CADV_JET_FUEL_MAX 6.0f

typedef enum cadv_weapon_kind {
    CADV_WK_MELEE = 0,
    CADV_WK_HITSCAN,
    CADV_WK_PROJECTILE, /* explodes on impact */
    CADV_WK_REMOTE,     /* thrown, sticks, ALT_FIRE detonates all */
    CADV_WK_SHRINK      /* shrinks enemies; walk over them to stomp */
} cadv_weapon_kind;

typedef struct cadv_weapon_def {
    const char *name;
    cadv_weapon_kind kind;
    int damage;        /* per pellet / explosion centre */
    int pellets;
    float spread;      /* radians */
    float cooldown;    /* seconds between shots */
    float range;
    int ammo_type;     /* 0..CADV_AMMO_TYPES-1 */
    int ammo_per_shot; /* 0 = infinite */
    float proj_speed;
    float splash;      /* explosion radius */
} cadv_weapon_def;

const cadv_weapon_def *cadv_weapon_def_get(int index); /* default table */

typedef enum cadv_enemy_state {
    CADV_ENEMY_IDLE = 0,
    CADV_ENEMY_CHASE,
    CADV_ENEMY_ATTACK,
    CADV_ENEMY_PAIN,
    CADV_ENEMY_DEAD,
    CADV_ENEMY_SHRUNK
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
    float fly_height;   /* > 0: hovers this high above its spawn floor */
    bool shrinkable;
} cadv_enemy_def;

const cadv_enemy_def *cadv_enemy_def_get(int index); /* default table */
#define CADV_ENEMY_TYPES 3

typedef struct cadv_enemy {
    int type;
    cadv_vec3 pos;
    float yaw;
    int health;
    cadv_enemy_state state;
    float timer;
    bool active;
    bool awake;     /* has seen the player at least once */
} cadv_enemy;

typedef struct cadv_projectile {
    cadv_vec3 pos, vel;
    int damage;
    float splash;
    float gravity;
    bool from_player;
    bool remote;    /* remote bomb: sticks, waits for detonation */
    bool stuck;
    bool active;
    float life;
} cadv_projectile;

typedef enum cadv_pickup_kind {
    CADV_PICKUP_HEALTH = 0,
    CADV_PICKUP_ARMOR,
    CADV_PICKUP_AMMO,   /* sub = ammo type */
    CADV_PICKUP_KEY,    /* value = key bits */
    CADV_PICKUP_WEAPON, /* value = weapon index */
    CADV_PICKUP_STAR,   /* the portal's Shining Star */
    CADV_PICKUP_JETPACK,
    CADV_PICKUP_FUEL
} cadv_pickup_kind;

typedef struct cadv_pickup {
    cadv_pickup_kind kind;
    int value;
    int sub;
    cadv_vec3 pos;
    bool active;
} cadv_pickup;

typedef struct cadv_door {
    int solid;
    uint32_t key;
    float open;
    bool opening;
    cadv_vec3 closed_lo, closed_hi;
} cadv_door;

typedef struct cadv_breakable {
    int solid;
    int hp;
    int group;      /* portal-defined (glass = 0, emitters = 1, ...) */
    bool broken;
} cadv_breakable;

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
    CADV_FPS_EV_EXPLOSION = 1 << 9,
    CADV_FPS_EV_BREAK = 1 << 10,
    CADV_FPS_EV_USE_SOLID = 1 << 11,  /* USE hit a non-door solid: see use_solid */
    CADV_FPS_EV_STOMP = 1 << 12,
    CADV_FPS_EV_SHRINK = 1 << 13,
    CADV_FPS_EV_JET = 1 << 14
};

typedef struct cadv_fps {
    cadv_world world;
    cadv_player player;
    cadv_rng rng;
    const cadv_weapon_def *wdefs;
    int wcount;
    const cadv_enemy_def *edefs;
    int ecount;
    int weapon;
    bool owned[CADV_FPS_MAX_WEAPONS];
    int ammo[CADV_AMMO_TYPES];
    float cooldown;
    cadv_enemy enemies[CADV_FPS_MAX_ENEMIES];
    int enemy_count;
    cadv_projectile proj[CADV_FPS_MAX_PROJECTILES];
    cadv_pickup pickups[CADV_FPS_MAX_PICKUPS];
    int pickup_count;
    cadv_door doors[CADV_FPS_MAX_DOORS];
    int door_count;
    cadv_breakable breakables[CADV_FPS_MAX_BREAKABLES];
    int breakable_count;
    int kills;
    bool star_taken;
    bool jetpack;
    float jet_fuel;
    int use_solid;        /* solid hit by the last USE, -1 = none */
    int last_killed;      /* enemy index of the last kill, -1 = none */
    int last_broken;      /* breakable index of the last break, -1 = none */
    float damage_scale;   /* difficulty: damage dealt to the player */
    float muzzle_flash;   /* seconds of muzzle light left */
} cadv_fps;

void cadv_fps_init(cadv_fps *f, uint64_t seed);
/* Replaces the weapon/enemy tables (pointers must stay valid). */
void cadv_fps_set_tables(cadv_fps *f, const cadv_weapon_def *w, int wcount, const cadv_enemy_def *e,
                         int ecount);
const cadv_weapon_def *cadv_fps_weapon(const cadv_fps *f, int index);
const cadv_enemy_def *cadv_fps_enemy_def(const cadv_fps *f, int type);
int cadv_fps_add_enemy(cadv_fps *f, int type, cadv_vec3 pos, float yaw);
int cadv_fps_add_pickup(cadv_fps *f, cadv_pickup_kind kind, int value, cadv_vec3 pos);
int cadv_fps_add_pickup_ex(cadv_fps *f, cadv_pickup_kind kind, int value, int sub, cadv_vec3 pos);
int cadv_fps_add_door(cadv_fps *f, cadv_vec3 lo, cadv_vec3 hi, uint32_t key);
int cadv_fps_add_breakable(cadv_fps *f, cadv_vec3 lo, cadv_vec3 hi, int hp, int group);
int cadv_fps_alive_enemies(const cadv_fps *f);
int cadv_fps_broken_count(const cadv_fps *f, int group);
/* Runs one tick of the whole FPS world; returns CADV_FPS_EV_* flags. */
uint32_t cadv_fps_tick(cadv_fps *f, const cadv_input_frame *in, float dt);
/* Fires the current weapon now (ignores cooldown); exposed for tests. */
uint32_t cadv_fps_fire(cadv_fps *f);
/* Detonates all of the player's remote bombs. */
uint32_t cadv_fps_detonate(cadv_fps *f);
/* Opens the door the player looks at, or reports the solid in use_solid. */
uint32_t cadv_fps_use(cadv_fps *f);
/* Applies an explosion (also usable by portals for scripted blasts). */
uint32_t cadv_fps_explode(cadv_fps *f, cadv_vec3 at, float radius, int damage, bool hurts_player);
/* Damages an enemy directly; returns event flags. */
uint32_t cadv_fps_damage_enemy(cadv_fps *f, int index, int damage);

#endif
