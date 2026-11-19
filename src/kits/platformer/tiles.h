/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_PLATFORMER_TILES_H
#define CADV_KITS_PLATFORMER_TILES_H

#include <stdbool.h>
#include <stdint.h>

#include "core/mathx.h"

/* Block-grid world for platformer portals: one tile = one metre, tile
 * (x, y, z) covers [x, x+1) x [y, y+1) x [z, z+1). Outside the grid the sides
 * and the top count as rock, below y = 0 is the abyss. */

#define CADV_TILES_MAX (48 * 18 * 104)

typedef enum cadv_tile {
    CADV_TILE_AIR = 0,
    CADV_TILE_ROCK,
    CADV_TILE_ICE,      /* solid; portals can melt it */
    CADV_TILE_BLOCK,    /* solid and pushable */
    CADV_TILE_DOOR,     /* solid until opened */
    CADV_TILE_GRATE,    /* solid until opened (lets water through visually) */
    CADV_TILE_WATER,
    CADV_TILE_KINDS
} cadv_tile;

typedef struct cadv_tiles {
    int w, h, d;
    uint8_t t[CADV_TILES_MAX];
    uint32_t version;   /* bumped on every change (for re-meshing) */
} cadv_tiles;

bool cadv_tiles_init(cadv_tiles *g, int w, int h, int d, cadv_tile fill);
cadv_tile cadv_tiles_get(const cadv_tiles *g, int x, int y, int z);
void cadv_tiles_set(cadv_tiles *g, int x, int y, int z, cadv_tile t);
void cadv_tiles_fill(cadv_tiles *g, int x0, int y0, int z0, int x1, int y1, int z1, cadv_tile t);
bool cadv_tile_solid(cadv_tile t);
bool cadv_tiles_solid_at(const cadv_tiles *g, int x, int y, int z);
/* Tile containing a world point (floor of each coordinate). */
cadv_tile cadv_tiles_at(const cadv_tiles *g, cadv_vec3 p);
/* True when the segment a-b crosses no solid tile (3D DDA). */
bool cadv_tiles_line_clear(const cadv_tiles *g, cadv_vec3 a, cadv_vec3 b);
/* Height of the walkable top under (x, z) at or below 'y' (the top face of the
 * highest solid tile whose top is <= y + 0.01), or -1000 over the abyss. */
float cadv_tiles_floor(const cadv_tiles *g, float x, float y, float z);

#endif
