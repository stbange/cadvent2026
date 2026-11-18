/* SPDX-License-Identifier: MIT */
#include "kits/vehicle/terrain.h"

#include <string.h>

#include "gfx/procgen.h"

bool cadv_terrain_init(cadv_terrain *t, int n, float cell, float min_h, float max_h, float sea)
{
    int i;
    if (n < 2 || n > CADV_TERRAIN_MAX_N || ((n - 1) % CADV_TERRAIN_CHUNK) != 0 || cell <= 0.0f || max_h <= min_h) {
        return false;
    }
    memset(t, 0, sizeof *t);
    t->n = n;
    t->cell = cell;
    t->min_h = min_h;
    t->max_h = max_h;
    t->sea = sea;
    for (i = 0; i < n * n; ++i) {
        t->h[i] = min_h;
    }
    cadv_terrain_mark_all_dirty(t);
    return true;
}

float cadv_terrain_size(const cadv_terrain *t) { return (float)(t->n - 1) * t->cell; }

int cadv_terrain_chunks_per_side(const cadv_terrain *t) { return (t->n - 1) / CADV_TERRAIN_CHUNK; }

void cadv_terrain_mark_all_dirty(cadv_terrain *t)
{
    int c = cadv_terrain_chunks_per_side(t);
    int i;
    for (i = 0; i < c * c; ++i) {
        ++t->chunk_ver[i];
    }
}

static void mark_dirty(cadv_terrain *t, int ix, int iz)
{
    int c = cadv_terrain_chunks_per_side(t);
    int cx;
    int cz;
    int dx;
    int dz;
    /* a sample on a chunk border belongs to both neighbours */
    for (dz = -1; dz <= 0; ++dz) {
        for (dx = -1; dx <= 0; ++dx) {
            cx = (ix + dx) / CADV_TERRAIN_CHUNK;
            cz = (iz + dz) / CADV_TERRAIN_CHUNK;
            if (ix + dx >= 0 && iz + dz >= 0 && cx < c && cz < c) {
                ++t->chunk_ver[cz * c + cx];
            }
        }
    }
}

void cadv_terrain_islands(cadv_terrain *t, const cadv_island *isl, int count, uint32_t seed)
{
    int ix;
    int iz;
    int i;
    float floor_h = t->min_h + 2.0f;
    for (iz = 0; iz < t->n; ++iz) {
        for (ix = 0; ix < t->n; ++ix) {
            float x = (float)ix * t->cell;
            float z = (float)iz * t->cell;
            float best = floor_h;
            float detail = cadv_fbm2(seed, (float)ix / 16.0f, (float)iz / 16.0f, 1 << 20, 4);
            for (i = 0; i < count; ++i) {
                float dx = (x - isl[i].x) / isl[i].radius;
                float dz = (z - isl[i].z) / isl[i].radius;
                float d = cadv_sqrtf(dx * dx + dz * dz);
                float k;
                float hgt;
                if (d >= 1.0f) {
                    continue;
                }
                /* plateau, then a smooth shoulder down to the sea floor */
                k = d <= isl[i].flat ? 1.0f : 1.0f - (d - isl[i].flat) / (1.0f - isl[i].flat);
                k = k * k * (3.0f - 2.0f * k);
                hgt = floor_h + isl[i].height * k * (0.85f + 0.3f * detail);
                if (hgt > best) {
                    best = hgt;
                }
            }
            t->h[iz * t->n + ix] = cadv_clampf(best, t->min_h, t->max_h);
        }
    }
    cadv_terrain_mark_all_dirty(t);
}

float cadv_terrain_sample(const cadv_terrain *t, int ix, int iz)
{
    ix = cadv_maxi(0, cadv_mini(t->n - 1, ix));
    iz = cadv_maxi(0, cadv_mini(t->n - 1, iz));
    return t->h[iz * t->n + ix];
}

float cadv_terrain_height(const cadv_terrain *t, float x, float z)
{
    float fx = cadv_clampf(x / t->cell, 0.0f, (float)(t->n - 1));
    float fz = cadv_clampf(z / t->cell, 0.0f, (float)(t->n - 1));
    int ix = (int)fx;
    int iz = (int)fz;
    float ax;
    float az;
    float a, b, c, d;
    if (ix >= t->n - 1) {
        ix = t->n - 2;
    }
    if (iz >= t->n - 1) {
        iz = t->n - 2;
    }
    ax = fx - (float)ix;
    az = fz - (float)iz;
    a = t->h[iz * t->n + ix];
    b = t->h[iz * t->n + ix + 1];
    c = t->h[(iz + 1) * t->n + ix];
    d = t->h[(iz + 1) * t->n + ix + 1];
    return cadv_lerpf(cadv_lerpf(a, b, ax), cadv_lerpf(c, d, ax), az);
}

float cadv_terrain_surface(const cadv_terrain *t, float x, float z)
{
    return cadv_maxf(cadv_terrain_height(t, x, z), t->sea);
}

cadv_vec3 cadv_terrain_normal(const cadv_terrain *t, float x, float z)
{
    float e = t->cell;
    float hx = cadv_terrain_height(t, x + e, z) - cadv_terrain_height(t, x - e, z);
    float hz = cadv_terrain_height(t, x, z + e) - cadv_terrain_height(t, x, z - e);
    return cadv_v3_norm(cadv_v3(-hx, 2.0f * e, -hz));
}

typedef struct span { int x0, x1, z0, z1; } span;

static span span_of(const cadv_terrain *t, float x, float z, float radius)
{
    span s;
    s.x0 = cadv_maxi(0, (int)((x - radius) / t->cell));
    s.x1 = cadv_mini(t->n - 1, (int)((x + radius) / t->cell) + 1);
    s.z0 = cadv_maxi(0, (int)((z - radius) / t->cell));
    s.z1 = cadv_mini(t->n - 1, (int)((z + radius) / t->cell) + 1);
    return s;
}

static float falloff(const cadv_terrain *t, int ix, int iz, float x, float z, float radius)
{
    float dx = (float)ix * t->cell - x;
    float dz = (float)iz * t->cell - z;
    float d = cadv_sqrtf(dx * dx + dz * dz) / radius;
    float k;
    if (d >= 1.0f) {
        return 0.0f;
    }
    k = 1.0f - d;
    return k * k * (3.0f - 2.0f * k);
}

float cadv_terrain_deform(cadv_terrain *t, float x, float z, float radius, float amount)
{
    span s;
    int ix;
    int iz;
    float moved = 0.0f;
    if (radius <= 0.0f) {
        return 0.0f;
    }
    s = span_of(t, x, z, radius);
    for (iz = s.z0; iz <= s.z1; ++iz) {
        for (ix = s.x0; ix <= s.x1; ++ix) {
            float k = falloff(t, ix, iz, x, z, radius);
            float *hp = &t->h[iz * t->n + ix];
            float old = *hp;
            if (k <= 0.0f) {
                continue;
            }
            *hp = cadv_clampf(old + amount * k, t->min_h, t->max_h);
            if (*hp != old) {
                moved += cadv_absf(*hp - old);
                mark_dirty(t, ix, iz);
            }
        }
    }
    return moved;
}

void cadv_terrain_raise_to(cadv_terrain *t, float x, float z, float radius, float height)
{
    span s = span_of(t, x, z, radius);
    int ix;
    int iz;
    height = cadv_clampf(height, t->min_h, t->max_h);
    for (iz = s.z0; iz <= s.z1; ++iz) {
        for (ix = s.x0; ix <= s.x1; ++ix) {
            float k = falloff(t, ix, iz, x, z, radius);
            float *hp = &t->h[iz * t->n + ix];
            float want = *hp + (height - *hp) * k;
            if (k > 0.0f && want > *hp) {
                *hp = want;
                mark_dirty(t, ix, iz);
            }
        }
    }
}

void cadv_terrain_flatten(cadv_terrain *t, float x, float z, float radius, float target, float rate)
{
    span s = span_of(t, x, z, radius);
    int ix;
    int iz;
    target = cadv_clampf(target, t->min_h, t->max_h);
    for (iz = s.z0; iz <= s.z1; ++iz) {
        for (ix = s.x0; ix <= s.x1; ++ix) {
            float k = falloff(t, ix, iz, x, z, radius);
            float *hp = &t->h[iz * t->n + ix];
            float diff = target - *hp;
            float step = rate * k;
            if (k <= 0.0f || diff == 0.0f) {
                continue;
            }
            *hp += cadv_clampf(diff, -step, step);
            mark_dirty(t, ix, iz);
        }
    }
}

float cadv_terrain_roughness(const cadv_terrain *t, float x, float z, float radius)
{
    span s = span_of(t, x, z, radius);
    float lo = 1e9f;
    float hi = -1e9f;
    int ix;
    int iz;
    for (iz = s.z0; iz <= s.z1; ++iz) {
        for (ix = s.x0; ix <= s.x1; ++ix) {
            float v = t->h[iz * t->n + ix];
            lo = cadv_minf(lo, v);
            hi = cadv_maxf(hi, v);
        }
    }
    return hi - lo;
}

bool cadv_terrain_raycast(const cadv_terrain *t, cadv_vec3 origin, cadv_vec3 dir, float max_dist, cadv_vec3 *hit)
{
    float step = t->cell * 0.5f;
    float d;
    float prev = 0.0f;
    dir = cadv_v3_norm(dir);
    if (origin.y <= cadv_terrain_height(t, origin.x, origin.z)) {
        *hit = origin;
        return true;
    }
    for (d = step; d <= max_dist + step * 0.5f; d += step) {
        cadv_vec3 p = cadv_v3_add(origin, cadv_v3_scale(dir, cadv_minf(d, max_dist)));
        if (p.y <= cadv_terrain_height(t, p.x, p.z)) {
            /* refine between prev and d */
            float a = prev;
            float b = cadv_minf(d, max_dist);
            int i;
            for (i = 0; i < 8; ++i) {
                float m = 0.5f * (a + b);
                cadv_vec3 q = cadv_v3_add(origin, cadv_v3_scale(dir, m));
                if (q.y <= cadv_terrain_height(t, q.x, q.z)) {
                    b = m;
                } else {
                    a = m;
                }
            }
            *hit = cadv_v3_add(origin, cadv_v3_scale(dir, b));
            return true;
        }
        prev = d;
    }
    return false;
}

bool cadv_terrain_line_clear(const cadv_terrain *t, cadv_vec3 a, cadv_vec3 b)
{
    cadv_vec3 hit;
    float len = cadv_v3_dist(a, b);
    if (len <= 0.0f) {
        return a.y > cadv_terrain_height(t, a.x, a.z);
    }
    return !cadv_terrain_raycast(t, a, cadv_v3_sub(b, a), len, &hit);
}
