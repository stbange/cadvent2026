/* SPDX-License-Identifier: MIT */
#include "kits/fps/tactics.h"

#include <string.h>

/* ---- recoil ------------------------------------------------------------------------ */

void cadv_recoil_offset(const cadv_recoil *r, int shot, float *dyaw, float *dpitch)
{
    int k;
    float pitch = 0.0f;
    float yaw = 0.0f;
    for (k = 1; k <= shot; ++k) {
        if (k <= r->top) {
            /* the climb eases off as it tops out */
            pitch += r->climb * (1.0f - 0.5f * (float)(k - 1) / (float)(r->top > 1 ? r->top : 1));
            yaw += r->sway * 0.08f * (float)((k % 3) - 1);
        } else {
            /* then it drifts left and right in a slow wave */
            float ph = (float)(k - r->top);
            pitch += r->climb * 0.08f * cadv_sinf(ph * 1.3f);
            yaw += r->sway * cadv_sinf(ph * 0.45f) * 0.35f;
        }
    }
    *dyaw = yaw;
    *dpitch = pitch;
}

float cadv_spread(float base, float move_penalty, float speed, float max_speed, bool airborne, bool crouched)
{
    float f = max_speed > 0.0f ? cadv_clampf(speed / max_speed, 0.0f, 1.0f) : 0.0f;
    float s = base + move_penalty * f;
    if (airborne) {
        s += move_penalty * 2.0f;
    }
    if (crouched) {
        s *= 0.5f;
    }
    return s;
}

/* ---- economy -------------------------------------------------------------------------- */

cadv_econ_rules cadv_econ_default(void)
{
    cadv_econ_rules r;
    r.start = 800;
    r.max = 16000;
    r.kill = 300;
    r.knife_kill = 1500;
    r.win_elimination = 3250;
    r.win_bomb = 3500;
    r.win_defuse = 3500;
    r.win_time = 3250;
    r.loss_base = 1400;
    r.loss_step = 500;
    r.loss_max = 3400;
    r.plant_bonus = 800;
    return r;
}

int cadv_econ_round_reward(const cadv_econ_rules *r, bool won, cadv_win_reason why, int loss_streak, bool attacker_planted,
                           bool is_attacker)
{
    int money;
    if (won) {
        switch (why) {
        case CADV_WIN_BOMB:
            money = r->win_bomb;
            break;
        case CADV_WIN_DEFUSE:
            money = r->win_defuse;
            break;
        case CADV_WIN_TIME:
            money = r->win_time;
            break;
        default:
            money = r->win_elimination;
            break;
        }
    } else {
        int streak = loss_streak < 1 ? 1 : loss_streak;
        money = r->loss_base + r->loss_step * (streak - 1);
        if (money > r->loss_max) {
            money = r->loss_max;
        }
        if (is_attacker && attacker_planted) {
            money += r->plant_bonus;
        }
    }
    return money;
}

int cadv_econ_add(const cadv_econ_rules *r, int money, int amount)
{
    int m = money + amount;
    return m < 0 ? 0 : (m > r->max ? r->max : m);
}

/* ---- smoke ------------------------------------------------------------------------------ */

bool cadv_segment_sphere(cadv_vec3 a, cadv_vec3 b, cadv_vec3 c, float r)
{
    cadv_vec3 ab = cadv_v3_sub(b, a);
    float len2 = cadv_v3_dot(ab, ab);
    float t = len2 > 0.0f ? cadv_clampf(cadv_v3_dot(cadv_v3_sub(c, a), ab) / len2, 0.0f, 1.0f) : 0.0f;
    cadv_vec3 p = cadv_v3_add(a, cadv_v3_scale(ab, t));
    return cadv_v3_dist(p, c) <= r;
}

/* ---- walk grid ---------------------------------------------------------------------------- */

bool cadv_nav_init(cadv_nav *n, int w, int h, float cell)
{
    if (w < 2 || h < 2 || w > CADV_NAV_MAX || h > CADV_NAV_MAX || cell <= 0.0f) {
        return false;
    }
    memset(n, 0, sizeof *n);
    n->w = w;
    n->h = h;
    n->cell = cell;
    return true;
}

static bool inside(const cadv_nav *n, int x, int z) { return x >= 0 && z >= 0 && x < n->w && z < n->h; }

void cadv_nav_set(cadv_nav *n, int x, int z, bool walkable)
{
    if (inside(n, x, z)) {
        n->walk[z * n->w + x] = walkable ? 1 : 0;
    }
}

bool cadv_nav_walkable(const cadv_nav *n, int x, int z) { return inside(n, x, z) && n->walk[z * n->w + x]; }

void cadv_nav_mark_cover(cadv_nav *n)
{
    int x;
    int z;
    for (z = 0; z < n->h; ++z) {
        for (x = 0; x < n->w; ++x) {
            bool c = false;
            if (cadv_nav_walkable(n, x, z)) {
                c = !cadv_nav_walkable(n, x + 1, z) || !cadv_nav_walkable(n, x - 1, z) || !cadv_nav_walkable(n, x, z + 1) ||
                    !cadv_nav_walkable(n, x, z - 1);
            }
            n->cover[z * n->w + x] = c ? 1 : 0;
        }
    }
}

cadv_nav_cell cadv_nav_cell_at(const cadv_nav *n, float x, float z)
{
    cadv_nav_cell c;
    int cx = (int)(x / n->cell);
    int cz = (int)(z / n->cell);
    c.x = (int8_t)(cx < 0 ? 0 : (cx >= n->w ? n->w - 1 : cx));
    c.z = (int8_t)(cz < 0 ? 0 : (cz >= n->h ? n->h - 1 : cz));
    return c;
}

cadv_vec3 cadv_nav_centre(const cadv_nav *n, cadv_nav_cell c) { return cadv_v3(((float)c.x + 0.5f) * n->cell, 0.0f, ((float)c.z + 0.5f) * n->cell); }

typedef struct nav_node {
    float g, f;
    int parent;
    uint8_t open, closed;
} nav_node;

int cadv_nav_path(const cadv_nav *n, cadv_nav_cell a, cadv_nav_cell b, cadv_nav_cell *out, int max)
{
    static const int dx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int dz[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    static nav_node nodes[CADV_NAV_MAX * CADV_NAV_MAX];
    int start = a.z * n->w + a.x;
    int goal = b.z * n->w + b.x;
    int count;
    int i;
    if (!cadv_nav_walkable(n, a.x, a.z) || !cadv_nav_walkable(n, b.x, b.z)) {
        return -1;
    }
    if (start == goal) {
        return 0;
    }
    memset(nodes, 0, sizeof(nav_node) * (size_t)(n->w * n->h));
    nodes[start].open = 1;
    nodes[start].parent = -1;
    nodes[start].f = cadv_absf((float)(a.x - b.x)) + cadv_absf((float)(a.z - b.z));
    for (;;) {
        int cur = -1;
        float best = 1e30f;
        int k;
        /* the grid is small: a linear scan for the best open node is fine */
        for (i = 0; i < n->w * n->h; ++i) {
            if (nodes[i].open && nodes[i].f < best) {
                best = nodes[i].f;
                cur = i;
            }
        }
        if (cur < 0) {
            return -1;
        }
        if (cur == goal) {
            break;
        }
        nodes[cur].open = 0;
        nodes[cur].closed = 1;
        for (k = 0; k < 8; ++k) {
            int x = cur % n->w + dx[k];
            int z = cur / n->w + dz[k];
            int ni = z * n->w + x;
            float g;
            if (!cadv_nav_walkable(n, x, z) || nodes[ni].closed) {
                continue;
            }
            if (k >= 4 && (!cadv_nav_walkable(n, cur % n->w + dx[k], cur / n->w) || !cadv_nav_walkable(n, cur % n->w, cur / n->w + dz[k]))) {
                continue; /* no corner cutting */
            }
            g = nodes[cur].g + (k >= 4 ? 1.4142f : 1.0f);
            if (!nodes[ni].open || g < nodes[ni].g) {
                float hx = cadv_absf((float)(x - b.x));
                float hz = cadv_absf((float)(z - b.z));
                nodes[ni].g = g;
                nodes[ni].f = g + cadv_maxf(hx, hz) + 0.4142f * cadv_minf(hx, hz);
                nodes[ni].parent = cur;
                nodes[ni].open = 1;
            }
        }
    }
    count = 0;
    for (i = goal; i != start; i = nodes[i].parent) {
        ++count;
    }
    if (count > max) {
        return -1;
    }
    for (i = goal; i != start; i = nodes[i].parent) {
        --count;
        out[count].x = (int8_t)(i % n->w);
        out[count].z = (int8_t)(i / n->w);
    }
    for (i = goal, count = 0; i != start; i = nodes[i].parent) {
        ++count;
    }
    return count;
}

bool cadv_nav_nearest_cover(const cadv_nav *n, cadv_nav_cell from, int radius, cadv_nav_cell *out)
{
    int best = -1;
    int r2 = radius * radius;
    int x;
    int z;
    for (z = from.z - radius; z <= from.z + radius; ++z) {
        for (x = from.x - radius; x <= from.x + radius; ++x) {
            int d = (x - from.x) * (x - from.x) + (z - from.z) * (z - from.z);
            if (inside(n, x, z) && n->cover[z * n->w + x] && d <= r2 && (best < 0 || d < best)) {
                best = d;
                out->x = (int8_t)x;
                out->z = (int8_t)z;
            }
        }
    }
    return best >= 0;
}
