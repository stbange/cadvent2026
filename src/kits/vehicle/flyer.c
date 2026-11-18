/* SPDX-License-Identifier: MIT */
#include "kits/vehicle/flyer.h"

#include <string.h>

void cadv_flyer_spawn(cadv_flyer *f, cadv_vec3 pos, float yaw)
{
    memset(f, 0, sizeof *f);
    f->pos = pos;
    f->yaw = yaw;
}

cadv_vec3 cadv_flyer_forward(const cadv_flyer *f) { return cadv_v3_from_yaw_pitch(f->yaw, f->pitch); }

float cadv_flyer_altitude(const cadv_flyer *f, const cadv_terrain *t)
{
    return f->pos.y - cadv_terrain_surface(t, f->pos.x, f->pos.z);
}

void cadv_flyer_tick(cadv_flyer *f, const cadv_flyer_def *d, const cadv_input_frame *in,
                     const cadv_terrain *t, float dt)
{
    cadv_vec3 fwd;
    cadv_vec3 right;
    cadv_vec3 wish;
    cadv_vec3 next;
    float speed;
    float max = d->max_speed;
    float lim = cadv_terrain_size(t);
    float floor_y;
    float vy = 0.0f;
    f->yaw = cadv_wrap_angle(f->yaw + in->look_x);
    f->pitch = cadv_clampf(f->pitch + in->look_y, -1.2f, 1.2f);
    f->bank = cadv_clampf(f->bank * 0.9f + (in->move_x * 0.5f + in->look_x * 4.0f) * 0.1f, -1.0f, 1.0f);
    if (cadv_input_down(in, CADV_ACT_RUN)) {
        max *= d->boost;
    }
    /* thrust in the horizontal plane; climbing is explicit */
    fwd = cadv_v3(-cadv_sinf(f->yaw), 0.0f, -cadv_cosf(f->yaw));
    right = cadv_v3(-fwd.z, 0.0f, fwd.x);
    wish = cadv_v3_add(cadv_v3_scale(fwd, cadv_clampf(in->move_y, -1.0f, 1.0f)),
                       cadv_v3_scale(right, cadv_clampf(in->move_x, -1.0f, 1.0f)));
    if (cadv_v3_len(wish) > 1.0f) {
        wish = cadv_v3_norm(wish);
    }
    f->vel = cadv_v3_add(f->vel, cadv_v3_scale(wish, d->accel * dt));
    f->vel = cadv_v3_scale(f->vel, 1.0f - cadv_clampf(d->drag * dt, 0.0f, 1.0f));
    f->vel.y = 0.0f;
    speed = cadv_v3_len(f->vel);
    if (speed > max) {
        f->vel = cadv_v3_scale(f->vel, max / speed);
    }
    if (cadv_input_down(in, CADV_ACT_JUMP)) {
        vy += d->climb;
    }
    if (cadv_input_down(in, CADV_ACT_CROUCH)) {
        vy -= d->climb;
    }
    /* diving along the look direction while thrusting */
    vy += in->move_y > 0.0f ? cadv_sinf(f->pitch) * in->move_y * speed * 0.6f : 0.0f;
    next = cadv_v3_add(f->pos, cadv_v3_scale(f->vel, dt));
    next.y += vy * dt;
    next.x = cadv_clampf(next.x, 1.0f, lim - 1.0f);
    next.z = cadv_clampf(next.z, 1.0f, lim - 1.0f);
    floor_y = cadv_terrain_surface(t, next.x, next.z) + d->clearance;
    f->bumped = false;
    if (next.y < floor_y) {
        /* a face too steep to glide up: stop instead of popping over it */
        float hstep = cadv_sqrtf((next.x - f->pos.x) * (next.x - f->pos.x) + (next.z - f->pos.z) * (next.z - f->pos.z));
        if (floor_y - next.y > 2.5f * hstep + 0.02f) {
            next.x = f->pos.x;
            next.z = f->pos.z;
            f->vel = cadv_v3_scale(f->vel, -0.3f);
            f->bumped = true;
            floor_y = cadv_terrain_surface(t, next.x, next.z) + d->clearance;
        }
        next.y = cadv_maxf(next.y, floor_y);
    }
    next.y = cadv_minf(next.y, d->ceiling);
    f->pos = next;
}
