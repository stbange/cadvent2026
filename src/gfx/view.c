/* SPDX-License-Identifier: MIT */
#include "gfx/view.h"

static cadv_rect rect(int x, int y, int w, int h)
{
    cadv_rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

cadv_rect cadv_view_split(int n, int index, int w, int h)
{
    int hw = w / 2;
    int hh = h / 2;
    if (n <= 1 || index < 0 || index >= n) {
        return rect(0, 0, w, h);
    }
    if (n == 2) {
        return index == 0 ? rect(0, 0, hw, h) : rect(hw, 0, w - hw, h);
    }
    if (n == 3 && index == 2) {
        return rect(0, 0, w, hh);
    }
    /* 3 (top two) and 4: quadrants, index 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right */
    return rect((index & 1) ? hw : 0, (index & 2) ? 0 : hh, (index & 1) ? w - hw : hw,
                (index & 2) ? hh : h - hh);
}

void cadv_view_internal_size(int view_w, int view_h, int internal_h, int *out_w, int *out_h)
{
    int w;
    if (view_h <= 0 || view_w <= 0) {
        *out_w = 2;
        *out_h = internal_h > 0 ? internal_h : 2;
        return;
    }
    if (internal_h > view_h) {
        internal_h = view_h;
    }
    if (internal_h < 2) {
        internal_h = 2;
    }
    w = (int)((long)internal_h * view_w / view_h);
    w &= ~1;
    *out_w = w < 2 ? 2 : w;
    *out_h = internal_h;
}

cadv_mat4 cadv_camera_view(const cadv_camera *c)
{
    cadv_vec3 fwd = cadv_v3_from_yaw_pitch(c->yaw, c->pitch);
    return cadv_m4_look_at(c->pos, cadv_v3_add(c->pos, fwd), cadv_v3(0, 1, 0));
}

cadv_mat4 cadv_camera_proj(const cadv_camera *c, float aspect)
{
    return cadv_m4_perspective(c->fovy, aspect, c->znear, c->zfar);
}

int cadv_sprite_frame(cadv_vec3 sprite_pos, float sprite_yaw, cadv_vec3 viewer_pos, int dirs)
{
    /* yaw convention: 0 faces -Z; the angle of the viewer around the sprite */
    float dx = viewer_pos.x - sprite_pos.x;
    float dz = viewer_pos.z - sprite_pos.z;
    float to_viewer = cadv_atan2f(-dx, -dz); /* yaw that would face the viewer */
    float rel = cadv_wrap_angle(to_viewer - sprite_yaw);
    float sector = CADV_TAU / (float)dirs;
    int f;
    if (dirs <= 1) {
        return 0;
    }
    if (rel < 0.0f) {
        rel += CADV_TAU;
    }
    f = (int)((rel + 0.5f * sector) / sector);
    return f % dirs;
}

int cadv_select_shadow_lights(const cadv_light_ref *lights, int n, cadv_vec3 eye, int *out,
                              int max_out)
{
    float score[64];
    int count = 0;
    int i;
    int j;
    if (n > 64) {
        n = 64;
    }
    for (i = 0; i < n && max_out > 0; ++i) {
        float s;
        if (!lights[i].casts_shadow || lights[i].intensity <= 0.0f) {
            continue;
        }
        s = cadv_v3_dist(lights[i].pos, eye) / (lights[i].radius * lights[i].intensity + 1e-3f);
        /* insertion into the sorted top list */
        for (j = count; j > 0 && score[j - 1] > s; --j) {
            if (j < max_out) {
                score[j] = score[j - 1];
                out[j] = out[j - 1];
            }
        }
        if (j < max_out) {
            score[j] = s;
            out[j] = i;
            if (count < max_out) {
                ++count;
            }
        }
    }
    return count;
}
