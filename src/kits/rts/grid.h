/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_RTS_GRID_H
#define CADV_KITS_RTS_GRID_H

#include <stdbool.h>
#include <stdint.h>

/* Battlefield grid for RTS portals: terrain passability, building footprints
 * and resource fields per cell, building placement rules and A* paths.
 * Cells are square; cell (x, z) covers world [x*size, (x+1)*size). */

#define CADV_RTS_GRID_MAX 96
#define CADV_RTS_PATH_MAX 256

enum {
    CADV_CELL_OPEN = 0,
    CADV_CELL_BLOCKED = 1, /* cliffs, rocks, trees, mines */
    CADV_CELL_BUILDING = 2,
    CADV_CELL_WATER = 3    /* only naval units sail here */
};

/* Resource kinds a cell can hold (0 is the classic single resource). */
#define CADV_RTS_RESOURCES 3

typedef struct cadv_rts_grid {
    int w, h;
    float size;
    uint8_t kind[CADV_RTS_GRID_MAX * CADV_RTS_GRID_MAX];
    int8_t owner[CADV_RTS_GRID_MAX * CADV_RTS_GRID_MAX];     /* building owner, -1 = none */
    int16_t building[CADV_RTS_GRID_MAX * CADV_RTS_GRID_MAX]; /* building id, -1 = none */
    uint16_t ore[CADV_RTS_GRID_MAX * CADV_RTS_GRID_MAX];     /* resource left in the cell */
    uint8_t ore_kind[CADV_RTS_GRID_MAX * CADV_RTS_GRID_MAX]; /* which resource (0..2) */
    uint8_t ore_block[CADV_RTS_GRID_MAX * CADV_RTS_GRID_MAX];/* 1: blocks movement until depleted (trees) */
    uint32_t version;                                         /* bumped when passability changes */
} cadv_rts_grid;

typedef struct cadv_cell { int x, z; } cadv_cell;

typedef enum cadv_place_result {
    CADV_PLACE_OK = 0,
    CADV_PLACE_OUT_OF_BOUNDS,
    CADV_PLACE_BLOCKED,     /* overlaps terrain, another building or ore */
    CADV_PLACE_TOO_FAR      /* not adjacent to one of the owner's buildings */
} cadv_place_result;

bool cadv_rts_grid_init(cadv_rts_grid *g, int w, int h, float size);
bool cadv_rts_in_bounds(const cadv_rts_grid *g, int x, int z);
int cadv_rts_index(const cadv_rts_grid *g, int x, int z);
cadv_cell cadv_rts_cell_at(const cadv_rts_grid *g, float wx, float wz);
void cadv_rts_cell_centre(const cadv_rts_grid *g, cadv_cell c, float *wx, float *wz);
void cadv_rts_set_blocked(cadv_rts_grid *g, int x0, int z0, int x1, int z1);
void cadv_rts_add_ore(cadv_rts_grid *g, int cx, int cz, int radius, int amount);
/* Passable for ground units (open ore cells are passable). */
bool cadv_rts_passable(const cadv_rts_grid *g, int x, int z);
/* Passable for naval units (water) or ground units. */
bool cadv_rts_passable_for(const cadv_rts_grid *g, int x, int z, bool naval);
/* Resource of 'kind' on every cell of a rectangle; 'blocking' resources
 * (trees, a mine, a platform) are impassable until mined out. */
void cadv_rts_add_resource(cadv_rts_grid *g, int x0, int z0, int x1, int z1, int kind, int amount, bool blocking);
void cadv_rts_set_water(cadv_rts_grid *g, int x0, int z0, int x1, int z1, bool water);
/* Nearest cell holding resource 'kind'; false if none left. */
bool cadv_rts_nearest_resource(const cadv_rts_grid *g, int x, int z, int kind, cadv_cell *out);
/* Placement: inside the map, every footprint cell open and free of ore, and
 * (unless 'first') within 'reach' cells of a building the owner already has. */
cadv_place_result cadv_rts_can_place(const cadv_rts_grid *g, int x, int z, int fw, int fh, int owner, int reach, bool first);
void cadv_rts_place(cadv_rts_grid *g, int x, int z, int fw, int fh, int owner, int id);
void cadv_rts_remove(cadv_rts_grid *g, int id);
/* Takes up to 'amount' ore from the cell; returns what was taken. */
int cadv_rts_mine(cadv_rts_grid *g, int x, int z, int amount);
/* Nearest cell with ore to (x, z) by squared distance; false if none left. */
bool cadv_rts_nearest_ore(const cadv_rts_grid *g, int x, int z, cadv_cell *out);
/* 8-way A* from a to b over passable cells (no corner cutting). Writes the
 * cells after 'a' up to and including 'b'; returns the count, 0 if a == b,
 * -1 if unreachable or the path is longer than 'max'. When b itself is not
 * passable (a building), the path ends next to it. */
int cadv_rts_path(const cadv_rts_grid *g, cadv_cell a, cadv_cell b, cadv_cell *out, int max);
/* The same for naval units (over water cells). */
int cadv_rts_path_for(const cadv_rts_grid *g, cadv_cell a, cadv_cell b, cadv_cell *out, int max, bool naval);

#endif
