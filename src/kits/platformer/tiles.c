/* SPDX-License-Identifier: MIT */
#include "kits/platformer/tiles.h"

#include <string.h>

static int ifloor(float v)
{
    int i = (int)v;
    return (v < 0.0f && (float)i != v) ? i - 1 : i;
}

bool cadv_tiles_init(cadv_tiles *g, int w, int h, int d, cadv_tile fill)
{
    if (w < 1 || h < 1 || d < 1 || w * h * d > CADV_TILES_MAX) {
        return false;
    }
    memset(g, 0, sizeof *g);
    g->w = w;
    g->h = h;
    g->d = d;
    memset(g->t, (int)fill, (size_t)(w * h * d));
    g->version = 1;
    return true;
}

cadv_tile cadv_tiles_get(const cadv_tiles *g, int x, int y, int z)
{
    if (y < 0) {
        return CADV_TILE_AIR; /* the abyss */
    }
    if (x < 0 || z < 0 || x >= g->w || z >= g->d || y >= g->h) {
        return CADV_TILE_ROCK;
    }
    return (cadv_tile)g->t[(y * g->d + z) * g->w + x];
}

void cadv_tiles_set(cadv_tiles *g, int x, int y, int z, cadv_tile t)
{
    if (x >= 0 && y >= 0 && z >= 0 && x < g->w && y < g->h && z < g->d) {
        uint8_t *c = &g->t[(y * g->d + z) * g->w + x];
        if (*c != (uint8_t)t) {
            *c = (uint8_t)t;
            ++g->version;
        }
    }
}

void cadv_tiles_fill(cadv_tiles *g, int x0, int y0, int z0, int x1, int y1, int z1, cadv_tile t)
{
    int x;
    int y;
    int z;
    for (y = y0; y <= y1; ++y) {
        for (z = z0; z <= z1; ++z) {
            for (x = x0; x <= x1; ++x) {
                cadv_tiles_set(g, x, y, z, t);
            }
        }
    }
}

bool cadv_tile_solid(cadv_tile t) { return t != CADV_TILE_AIR && t != CADV_TILE_WATER; }

bool cadv_tiles_solid_at(const cadv_tiles *g, int x, int y, int z) { return cadv_tile_solid(cadv_tiles_get(g, x, y, z)); }

cadv_tile cadv_tiles_at(const cadv_tiles *g, cadv_vec3 p) { return cadv_tiles_get(g, ifloor(p.x), ifloor(p.y), ifloor(p.z)); }

bool cadv_tiles_line_clear(const cadv_tiles *g, cadv_vec3 a, cadv_vec3 b)
{
    cadv_vec3 d = cadv_v3_sub(b, a);
    float len = cadv_v3_len(d);
    int steps;
    int i;
    if (len <= 0.0f) {
        return !cadv_tile_solid(cadv_tiles_at(g, a));
    }
    steps = (int)(len / 0.2f) + 1;
    for (i = 0; i <= steps; ++i) {
        cadv_vec3 p = cadv_v3_add(a, cadv_v3_scale(d, (float)i / (float)steps));
        if (cadv_tile_solid(cadv_tiles_at(g, p))) {
            return false;
        }
    }
    return true;
}

float cadv_tiles_floor(const cadv_tiles *g, float x, float y, float z)
{
    int ix = ifloor(x);
    int iz = ifloor(z);
    int iy = ifloor(y + 0.01f) - 1;
    for (; iy >= 0; --iy) {
        if (cadv_tiles_solid_at(g, ix, iy, iz)) {
            return (float)(iy + 1);
        }
    }
    return -1000.0f;
}
