/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_VEHICLE_WALKER_H
#define CADV_KITS_VEHICLE_WALKER_H

#include <stdbool.h>

#include "kits/vehicle/terrain.h"

/* A giant walking machine: legs and torso turn separately (torso twist), a
 * throttle sets the walking speed (with reverse), weapons and jump jets add
 * heat that heat sinks shed (faster in water, snow vents or with coolant),
 * and too much heat shuts the machine down until it has cooled. Damage lands
 * on eight components; a lost arm takes its weapons, a hurt leg slows the
 * walk, and losing the head, the centre torso or both legs destroys it. */

typedef enum cadv_walker_zone {
    CADV_WZ_HEAD = 0,
    CADV_WZ_CT,
    CADV_WZ_LT,
    CADV_WZ_RT,
    CADV_WZ_LA,
    CADV_WZ_RA,
    CADV_WZ_LL,
    CADV_WZ_RL,
    CADV_WZ_COUNT
} cadv_walker_zone;

typedef struct cadv_walker_def {
    float max_speed;     /* m/s at full throttle */
    float reverse;       /* fraction of max speed when reversing (throttle < 0) */
    float accel;         /* m/s^2 towards the throttle speed */
    float throttle_rate; /* throttle change per second at full stick */
    float turn_rate;     /* legs, rad/s at full stick */
    float twist_limit;   /* torso twist either way, rad */
    float twist_rate;    /* rad/s the torso can follow the look */
    float height;        /* cockpit height above the feet */
    float max_climb;     /* steepest walkable slope (rise per metre) */
    float jump_thrust;   /* upward acceleration with jump jets, m/s^2 */
    float jump_fuel;     /* seconds of thrust */
    float jump_heat;     /* heat per second of thrust */
    float gravity;
    float heat_cap;      /* shutdown at this heat */
    float heat_shed;     /* heat per second shed by the sinks */
    float restart_heat;  /* a shut-down machine restarts below this */
    int armour[CADV_WZ_COUNT];
} cadv_walker_def;

/* Per-tick controls, already mapped from whatever input drives the walker. */
typedef struct cadv_walker_ctl {
    float throttle;  /* -1..1 change of the throttle (stick) */
    float turn;      /* -1..1 legs (positive = right) */
    float twist;     /* rad of wanted torso turn this tick */
    float pitch;     /* rad of wanted aim pitch change this tick */
    bool stop;       /* all stop: throttle to 0 */
    bool centre;     /* bring the torso back over the legs */
    bool jump;       /* jump jets */
} cadv_walker_ctl;

typedef struct cadv_walker {
    cadv_vec3 pos;        /* feet */
    float vy;
    float legs_yaw;       /* walking direction */
    float twist;          /* torso relative to the legs */
    float pitch;
    float throttle;       /* -1..1 */
    float speed;          /* signed m/s along the legs */
    float fuel;           /* jump jet seconds left */
    float heat;
    bool shutdown;
    float shutdown_time;  /* seconds since the shutdown */
    int shutdowns;        /* times it overheated */
    bool on_ground;
    bool blocked;         /* a slope stopped it this tick */
    bool jetting;
    int hp[CADV_WZ_COUNT];
    int lost;             /* components destroyed so far */
    float stride;         /* walk cycle phase, radians */
} cadv_walker;

void cadv_walker_spawn(cadv_walker *w, const cadv_walker_def *d, cadv_vec3 pos, float yaw);
/* 'cooling' multiplies the heat shed this tick (1 = normal). */
void cadv_walker_tick(cadv_walker *w, const cadv_walker_def *d, const cadv_walker_ctl *c, const cadv_terrain *t,
                      float cooling, float dt);
void cadv_walker_add_heat(cadv_walker *w, const cadv_walker_def *d, float heat);
/* Applies damage to a component; a destroyed arm or leg passes the rest to
 * the torso beside it. Returns true when this hit destroyed the walker. */
bool cadv_walker_damage(cadv_walker *w, cadv_walker_zone zone, int amount);
bool cadv_walker_alive(const cadv_walker *w);
bool cadv_walker_has(const cadv_walker *w, cadv_walker_zone zone);
/* Which component a hit at world point 'p' strikes. */
cadv_walker_zone cadv_walker_hit_zone(const cadv_walker *w, const cadv_walker_def *d, cadv_vec3 p);
float cadv_walker_speed_factor(const cadv_walker *w, const cadv_walker_def *d);
float cadv_walker_torso_yaw(const cadv_walker *w);
cadv_vec3 cadv_walker_eye(const cadv_walker *w, const cadv_walker_def *d);
cadv_vec3 cadv_walker_aim(const cadv_walker *w); /* unit vector the torso points at */
const char *cadv_walker_zone_name(cadv_walker_zone z);

#endif
