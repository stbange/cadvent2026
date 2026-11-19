/* SPDX-License-Identifier: MIT */
#include "kits/rts/grid.h"

#include <string.h>

bool cadv_rts_grid_init(cadv_rts_grid *g, int w, int h, float size)
{
    int i;
    if (w < 1 || h < 1 || w > CADV_RTS_GRID_MAX || h > CADV_RTS_GRID_MAX || size <= 0.0f) {
        return false;
    }
    memset(g, 0, sizeof *g);
    g->w = w;
    g->h = h;
    g->size = size;
    for (i = 0; i < w * h; ++i) {
        g->owner[i] = -1;
        g->building[i] = -1;
    }
    return true;
}

bool cadv_rts_in_bounds(const cadv_rts_grid *g, int x, int z) { return x >= 0 && z >= 0 && x < g->w && z < g->h; }

int cadv_rts_index(const cadv_rts_grid *g, int x, int z) { return z * g->w + x; }

cadv_cell cadv_rts_cell_at(const cadv_rts_grid *g, float wx, float wz)
{
    cadv_cell c;
    float fx = wx / g->size;
    float fz = wz / g->size;
    c.x = (int)fx - (fx < 0.0f && (float)(int)fx != fx ? 1 : 0);
    c.z = (int)fz - (fz < 0.0f && (float)(int)fz != fz ? 1 : 0);
    return c;
}

void cadv_rts_cell_centre(const cadv_rts_grid *g, cadv_cell c, float *wx, float *wz)
{
    *wx = ((float)c.x + 0.5f) * g->size;
    *wz = ((float)c.z + 0.5f) * g->size;
}

void cadv_rts_set_blocked(cadv_rts_grid *g, int x0, int z0, int x1, int z1)
{
    int x;
    int z;
    for (z = z0; z <= z1; ++z) {
        for (x = x0; x <= x1; ++x) {
            if (cadv_rts_in_bounds(g, x, z)) {
                g->kind[cadv_rts_index(g, x, z)] = CADV_CELL_BLOCKED;
            }
        }
    }
}

void cadv_rts_add_ore(cadv_rts_grid *g, int cx, int cz, int radius, int amount)
{
    int x;
    int z;
    for (z = cz - radius; z <= cz + radius; ++z) {
        for (x = cx - radius; x <= cx + radius; ++x) {
            int dx = x - cx;
            int dz = z - cz;
            if (cadv_rts_in_bounds(g, x, z) && dx * dx + dz * dz <= radius * radius &&
                g->kind[cadv_rts_index(g, x, z)] == CADV_CELL_OPEN) {
                int i = cadv_rts_index(g, x, z);
                int v = (int)g->ore[i] + amount;
                g->ore[i] = (uint16_t)(v > 65535 ? 65535 : v);
            }
        }
    }
}

bool cadv_rts_passable(const cadv_rts_grid *g, int x, int z)
{
    return cadv_rts_in_bounds(g, x, z) && g->kind[cadv_rts_index(g, x, z)] == CADV_CELL_OPEN;
}

bool cadv_rts_passable_for(const cadv_rts_grid *g, int x, int z, bool naval)
{
    return cadv_rts_in_bounds(g, x, z) && g->kind[cadv_rts_index(g, x, z)] == (naval ? CADV_CELL_WATER : CADV_CELL_OPEN);
}

void cadv_rts_add_resource(cadv_rts_grid *g, int x0, int z0, int x1, int z1, int kind, int amount, bool blocking)
{
    int x;
    int z;
    for (z = z0; z <= z1; ++z) {
        for (x = x0; x <= x1; ++x) {
            if (cadv_rts_in_bounds(g, x, z)) {
                int i = cadv_rts_index(g, x, z);
                g->ore[i] = (uint16_t)(amount > 65535 ? 65535 : amount);
                g->ore_kind[i] = (uint8_t)kind;
                g->ore_block[i] = blocking ? 1u : 0u;
                if (blocking) {
                    g->kind[i] = CADV_CELL_BLOCKED;
                }
            }
        }
    }
    ++g->version;
}

void cadv_rts_set_water(cadv_rts_grid *g, int x0, int z0, int x1, int z1, bool water)
{
    int x;
    int z;
    for (z = z0; z <= z1; ++z) {
        for (x = x0; x <= x1; ++x) {
            if (cadv_rts_in_bounds(g, x, z)) {
                int i = cadv_rts_index(g, x, z);
                if (g->kind[i] == CADV_CELL_OPEN || g->kind[i] == CADV_CELL_WATER) {
                    g->kind[i] = water ? CADV_CELL_WATER : CADV_CELL_OPEN;
                }
            }
        }
    }
    ++g->version;
}

bool cadv_rts_nearest_resource(const cadv_rts_grid *g, int x, int z, int kind, cadv_cell *out)
{
    int cx;
    int cz;
    int best = -1;
    for (cz = 0; cz < g->h; ++cz) {
        for (cx = 0; cx < g->w; ++cx) {
            int i = cadv_rts_index(g, cx, cz);
            int d = (cx - x) * (cx - x) + (cz - z) * (cz - z);
            if (g->ore[i] > 0 && g->ore_kind[i] == kind && (best < 0 || d < best)) {
                best = d;
                out->x = cx;
                out->z = cz;
            }
        }
    }
    return best >= 0;
}

cadv_place_result cadv_rts_can_place(const cadv_rts_grid *g, int x, int z, int fw, int fh, int owner, int reach, bool first)
{
    int cx;
    int cz;
    bool near = first;
    if (fw < 1 || fh < 1 || !cadv_rts_in_bounds(g, x, z) || !cadv_rts_in_bounds(g, x + fw - 1, z + fh - 1)) {
        return CADV_PLACE_OUT_OF_BOUNDS;
    }
    for (cz = z; cz < z + fh; ++cz) {
        for (cx = x; cx < x + fw; ++cx) {
            int i = cadv_rts_index(g, cx, cz);
            if (g->kind[i] != CADV_CELL_OPEN || g->ore[i] > 0) {
                return CADV_PLACE_BLOCKED;
            }
        }
    }
    for (cz = z - reach; cz < z + fh + reach && !near; ++cz) {
        for (cx = x - reach; cx < x + fw + reach && !near; ++cx) {
            if (cadv_rts_in_bounds(g, cx, cz)) {
                int i = cadv_rts_index(g, cx, cz);
                near = g->kind[i] == CADV_CELL_BUILDING && g->owner[i] == owner;
            }
        }
    }
    return near ? CADV_PLACE_OK : CADV_PLACE_TOO_FAR;
}

void cadv_rts_place(cadv_rts_grid *g, int x, int z, int fw, int fh, int owner, int id)
{
    int cx;
    int cz;
    for (cz = z; cz < z + fh; ++cz) {
        for (cx = x; cx < x + fw; ++cx) {
            if (cadv_rts_in_bounds(g, cx, cz)) {
                int i = cadv_rts_index(g, cx, cz);
                g->kind[i] = CADV_CELL_BUILDING;
                g->owner[i] = (int8_t)owner;
                g->building[i] = (int16_t)id;
            }
        }
    }
}

void cadv_rts_remove(cadv_rts_grid *g, int id)
{
    int i;
    for (i = 0; i < g->w * g->h; ++i) {
        if (g->building[i] == id) {
            g->kind[i] = CADV_CELL_OPEN;
            g->owner[i] = -1;
            g->building[i] = -1;
        }
    }
}

int cadv_rts_mine(cadv_rts_grid *g, int x, int z, int amount)
{
    int i;
    int take;
    if (!cadv_rts_in_bounds(g, x, z)) {
        return 0;
    }
    i = cadv_rts_index(g, x, z);
    take = amount < (int)g->ore[i] ? amount : (int)g->ore[i];
    g->ore[i] = (uint16_t)((int)g->ore[i] - take);
    if (g->ore[i] == 0 && g->ore_block[i]) {
        g->ore_block[i] = 0; /* a felled forest opens a path */
        g->kind[i] = CADV_CELL_OPEN;
        ++g->version;
    }
    return take;
}

bool cadv_rts_nearest_ore(const cadv_rts_grid *g, int x, int z, cadv_cell *out)
{
    int cx;
    int cz;
    int best = -1;
    for (cz = 0; cz < g->h; ++cz) {
        for (cx = 0; cx < g->w; ++cx) {
            int d = (cx - x) * (cx - x) + (cz - z) * (cz - z);
            int i = cadv_rts_index(g, cx, cz);
            if (g->ore[i] > 0 && g->ore_kind[i] == 0 && (best < 0 || d < best)) {
                best = d;
                out->x = cx;
                out->z = cz;
            }
        }
    }
    return best >= 0;
}

/* ---- A* ---------------------------------------------------------------------- */

typedef struct node {
    int g;       /* cost so far (10 straight, 14 diagonal) */
    int f;
    int parent;  /* index, -1 = start */
    uint8_t state; /* 0 new, 1 open, 2 closed */
} node;

static int heuristic(int x, int z, cadv_cell b)
{
    int dx = x > b.x ? x - b.x : b.x - x;
    int dz = z > b.z ? z - b.z : b.z - z;
    int mn = dx < dz ? dx : dz;
    int mx = dx < dz ? dz : dx;
    return 14 * mn + 10 * (mx - mn);
}

/* Next to the goal cell, or (when the goal is part of a building) next to
 * any cell of that building. */
static bool touches(const cadv_rts_grid *g, int x, int z, cadv_cell b, int building)
{
    int dx;
    int dz;
    for (dz = -1; dz <= 1; ++dz) {
        for (dx = -1; dx <= 1; ++dx) {
            int nx = x + dx;
            int nz = z + dz;
            if ((nx == b.x && nz == b.z) ||
                (building >= 0 && cadv_rts_in_bounds(g, nx, nz) && g->building[cadv_rts_index(g, nx, nz)] == building)) {
                return true;
            }
        }
    }
    return false;
}

int cadv_rts_path(const cadv_rts_grid *g, cadv_cell a, cadv_cell b, cadv_cell *out, int max)
{
    return cadv_rts_path_for(g, a, b, out, max, false);
}

int cadv_rts_path_for(const cadv_rts_grid *g, cadv_cell a, cadv_cell b, cadv_cell *out, int max, bool naval)
{
    static const int dx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int dz[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    static node nodes[CADV_RTS_GRID_MAX * CADV_RTS_GRID_MAX];
    static int open[CADV_RTS_GRID_MAX * CADV_RTS_GRID_MAX];
    int nopen = 0;
    int goal = -1;
    bool goal_solid;
    int goal_building;
    int i;
    if (!cadv_rts_in_bounds(g, a.x, a.z) || !cadv_rts_in_bounds(g, b.x, b.z)) {
        return -1;
    }
    if (a.x == b.x && a.z == b.z) {
        return 0;
    }
    goal_solid = !cadv_rts_passable_for(g, b.x, b.z, naval);
    goal_building = g->building[cadv_rts_index(g, b.x, b.z)];
    memset(nodes, 0, sizeof(node) * (size_t)(g->w * g->h));
    i = cadv_rts_index(g, a.x, a.z);
    nodes[i].parent = -1;
    nodes[i].f = heuristic(a.x, a.z, b);
    nodes[i].state = 1;
    open[nopen++] = i;
    while (nopen > 0) {
        int bi = 0;
        int cur;
        int cx;
        int cz;
        int k;
        for (k = 1; k < nopen; ++k) {
            if (nodes[open[k]].f < nodes[open[bi]].f) {
                bi = k;
            }
        }
        cur = open[bi];
        open[bi] = open[--nopen];
        nodes[cur].state = 2;
        cx = cur % g->w;
        cz = cur / g->w;
        if ((cx == b.x && cz == b.z) || (goal_solid && touches(g, cx, cz, b, goal_building))) {
            goal = cur;
            break;
        }
        for (k = 0; k < 8; ++k) {
            int nx = cx + dx[k];
            int nz = cz + dz[k];
            int ni;
            int cost;
            if (!cadv_rts_passable_for(g, nx, nz, naval)) {
                continue;
            }
            if (k >= 4 && (!cadv_rts_passable_for(g, nx, cz, naval) || !cadv_rts_passable_for(g, cx, nz, naval))) {
                continue; /* no squeezing past corners */
            }
            ni = cadv_rts_index(g, nx, nz);
            cost = nodes[cur].g + (k >= 4 ? 14 : 10);
            if (nodes[ni].state == 2 || (nodes[ni].state == 1 && cost >= nodes[ni].g)) {
                continue;
            }
            nodes[ni].g = cost;
            nodes[ni].f = cost + heuristic(nx, nz, b);
            nodes[ni].parent = cur;
            if (nodes[ni].state == 0) {
                nodes[ni].state = 1;
                open[nopen++] = ni;
            }
        }
    }
    if (goal < 0) {
        return -1;
    }
    {
        int len = 0;
        int n;
        for (n = goal; nodes[n].parent >= 0; n = nodes[n].parent) {
            ++len;
        }
        if (len > max) {
            return -1;
        }
        n = goal;
        for (i = len - 1; i >= 0; --i) {
            out[i].x = n % g->w;
            out[i].z = n / g->w;
            n = nodes[n].parent;
        }
        return len;
    }
}
