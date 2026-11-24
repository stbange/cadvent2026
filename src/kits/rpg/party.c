/* SPDX-License-Identifier: MIT */
#include "kits/rpg/party.h"

#include <string.h>

const char *cadv_order_name(cadv_order o)
{
    static const char *const names[CADV_ORDERS] = {"FOLLOW", "HOLD", "ATTACK", "COVER"};
    return (int)o >= 0 && o < CADV_ORDERS ? names[o] : "?";
}

cadv_vec3 cadv_formation_slot(cadv_vec3 leader, float yaw, int slot, int count, float spacing)
{
    /* rows of two behind the leader, alternating left and right; an odd last one in the middle */
    float fx = -cadv_sinf(yaw);
    float fz = -cadv_cosf(yaw);
    float rx = -fz;
    float rz = fx;
    int row = slot / 2 + 1;
    float side = (slot == count - 1 && count % 2 == 1) ? 0.0f : (slot % 2 == 0 ? -1.0f : 1.0f);
    float back = (float)row * spacing;
    float across = side * spacing * 0.8f * (float)row;
    return cadv_v3(leader.x - fx * back + rx * across, leader.y, leader.z - fz * back + rz * across);
}

void cadv_overclock_init(cadv_overclock *o, float max, float drain, float regen, float crash_len)
{
    memset(o, 0, sizeof *o);
    o->max = max;
    o->stamina = max;
    o->drain = drain;
    o->regen = regen;
    o->crash_len = crash_len;
}

bool cadv_overclock_set(cadv_overclock *o, cadv_slider which, bool on)
{
    if ((int)which < 0 || which >= CADV_OC_COUNT || (on && o->crash_t > 0.0f)) {
        return false;
    }
    o->on[which] = on;
    return true;
}

int cadv_overclock_active(const cadv_overclock *o)
{
    int n = 0;
    int i;
    for (i = 0; i < CADV_OC_COUNT; ++i) {
        n += o->on[i] ? 1 : 0;
    }
    return n;
}

bool cadv_overclock_tick(cadv_overclock *o, float dt)
{
    int n = cadv_overclock_active(o);
    if (o->crash_t > 0.0f) {
        o->crash_t = cadv_maxf(0.0f, o->crash_t - dt);
        o->stamina = cadv_minf(o->max, o->stamina + o->regen * 0.5f * dt);
        return false;
    }
    if (n == 0) {
        o->stamina = cadv_minf(o->max, o->stamina + o->regen * dt);
        return false;
    }
    o->stamina -= o->drain * (float)n * dt;
    if (o->stamina <= 0.0f) {
        int i;
        o->stamina = 0.0f;
        o->crash_t = o->crash_len;
        for (i = 0; i < CADV_OC_COUNT; ++i) {
            o->on[i] = false;
        }
        return true;
    }
    return false;
}

bool cadv_overclock_crashed(const cadv_overclock *o) { return o->crash_t > 0.0f; }

float cadv_overclock_level(const cadv_overclock *o, cadv_slider which)
{
    if (o->crash_t > 0.0f) {
        return -0.5f;
    }
    return (int)which >= 0 && which < CADV_OC_COUNT && o->on[which] ? 1.0f : 0.0f;
}

void cadv_ring_open(cadv_ring *r, int count)
{
    r->open = true;
    r->count = count;
    r->sel = -1;
    r->x = 0.0f;
    r->y = 0.0f;
}

void cadv_ring_look(cadv_ring *r, float dx, float dy)
{
    float len;
    if (!r->open) {
        return;
    }
    r->x = cadv_clampf(r->x + dx, -1.0f, 1.0f);
    r->y = cadv_clampf(r->y + dy, -1.0f, 1.0f);
    len = cadv_sqrtf(r->x * r->x + r->y * r->y);
    if (len > 0.15f) {
        /* angle from straight up (look_y > 0), clockwise towards look_x > 0 */
        float a = cadv_atan2f(r->x, r->y);
        float step = 2.0f * CADV_PI / (float)r->count;
        int s = (int)((a + CADV_PI * 2.0f + step * 0.5f) / step) % r->count;
        r->sel = s;
    }
}

int cadv_ring_close(cadv_ring *r)
{
    int s = r->open ? r->sel : -1;
    r->open = false;
    return s;
}
