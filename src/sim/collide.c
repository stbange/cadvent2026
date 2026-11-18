/* SPDX-License-Identifier: MIT */
#include "sim/collide.h"

#include <string.h>

#define SKIN 0.001f

cadv_aabb cadv_aabb_make(cadv_vec3 lo, cadv_vec3 hi)
{
    cadv_aabb b;
    b.lo = cadv_v3(cadv_minf(lo.x, hi.x), cadv_minf(lo.y, hi.y), cadv_minf(lo.z, hi.z));
    b.hi = cadv_v3(cadv_maxf(lo.x, hi.x), cadv_maxf(lo.y, hi.y), cadv_maxf(lo.z, hi.z));
    return b;
}

bool cadv_aabb_overlap(cadv_aabb a, cadv_aabb b)
{
    return a.lo.x < b.hi.x && a.hi.x > b.lo.x && a.lo.y < b.hi.y && a.hi.y > b.lo.y &&
           a.lo.z < b.hi.z && a.hi.z > b.lo.z;
}

bool cadv_aabb_contains(cadv_aabb a, cadv_vec3 p)
{
    return p.x >= a.lo.x && p.x <= a.hi.x && p.y >= a.lo.y && p.y <= a.hi.y && p.z >= a.lo.z &&
           p.z <= a.hi.z;
}

void cadv_world_clear(cadv_world *w) { w->count = 0; }

int cadv_world_add(cadv_world *w, cadv_vec3 lo, cadv_vec3 hi, int tag)
{
    cadv_solid *s;
    if (w->count >= CADV_MAX_SOLIDS) {
        return -1;
    }
    s = &w->s[w->count];
    s->box = cadv_aabb_make(lo, hi);
    s->tag = tag;
    s->enabled = true;
    return w->count++;
}

bool cadv_world_overlaps(const cadv_world *w, cadv_aabb box)
{
    int i;
    for (i = 0; i < w->count; ++i) {
        if (w->s[i].enabled && cadv_aabb_overlap(w->s[i].box, box)) {
            return true;
        }
    }
    return false;
}

bool cadv_ray_aabb(cadv_vec3 o, cadv_vec3 d, cadv_aabb b, float max_dist, float *t, cadv_vec3 *normal)
{
    float tmin = 0.0f;
    float tmax = max_dist;
    int axis = -1;
    float sign = 0.0f;
    int a;
    const float *op = &o.x;
    const float *dp = &d.x;
    const float *lo = &b.lo.x;
    const float *hi = &b.hi.x;
    for (a = 0; a < 3; ++a) {
        if (cadv_absf(dp[a]) < 1e-9f) {
            if (op[a] < lo[a] || op[a] > hi[a]) {
                return false;
            }
        } else {
            float inv = 1.0f / dp[a];
            float t1 = (lo[a] - op[a]) * inv;
            float t2 = (hi[a] - op[a]) * inv;
            float s = -1.0f;
            if (t1 > t2) {
                float tmp = t1;
                t1 = t2;
                t2 = tmp;
                s = 1.0f;
            }
            if (t1 > tmin) {
                tmin = t1;
                axis = a;
                sign = s;
            }
            if (t2 < tmax) {
                tmax = t2;
            }
            if (tmin > tmax) {
                return false;
            }
        }
    }
    *t = tmin;
    if (normal) {
        *normal = cadv_v3(0, 0, 0);
        if (axis == 0) {
            normal->x = sign;
        } else if (axis == 1) {
            normal->y = sign;
        } else if (axis == 2) {
            normal->z = sign;
        }
    }
    return true;
}

bool cadv_world_raycast(const cadv_world *w, cadv_vec3 origin, cadv_vec3 dir, float max_dist,
                        float *dist, cadv_vec3 *normal, int *solid)
{
    float best = max_dist;
    int hit = -1;
    cadv_vec3 bn = cadv_v3(0, 0, 0);
    int i;
    for (i = 0; i < w->count; ++i) {
        float t;
        cadv_vec3 n;
        if (w->s[i].enabled && cadv_ray_aabb(origin, dir, w->s[i].box, best, &t, &n) && t < best) {
            best = t;
            hit = i;
            bn = n;
        }
    }
    if (hit < 0) {
        return false;
    }
    if (dist) {
        *dist = best;
    }
    if (normal) {
        *normal = bn;
    }
    if (solid) {
        *solid = hit;
    }
    return true;
}

bool cadv_ray_cylinder(cadv_vec3 o, cadv_vec3 d, cadv_vec3 base, float radius, float height,
                       float max_dist, float *t)
{
    /* 2D circle intersection in XZ, then height check */
    float ox = o.x - base.x;
    float oz = o.z - base.z;
    float a = d.x * d.x + d.z * d.z;
    float b = 2.0f * (ox * d.x + oz * d.z);
    float c = ox * ox + oz * oz - radius * radius;
    float disc;
    float s;
    float y;
    if (a < 1e-9f) {
        return false;
    }
    disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) {
        return false;
    }
    s = (-b - cadv_sqrtf(disc)) / (2.0f * a);
    if (s < 0.0f) {
        s = (-b + cadv_sqrtf(disc)) / (2.0f * a);
    }
    if (s < 0.0f || s > max_dist) {
        return false;
    }
    y = o.y + d.y * s;
    if (y < base.y || y > base.y + height) {
        return false;
    }
    *t = s;
    return true;
}

bool cadv_world_line_of_sight(const cadv_world *w, cadv_vec3 a, cadv_vec3 b)
{
    cadv_vec3 d = cadv_v3_sub(b, a);
    float len = cadv_v3_len(d);
    if (len < 1e-6f) {
        return true;
    }
    return !cadv_world_raycast(w, a, cadv_v3_scale(d, 1.0f / len), len, NULL, NULL, NULL);
}

static cadv_aabb body(cadv_vec3 p, float r, float h)
{
    return cadv_aabb_make(cadv_v3(p.x - r, p.y, p.z - r), cadv_v3(p.x + r, p.y + h, p.z + r));
}

/* Moves along one axis and clamps against solids; returns true if blocked. */
static bool move_axis(const cadv_world *w, cadv_vec3 *pos, float r, float h, int axis, float amount)
{
    cadv_vec3 np = *pos;
    cadv_aabb b;
    int i;
    bool blocked = false;
    float *c = axis == 0 ? &np.x : (axis == 1 ? &np.y : &np.z);
    if (amount == 0.0f) {
        return false;
    }
    *c += amount;
    b = body(np, r, h);
    for (i = 0; i < w->count; ++i) {
        const cadv_aabb *s = &w->s[i].box;
        if (!w->s[i].enabled || !cadv_aabb_overlap(*s, b)) {
            continue;
        }
        blocked = true;
        if (axis == 0) {
            np.x = amount > 0.0f ? s->lo.x - r - SKIN : s->hi.x + r + SKIN;
        } else if (axis == 1) {
            np.y = amount > 0.0f ? s->lo.y - h - SKIN : s->hi.y + SKIN;
        } else {
            np.z = amount > 0.0f ? s->lo.z - r - SKIN : s->hi.z + r + SKIN;
        }
        b = body(np, r, h);
    }
    *pos = np;
    return blocked;
}

static cadv_move_result move_once(const cadv_world *w, cadv_vec3 *pos, float r, float h,
                                  cadv_vec3 delta, float step_h)
{
    cadv_move_result res;
    cadv_vec3 start = *pos;
    cadv_aabb probe;
    memset(&res, 0, sizeof res);
    res.hit_x = move_axis(w, pos, r, h, 0, delta.x);
    res.hit_z = move_axis(w, pos, r, h, 2, delta.z);
    if ((res.hit_x || res.hit_z) && step_h > 0.0f && delta.y <= 0.0f) {
        /* try the same horizontal move from a raised position */
        cadv_vec3 up = start;
        bool bx;
        bool bz;
        if (!move_axis(w, &up, r, h, 1, step_h)) {
            bx = move_axis(w, &up, r, h, 0, delta.x);
            bz = move_axis(w, &up, r, h, 2, delta.z);
            if ((!bx || !res.hit_x) && (!bz || !res.hit_z) &&
                (cadv_absf(up.x - start.x) + cadv_absf(up.z - start.z)) >
                    (cadv_absf(pos->x - start.x) + cadv_absf(pos->z - start.z)) + 1e-4f) {
                move_axis(w, &up, r, h, 1, -step_h); /* settle back down onto the step */
                *pos = up;
                res.hit_x = bx;
                res.hit_z = bz;
                res.stepped = true;
            }
        }
    }
    if (move_axis(w, pos, r, h, 1, delta.y)) {
        if (delta.y < 0.0f) {
            res.on_ground = true;
        } else {
            res.hit_ceiling = true;
        }
    }
    if (!res.on_ground) {
        probe = body(cadv_v3(pos->x, pos->y - 0.01f, pos->z), r, h);
        res.on_ground = cadv_world_overlaps(w, probe);
    }
    return res;
}

cadv_move_result cadv_world_move(const cadv_world *w, cadv_vec3 *pos, float r, float h,
                                 cadv_vec3 delta, float step_h)
{
    /* sub-step so fast bodies cannot tunnel through thin walls */
    float m = cadv_maxf(cadv_absf(delta.x), cadv_maxf(cadv_absf(delta.y), cadv_absf(delta.z)));
    float lim = cadv_maxf(0.05f, r * 0.9f);
    int n = (int)(m / lim) + 1;
    int i;
    cadv_move_result total;
    cadv_vec3 step;
    memset(&total, 0, sizeof total);
    if (n > 64) {
        n = 64;
    }
    step = cadv_v3_scale(delta, 1.0f / (float)n);
    for (i = 0; i < n; ++i) {
        cadv_move_result r1 = move_once(w, pos, r, h, step, step_h);
        total.hit_x |= r1.hit_x;
        total.hit_z |= r1.hit_z;
        total.hit_ceiling |= r1.hit_ceiling;
        total.stepped |= r1.stepped;
        total.on_ground = r1.on_ground;
        if (r1.hit_x) {
            step.x = 0.0f;
        }
        if (r1.hit_z) {
            step.z = 0.0f;
        }
        if (r1.hit_ceiling || (r1.on_ground && step.y < 0.0f)) {
            step.y = 0.0f;
        }
    }
    return total;
}
