/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_FPS_STORY_H
#define CADV_KITS_FPS_STORY_H

#include <stdbool.h>
#include <stdint.h>

#include "kits/fps/tactics.h"
#include "sim/collide.h"

/* Story-driven shooter pieces: trigger volumes for scripted sequences that
 * never take control away, NPC followers, a squad AI decision rule and wall
 * chargers with a limited reserve. */

/* ---- triggers ---------------------------------------------------------------------- */

typedef struct cadv_trigger {
    cadv_aabb box;
    bool once;      /* fires on the first entry only; else on every entry */
    bool inside;
    int fired;      /* times it has fired */
} cadv_trigger;

void cadv_trigger_make(cadv_trigger *t, cadv_vec3 lo, cadv_vec3 hi, bool once);
/* True on the tick a point enters the volume (subject to 'once'). */
bool cadv_trigger_update(cadv_trigger *t, cadv_vec3 p);

/* ---- followers -------------------------------------------------------------------------- */

typedef enum cadv_follow_mode { CADV_FOLLOW_WAIT = 0, CADV_FOLLOW_ON, CADV_FOLLOW_GOTO } cadv_follow_mode;

typedef struct cadv_follower {
    cadv_follow_mode mode;
    cadv_vec3 pos;           /* feet */
    cadv_vec3 goal;          /* for GOTO */
    float speed;             /* m/s */
    float keep;              /* distance kept from the leader */
    cadv_nav_cell path[96];
    int path_len, path_pos;
    int repath;              /* ticks to the next re-plan */
    bool arrived;
} cadv_follower;

void cadv_follower_init(cadv_follower *f, cadv_vec3 pos, float speed, float keep);
/* Use on a follower: WAIT -> follow, following -> wait. */
void cadv_follower_toggle(cadv_follower *f);
void cadv_follower_goto(cadv_follower *f, cadv_vec3 goal);
/* Moves along the walk grid towards the leader (or the goal), colliding with
 * the world; returns the distance still to go. */
float cadv_follower_tick(cadv_follower *f, const cadv_nav *nav, const cadv_world *w, cadv_vec3 leader, float dt);

/* ---- squad AI -------------------------------------------------------------------------------- */

typedef enum cadv_squad_state {
    CADV_SQUAD_PATROL = 0,
    CADV_SQUAD_ENGAGE,
    CADV_SQUAD_FLANK,
    CADV_SQUAD_SUPPRESS,
    CADV_SQUAD_GRENADE,
    CADV_SQUAD_RETREAT
} cadv_squad_state;

typedef struct cadv_squad_view {
    bool sees_target;
    float target_dist;
    float hp;              /* own health 0..1 */
    int allies_engaged;    /* squad mates already fighting */
    bool has_grenade;
    float target_hidden;   /* seconds the target has been in cover */
    bool flank_open;       /* a flanking route exists */
} cadv_squad_view;

/* Next state for one soldier given what it knows. Hurt soldiers fall back,
 * a target holed up in cover gets a grenade, a second soldier flanks, a third
 * pins the target down, everyone else engages; nobody to fight: patrol. */
cadv_squad_state cadv_squad_decide(cadv_squad_state current, const cadv_squad_view *v);
const char *cadv_squad_state_name(cadv_squad_state s);

/* ---- chargers ---------------------------------------------------------------------------------- */

typedef struct cadv_charger {
    float reserve;     /* what is left to give */
    float rate;        /* per second */
} cadv_charger;

/* Tops 'value' up towards 'max' for 'dt' seconds; returns what was given. */
float cadv_charger_use(cadv_charger *c, float *value, float max, float dt);

#endif
