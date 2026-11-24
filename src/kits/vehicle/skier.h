/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_VEHICLE_SKIER_H
#define CADV_KITS_VEHICLE_SKIER_H

#include <stdbool.h>

#include "core/mathx.h"
#include "kits/vehicle/terrain.h"
#include "sim/collide.h"

/* Armoured skiers on heightmap terrain: walk, ski (hold jump on the ground:
 * no friction, gravity pulls you along the slope), jetpack on an energy
 * reserve that drains while jetting and regenerates otherwise. Three armour
 * classes trade speed and energy for health and weapon slots. */

#define CADV_SKIER_GRAVITY 20.0f
#define CADV_SKIER_RADIUS 0.45f
#define CADV_SKIER_HEIGHT 2.0f

typedef enum cadv_armour_class { CADV_ARMOUR_LIGHT, CADV_ARMOUR_MEDIUM, CADV_ARMOUR_HEAVY, CADV_ARMOURS } cadv_armour_class;

typedef struct cadv_armour_def {
    const char *name;
    float run_speed;   /* m/s on foot */
    float mass;        /* scales knockback */
    float energy_max;
    float jet_accel;   /* m/s^2 upwards while jetting (gravity is 20) */
    float jet_drain;   /* energy per second while jetting */
    float regen;       /* energy per second otherwise */
    float hp;
    int slots;         /* weapon slots */
} cadv_armour_def;

typedef struct cadv_skier_input {
    float move_x, move_y; /* strafe right / forward, -1..1 */
    bool ski;             /* held */
    bool jet;             /* held */
    bool jump;            /* pressed this tick */
} cadv_skier_input;

typedef struct cadv_skier {
    cadv_vec3 pos;        /* feet */
    cadv_vec3 vel;
    float yaw, pitch;
    float energy;
    float hp;
    cadv_armour_class armour;
    bool energy_pack;     /* +50% regeneration */
    bool on_ground;
    bool skiing;          /* sliding without friction this tick */
    bool jetting;
    float airtime;        /* seconds since last touching the ground */
    float jet_lock;       /* seconds before jetting works again after running dry */
} cadv_skier;

const cadv_armour_def *cadv_armour_get(cadv_armour_class c);
void cadv_skier_init(cadv_skier *k, cadv_armour_class c, cadv_vec3 pos, float yaw);
/* Changes the armour; health and energy keep their fraction. */
void cadv_skier_set_armour(cadv_skier *k, cadv_armour_class c);
float cadv_skier_regen(const cadv_skier *k);
/* Advances one step on the terrain, colliding with the solids too. */
void cadv_skier_step(cadv_skier *k, const cadv_terrain *t, const cadv_world *w, const cadv_skier_input *in, float dt);
float cadv_skier_speed(const cadv_skier *k);
/* A push (explosions): the change in velocity is impulse / mass. */
void cadv_skier_push(cadv_skier *k, cadv_vec3 impulse);
cadv_vec3 cadv_skier_eye(const cadv_skier *k);
cadv_vec3 cadv_skier_forward(const cadv_skier *k);

#endif
