/* SPDX-License-Identifier: MIT */
#ifndef CADV_SIM_COLLIDE_H
#define CADV_SIM_COLLIDE_H

#include <stdbool.h>

#include "core/mathx.h"

/* Static level collision made of axis-aligned boxes. Simple, deterministic and
 * a good fit for the blocky 90s level geometry of the hall and most portals. */

#define CADV_MAX_SOLIDS 1024

typedef struct cadv_aabb {
    cadv_vec3 lo, hi;
} cadv_aabb;

typedef struct cadv_solid {
    cadv_aabb box;
    int tag;      /* game-defined (door id, wall, ...); 0 = plain */
    bool enabled; /* disabled solids are skipped (open doors) */
} cadv_solid;

typedef struct cadv_world {
    cadv_solid s[CADV_MAX_SOLIDS];
    int count;
} cadv_world;

cadv_aabb cadv_aabb_make(cadv_vec3 lo, cadv_vec3 hi);
bool cadv_aabb_overlap(cadv_aabb a, cadv_aabb b);
bool cadv_aabb_contains(cadv_aabb a, cadv_vec3 p);

void cadv_world_clear(cadv_world *w);
/* Returns the solid index or -1 when full. */
int cadv_world_add(cadv_world *w, cadv_vec3 lo, cadv_vec3 hi, int tag);
bool cadv_world_overlaps(const cadv_world *w, cadv_aabb box);

/* Ray cast against enabled solids. Returns true on a hit within max_dist and
 * fills distance, hit normal and solid index (any out pointer may be NULL). */
bool cadv_world_raycast(const cadv_world *w, cadv_vec3 origin, cadv_vec3 dir, float max_dist,
                        float *dist, cadv_vec3 *normal, int *solid);
/* Ray vs a single box (slab test). */
bool cadv_ray_aabb(cadv_vec3 origin, cadv_vec3 dir, cadv_aabb b, float max_dist, float *t,
                   cadv_vec3 *normal);
/* Ray vs vertical cylinder (enemies): base centre, radius, height. */
bool cadv_ray_cylinder(cadv_vec3 origin, cadv_vec3 dir, cadv_vec3 base, float radius, float height,
                       float max_dist, float *t);
/* True if the segment a-b is not blocked by any solid. */
bool cadv_world_line_of_sight(const cadv_world *w, cadv_vec3 a, cadv_vec3 b);

typedef struct cadv_move_result {
    bool hit_x, hit_z;   /* blocked horizontally */
    bool on_ground;      /* resting on something after the move */
    bool hit_ceiling;
    bool stepped;        /* climbed a step */
} cadv_move_result;

/* Moves a body (feet at *pos, box of half-width r and height h) by delta,
 * sliding along walls and stepping up obstacles up to step_h. */
cadv_move_result cadv_world_move(const cadv_world *w, cadv_vec3 *pos, float r, float h,
                                 cadv_vec3 delta, float step_h);

#endif
