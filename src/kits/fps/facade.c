/* SPDX-License-Identifier: MIT */
#include "kits/fps/facade.h"

#include <string.h>

bool cadv_facade_build(cadv_facade *f, cadv_world *w, cadv_vec3 origin, int axis, int cols, int rows, float size, float depth, int hp)
{
    int x;
    int y;
    memset(f, 0, sizeof *f);
    if (cols < 1 || rows < 1 || cols > CADV_FACADE_W || rows > CADV_FACADE_H) {
        return false;
    }
    f->origin = origin;
    f->axis = axis;
    f->w = cols;
    f->h = rows;
    f->size = size;
    f->depth = depth;
    for (y = 0; y < rows; ++y) {
        for (x = 0; x < cols; ++x) {
            int i = y * cols + x;
            cadv_vec3 lo = axis == 0 ? cadv_v3(origin.x + (float)x * size, origin.y + (float)y * size, origin.z)
                                     : cadv_v3(origin.x, origin.y + (float)y * size, origin.z + (float)x * size);
            cadv_vec3 hi = axis == 0 ? cadv_v3(lo.x + size, lo.y + size, lo.z + depth) : cadv_v3(lo.x + depth, lo.y + size, lo.z + size);
            f->hp[i] = (uint8_t)(hp > 255 ? 255 : hp);
            f->solid[i] = (int16_t)cadv_world_add(w, lo, hi, 0);
            ++f->alive;
        }
    }
    return true;
}

bool cadv_facade_alive(const cadv_facade *f, int x, int y)
{
    return x >= 0 && y >= 0 && x < f->w && y < f->h && f->hp[y * f->w + x] > 0;
}

cadv_vec3 cadv_facade_centre(const cadv_facade *f, int x, int y)
{
    float a = ((float)x + 0.5f) * f->size;
    float up = ((float)y + 0.5f) * f->size;
    return f->axis == 0 ? cadv_v3(f->origin.x + a, f->origin.y + up, f->origin.z + f->depth * 0.5f)
                        : cadv_v3(f->origin.x + f->depth * 0.5f, f->origin.y + up, f->origin.z + a);
}

static void remove_chunk(cadv_facade *f, cadv_world *w, int i)
{
    f->hp[i] = 0;
    if (f->solid[i] >= 0) {
        w->s[f->solid[i]].enabled = false;
    }
    --f->alive;
}

int cadv_facade_blast(cadv_facade *f, cadv_world *w, cadv_vec3 at, float radius, int damage)
{
    int x;
    int y;
    int gone = 0;
    bool changed = true;
    for (y = 0; y < f->h; ++y) {
        for (x = 0; x < f->w; ++x) {
            int i = y * f->w + x;
            float d = cadv_v3_dist(cadv_facade_centre(f, x, y), at);
            int dmg;
            if (f->hp[i] == 0 || d > radius) {
                continue;
            }
            dmg = (int)((float)damage * (1.0f - 0.5f * d / radius));
            if (dmg >= f->hp[i]) {
                remove_chunk(f, w, i);
                ++gone;
            } else {
                f->hp[i] = (uint8_t)(f->hp[i] - dmg);
            }
        }
    }
    /* loose chunks: nothing below and nothing to either side */
    while (gone > 0 && changed) {
        changed = false;
        for (y = 1; y < f->h; ++y) {
            for (x = 0; x < f->w; ++x) {
                if (cadv_facade_alive(f, x, y) && !cadv_facade_alive(f, x, y - 1) && !cadv_facade_alive(f, x - 1, y) && !cadv_facade_alive(f, x + 1, y)) {
                    remove_chunk(f, w, y * f->w + x);
                    ++gone;
                    changed = true;
                }
            }
        }
    }
    if (gone > 0) {
        ++f->ver;
    }
    return gone;
}
