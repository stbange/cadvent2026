/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_FPS_FACADE_H
#define CADV_KITS_FPS_FACADE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/mathx.h"
#include "sim/collide.h"

/* Destructible building fronts: a wall split into a grid of voxel chunks,
 * each a solid with its own hit points. Blasts chip and remove chunks in
 * their radius; chunks left hanging with nothing below or beside them fall
 * too, so a wall is never left floating. */

#define CADV_FACADE_W 12
#define CADV_FACADE_H 8

typedef struct cadv_facade {
    cadv_vec3 origin;   /* bottom-left-front corner */
    int axis;           /* 0: runs along +x, 1: runs along +z */
    int w, h;
    float size, depth;
    uint8_t hp[CADV_FACADE_W * CADV_FACADE_H];
    int16_t solid[CADV_FACADE_W * CADV_FACADE_H];
    int alive;
    uint32_t ver;       /* bumped whenever a chunk goes */
} cadv_facade;

bool cadv_facade_build(cadv_facade *f, cadv_world *w, cadv_vec3 origin, int axis, int cols, int rows, float size, float depth, int hp);
bool cadv_facade_alive(const cadv_facade *f, int x, int y);
cadv_vec3 cadv_facade_centre(const cadv_facade *f, int x, int y);
/* Damages every chunk within radius (falling off to the edge); returns how
 * many chunks went, loose ones included. */
int cadv_facade_blast(cadv_facade *f, cadv_world *w, cadv_vec3 at, float radius, int damage);

#endif
