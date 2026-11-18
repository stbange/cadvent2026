/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_VEHICLE_TERRAIN_H
#define CADV_KITS_VEHICLE_TERRAIN_H

#include <stdbool.h>
#include <stdint.h>

#include "core/mathx.h"

/* Deformable heightmap terrain for vehicle portals. The grid has n x n height
 * samples spaced 'cell' metres apart; world x/z run from 0 to (n-1)*cell.
 * Heights always stay inside [min_h, max_h], whatever spells do to them. */

#define CADV_TERRAIN_MAX_N 129
#define CADV_TERRAIN_CHUNK 16 /* cells per render chunk side */
#define CADV_TERRAIN_MAX_CHUNKS (((CADV_TERRAIN_MAX_N - 1) / CADV_TERRAIN_CHUNK) * ((CADV_TERRAIN_MAX_N - 1) / CADV_TERRAIN_CHUNK))

typedef struct cadv_terrain {
    int n;
    float cell;
    float min_h, max_h;
    float sea;                 /* water level (heights below are under water) */
    float h[CADV_TERRAIN_MAX_N * CADV_TERRAIN_MAX_N];
    uint32_t chunk_ver[CADV_TERRAIN_MAX_CHUNKS]; /* bumped whenever a render chunk changes */
} cadv_terrain;

typedef struct cadv_island {
    float x, z;     /* centre in world metres */
    float radius;
    float height;   /* peak height above the sea floor */
    float flat;     /* 0..1: how much of the top is a plateau */
} cadv_island;

bool cadv_terrain_init(cadv_terrain *t, int n, float cell, float min_h, float max_h, float sea);
float cadv_terrain_size(const cadv_terrain *t);
/* Builds islands with fbm detail on a sea floor at min_h + 2. */
void cadv_terrain_islands(cadv_terrain *t, const cadv_island *isl, int count, uint32_t seed);
float cadv_terrain_sample(const cadv_terrain *t, int ix, int iz); /* clamped grid lookup */
/* Bilinear height at world x/z (clamped to the edge). */
float cadv_terrain_height(const cadv_terrain *t, float x, float z);
/* Height of the surface you would stand or float on: max(ground, sea). */
float cadv_terrain_surface(const cadv_terrain *t, float x, float z);
cadv_vec3 cadv_terrain_normal(const cadv_terrain *t, float x, float z);
/* Adds 'amount' metres at the centre with a smooth falloff to zero at radius.
 * Returns the total volume moved (sum of absolute changes, metres). */
float cadv_terrain_deform(cadv_terrain *t, float x, float z, float radius, float amount);
/* Lifts the ground towards 'height' (never lowers it), full strength at the
 * centre fading to nothing at radius: stamping the same spot twice is a no-op. */
void cadv_terrain_raise_to(cadv_terrain *t, float x, float z, float radius, float height);
/* Moves heights in the radius towards 'target' by up to 'rate' metres. */
void cadv_terrain_flatten(cadv_terrain *t, float x, float z, float radius, float target, float rate);
/* Steepest height difference within the radius (0 = flat). */
float cadv_terrain_roughness(const cadv_terrain *t, float x, float z, float radius);
/* Marches the ray against the ground; true and the hit point when it lands
 * within max_dist. */
bool cadv_terrain_raycast(const cadv_terrain *t, cadv_vec3 origin, cadv_vec3 dir, float max_dist, cadv_vec3 *hit);
/* True when the straight line from a to b stays above the ground. */
bool cadv_terrain_line_clear(const cadv_terrain *t, cadv_vec3 a, cadv_vec3 b);
int cadv_terrain_chunks_per_side(const cadv_terrain *t);
void cadv_terrain_mark_all_dirty(cadv_terrain *t);

#endif
