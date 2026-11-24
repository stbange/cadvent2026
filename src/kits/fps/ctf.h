/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_FPS_CTF_H
#define CADV_KITS_FPS_CTF_H

#include <stdbool.h>

#include "core/mathx.h"

/* Capture the Flag pieces: flags that are carried, dropped and returned on
 * a timer, captures, and base power: a generator that feeds the turrets and
 * inventory stations, goes dark when destroyed and is repaired after a while. */

#define CADV_FLAG_RETURN 30.0f   /* seconds a dropped flag lies before going home */
#define CADV_FLAG_TOUCH 2.5f     /* metres */

typedef enum cadv_flag_state { CADV_FLAG_HOME, CADV_FLAG_CARRIED, CADV_FLAG_DROPPED } cadv_flag_state;
typedef enum cadv_flag_event { CADV_FLAG_NONE, CADV_FLAG_TAKEN, CADV_FLAG_RETURNED, CADV_FLAG_CAPTURED } cadv_flag_event;

typedef struct cadv_flag {
    int team;
    cadv_flag_state state;
    cadv_vec3 home, pos, vel;
    int carrier;        /* agent index while carried, -1 otherwise */
    float return_t;     /* seconds left while dropped */
} cadv_flag;

void cadv_flag_init(cadv_flag *f, int team, cadv_vec3 home);
/* Someone of 'team' touches the flag: the enemy picks it up (home or
 * dropped), a team mate returns it when dropped. */
cadv_flag_event cadv_flag_touch(cadv_flag *f, int agent, int team);
/* The carrier lets go (or falls): the flag drops with some of their speed. */
void cadv_flag_drop(cadv_flag *f, cadv_vec3 pos, cadv_vec3 vel);
/* Counts the return timer down; true when the flag went home by itself. */
bool cadv_flag_tick(cadv_flag *f, float dt);
/* The carrier of 'enemy' reaches the stand of 'own': a capture only when
 * the own flag is home. Sends the captured flag home. */
cadv_flag_event cadv_flag_try_capture(cadv_flag *enemy, const cadv_flag *own, cadv_vec3 carrier_pos);
void cadv_flag_return(cadv_flag *f);

typedef struct cadv_generator {
    float hp, max_hp;
    float repair_time;  /* seconds from destroyed to powered again */
    float down_t;       /* seconds left until repaired, 0 when running */
} cadv_generator;

typedef struct cadv_turret {
    cadv_vec3 pos;
    float range;
    float cooldown;     /* seconds between shots */
    float reload;       /* seconds left */
} cadv_turret;

void cadv_generator_init(cadv_generator *g, float hp, float repair_time);
bool cadv_generator_powered(const cadv_generator *g);
/* Returns true when this damage destroyed it. */
bool cadv_generator_damage(cadv_generator *g, float amount);
/* Returns true when the repair finished this tick. */
bool cadv_generator_tick(cadv_generator *g, float dt);
/* Counts the reload down; true when the turret may fire at a target at
 * 'dist': it needs power, range and a loaded barrel. */
bool cadv_turret_ready(cadv_turret *t, const cadv_generator *g, float dist, float dt);

#endif
