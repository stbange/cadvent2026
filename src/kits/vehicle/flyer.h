/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_VEHICLE_FLYER_H
#define CADV_KITS_VEHICLE_FLYER_H

#include "input/input.h"
#include "kits/vehicle/terrain.h"

/* A hovering flyer (magic carpet, jet sled ...): forward thrust and strafe from
 * the move stick, yaw/pitch from look, JUMP climbs and CROUCH sinks. It keeps
 * a clearance above ground and water, stays inside the world and under a
 * ceiling, glides up gentle slopes and bumps into faces steeper than about
 * 68 degrees instead of popping over them. */

typedef struct cadv_flyer_def {
    float max_speed;    /* m/s at full thrust */
    float accel;        /* m/s^2 */
    float drag;         /* fraction of velocity lost per second (0..1) */
    float climb;        /* m/s vertical with JUMP/CROUCH */
    float clearance;    /* minimum height above the surface */
    float ceiling;      /* maximum altitude */
    float boost;        /* speed multiplier with RUN */
} cadv_flyer_def;

typedef struct cadv_flyer {
    cadv_vec3 pos;
    cadv_vec3 vel;
    float yaw, pitch;
    float bank;         /* visual roll from strafing/turning, -1..1 */
    bool bumped;        /* hit terrain this tick */
} cadv_flyer;

void cadv_flyer_spawn(cadv_flyer *f, cadv_vec3 pos, float yaw);
void cadv_flyer_tick(cadv_flyer *f, const cadv_flyer_def *d, const cadv_input_frame *in,
                     const cadv_terrain *t, float dt);
cadv_vec3 cadv_flyer_forward(const cadv_flyer *f); /* look direction incl. pitch */
float cadv_flyer_altitude(const cadv_flyer *f, const cadv_terrain *t); /* above the surface */

#endif
