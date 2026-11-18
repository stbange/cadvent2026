/* SPDX-License-Identifier: MIT */
#include "hub/layout.h"

#include <stdbool.h>

static const float k_slot_z[6] = {-4.0f, -9.0f, -14.0f, -19.0f, -24.0f, -29.0f};

cadv_portal_slot cadv_hall_portal(int day)
{
    cadv_portal_slot s;
    int i;
    if (day < 1) {
        day = 1;
    }
    if (day >= 24) {
        s.base = cadv_v3(0.0f, 0.0f, CADV_APSE_NORTH);
        s.yaw = CADV_PI; /* faces +Z (south) */
        s.normal = cadv_v3(0, 0, 1);
        return s;
    }
    i = (day - 1) % 6;
    if (day <= 6 || (day >= 13 && day <= 18)) {
        s.base = cadv_v3(-CADV_HALL_W, day <= 6 ? 0.0f : CADV_BALCONY_Y, k_slot_z[i]);
        s.yaw = -0.5f * CADV_PI; /* faces +X */
        s.normal = cadv_v3(1, 0, 0);
    } else {
        int j = day <= 12 ? (day - 7) : (day - 19);
        s.base = cadv_v3(CADV_HALL_W, day <= 12 ? 0.0f : CADV_BALCONY_Y, k_slot_z[j]);
        s.yaw = 0.5f * CADV_PI; /* faces -X */
        s.normal = cadv_v3(-1, 0, 0);
    }
    return s;
}

cadv_vec3 cadv_hall_portal_front(int day, float distance)
{
    cadv_portal_slot s = cadv_hall_portal(day);
    return cadv_v3_add(s.base, cadv_v3_scale(s.normal, distance));
}

cadv_vec3 cadv_hall_socket(int day)
{
    /* 23 sockets on a half circle of radius 3.4 m around portal 24 */
    float t = (float)(day - 1) / 22.0f;
    float a = CADV_PI * (0.08f + 0.84f * t);
    return cadv_v3(-3.4f * cadv_cosf(a), 2.2f + 3.4f * cadv_sinf(a), CADV_APSE_NORTH + 0.05f);
}

cadv_vec3 cadv_hall_tree(void) { return cadv_v3(0.0f, 0.0f, -16.0f); }
cadv_vec3 cadv_hall_spawn(void) { return cadv_v3(0.0f, 0.0f, 5.5f); }
float cadv_hall_spawn_yaw(void) { return 0.0f; }

cadv_vec3 cadv_hall_legend_plaque(int index)
{
    return cadv_v3(5.0f + 1.6f * (float)index, 2.2f, CADV_HALL_SOUTH - 0.05f);
}

cadv_vec3 cadv_hall_arch_use_point(void) { return cadv_v3(0.0f, 0.0f, CADV_APSE_NORTH + 3.0f); }

void cadv_hall_build_world(cadv_world *w)
{
    float W = CADV_HALL_W;
    float H = CADV_HALL_HEIGHT;
    float by = CADV_BALCONY_Y;
    float bd = CADV_BALCONY_DEPTH;
    int i;
    cadv_world_clear(w);
    /* floor, ceiling, outer walls */
    cadv_world_add(w, cadv_v3(-W - 1, -1, CADV_APSE_NORTH - 1), cadv_v3(W + 1, 0, CADV_HALL_SOUTH + 1), 0);
    cadv_world_add(w, cadv_v3(-W - 1, H, CADV_APSE_NORTH - 1), cadv_v3(W + 1, H + 1, CADV_HALL_SOUTH + 1), 0);
    cadv_world_add(w, cadv_v3(-W - 1, 0, CADV_HALL_SOUTH), cadv_v3(W + 1, H, CADV_HALL_SOUTH + 1), 0);
    cadv_world_add(w, cadv_v3(-W - 1, 0, CADV_APSE_NORTH), cadv_v3(-W, H, CADV_HALL_SOUTH), 0);
    cadv_world_add(w, cadv_v3(W, 0, CADV_APSE_NORTH), cadv_v3(W + 1, H, CADV_HALL_SOUTH), 0);
    /* north wall with the apse opening, apse walls */
    cadv_world_add(w, cadv_v3(-W, 0, CADV_HALL_NORTH - 1), cadv_v3(-CADV_APSE_HALF, H, CADV_HALL_NORTH), 0);
    cadv_world_add(w, cadv_v3(CADV_APSE_HALF, 0, CADV_HALL_NORTH - 1), cadv_v3(W, H, CADV_HALL_NORTH), 0);
    cadv_world_add(w, cadv_v3(-CADV_APSE_HALF - 1, 0, CADV_APSE_NORTH - 1), cadv_v3(CADV_APSE_HALF + 1, H, CADV_APSE_NORTH), 0);
    cadv_world_add(w, cadv_v3(-CADV_APSE_HALF - 1, 0, CADV_APSE_NORTH), cadv_v3(-CADV_APSE_HALF, H, CADV_HALL_NORTH), 0);
    cadv_world_add(w, cadv_v3(CADV_APSE_HALF, 0, CADV_APSE_NORTH), cadv_v3(CADV_APSE_HALF + 1, H, CADV_HALL_NORTH), 0);
    /* balconies: floor slab and railing, both sides */
    for (i = 0; i < 2; ++i) {
        float s = i == 0 ? -1.0f : 1.0f;
        float inner = s * (W - bd);
        float lo = s < 0 ? -W : inner;
        float hi = s < 0 ? inner : W;
        float rail_lo = s < 0 ? inner - 0.25f : inner;
        float rail_hi = s < 0 ? inner : inner + 0.25f;
        int k;
        cadv_world_add(w, cadv_v3(lo, by - 0.3f, CADV_HALL_NORTH), cadv_v3(hi, by, 0.0f), 0);
        cadv_world_add(w, cadv_v3(rail_lo, by, CADV_HALL_NORTH), cadv_v3(rail_hi, by + 1.0f, 0.0f), 0);
        /* stairs: 16 steps of 0.25 m rising northwards from z = 6 to z = 0 */
        for (k = 0; k < 16; ++k) {
            float z0 = 6.0f - (float)k * 0.375f;
            cadv_world_add(w, cadv_v3(lo, 0.0f, z0 - 0.375f), cadv_v3(hi, 0.25f * (float)(k + 1), z0), 0);
        }
        /* columns under the balcony edge */
        for (k = 0; k < 5; ++k) {
            float cz = -6.5f - 5.0f * (float)k;
            float cx = s * (W - bd - 0.6f);
            cadv_world_add(w, cadv_v3(cx - 0.45f, 0, cz - 0.45f), cadv_v3(cx + 0.45f, H, cz + 0.45f), 0);
        }
    }
    /* the tree: trunk and the lowest, widest layer of branches */
    cadv_world_add(w, cadv_v3(-2.2f, 0, -18.2f), cadv_v3(2.2f, 2.5f, -13.8f), 0);
    cadv_world_add(w, cadv_v3(-0.6f, 2.5f, -16.6f), cadv_v3(0.6f, 9.0f, -15.4f), 0);
}

int cadv_hall_path(int day, cadv_vec3 *out, int max)
{
    cadv_vec3 p[8];
    int n = 0;
    int i;
    cadv_portal_slot s = cadv_hall_portal(day);
    bool west = s.normal.x > 0.5f;
    bool balcony = s.base.y > 1.0f;
    float lane = west ? -7.0f : 7.0f; /* walking lane between columns and tree */
    if (day >= 24) {
        p[n++] = cadv_v3(4.5f, 0, 0.0f);
        p[n++] = cadv_v3(4.5f, 0, -24.0f);
        p[n++] = cadv_v3(0.0f, 0, -30.0f);
        p[n++] = cadv_hall_portal_front(24, 1.8f);
    } else if (!balcony) {
        p[n++] = cadv_v3(lane, 0, 2.0f);
        p[n++] = cadv_v3(lane, 0, s.base.z);
        p[n++] = cadv_hall_portal_front(day, 1.5f);
    } else {
        float sx = west ? -14.0f : 14.0f;
        p[n++] = cadv_v3(sx, 0, 7.0f);  /* foot of the stairs */
        p[n++] = cadv_v3(sx, CADV_BALCONY_Y, -1.0f);
        p[n++] = cadv_v3(sx, CADV_BALCONY_Y, s.base.z);
        p[n++] = cadv_hall_portal_front(day, 1.2f);
    }
    for (i = 0; i < n && i < max; ++i) {
        out[i] = p[i];
    }
    return n < max ? n : max;
}
