/* SPDX-License-Identifier: MIT */
#include "kits/fps/player.h"

#include <string.h>

cadv_player_tuning cadv_player_default_tuning(void)
{
    cadv_player_tuning t;
    t.walk_speed = 4.5f;
    t.run_speed = 7.0f;
    t.crouch_speed = 2.2f;
    t.accel = 12.0f;
    t.air_accel = 2.5f;
    t.friction = 10.0f;
    t.jump_speed = 5.2f;
    t.gravity = 16.0f;
    t.max_pitch = 1.45f;
    return t;
}

void cadv_player_spawn(cadv_player *p, cadv_vec3 feet, float yaw)
{
    memset(p, 0, sizeof *p);
    p->pos = feet;
    p->yaw = yaw;
    p->health = 100;
    p->tune = cadv_player_default_tuning();
}

float cadv_player_height(const cadv_player *p)
{
    return p->crouching ? CADV_PLAYER_CROUCH_HEIGHT : CADV_PLAYER_HEIGHT;
}

cadv_vec3 cadv_player_eye(const cadv_player *p)
{
    float eye = p->crouching ? CADV_PLAYER_CROUCH_HEIGHT - 0.15f : CADV_PLAYER_EYE;
    return cadv_v3(p->pos.x, p->pos.y + eye, p->pos.z);
}

cadv_vec3 cadv_player_forward(const cadv_player *p) { return cadv_v3_from_yaw_pitch(p->yaw, p->pitch); }

uint32_t cadv_player_tick(cadv_player *p, const cadv_world *w, const cadv_input_frame *in, float dt)
{
    uint32_t ev = 0;
    cadv_vec3 fwd;
    cadv_vec3 right;
    cadv_vec3 wish;
    float speed;
    float wl;
    float accel;
    cadv_move_result mr;
    bool was_ground = p->on_ground;
    bool want_crouch = cadv_input_down(in, CADV_ACT_CROUCH);

    p->yaw = cadv_wrap_angle(p->yaw + in->look_x);
    p->pitch = cadv_clampf(p->pitch + in->look_y, -p->tune.max_pitch, p->tune.max_pitch);
    /* stand up only if there is head room */
    if (!want_crouch && p->crouching) {
        cadv_aabb head = cadv_aabb_make(
            cadv_v3(p->pos.x - CADV_PLAYER_RADIUS, p->pos.y, p->pos.z - CADV_PLAYER_RADIUS),
            cadv_v3(p->pos.x + CADV_PLAYER_RADIUS, p->pos.y + CADV_PLAYER_HEIGHT, p->pos.z + CADV_PLAYER_RADIUS));
        if (!cadv_world_overlaps(w, head)) {
            p->crouching = false;
        }
    } else if (want_crouch) {
        p->crouching = true;
    }
    fwd = cadv_v3(-cadv_sinf(p->yaw), 0.0f, -cadv_cosf(p->yaw));
    right = cadv_v3(-fwd.z, 0.0f, fwd.x);
    wish = cadv_v3_add(cadv_v3_scale(fwd, in->move_y), cadv_v3_scale(right, in->move_x));
    wl = cadv_v3_len(wish);
    if (wl > 1.0f) {
        wish = cadv_v3_scale(wish, 1.0f / wl);
    }
    speed = p->crouching ? p->tune.crouch_speed
                         : (cadv_input_down(in, CADV_ACT_RUN) ? p->tune.run_speed : p->tune.walk_speed);
    accel = p->on_ground ? p->tune.accel : p->tune.air_accel;
    {
        /* move horizontal velocity towards the wished velocity */
        float k = cadv_clampf(accel * dt, 0.0f, 1.0f);
        cadv_vec3 target = cadv_v3_scale(wish, speed);
        if (p->on_ground && wl < 0.01f) {
            k = cadv_clampf(p->tune.friction * dt, 0.0f, 1.0f);
        }
        p->vel.x += (target.x - p->vel.x) * k;
        p->vel.z += (target.z - p->vel.z) * k;
    }
    if (p->on_ground && cadv_input_pressed(in, CADV_ACT_JUMP) && !p->crouching) {
        p->vel.y = p->tune.jump_speed;
        p->on_ground = false;
        ev |= CADV_PEV_JUMP;
    }
    p->vel.y -= p->tune.gravity * dt;
    mr = cadv_world_move(w, &p->pos, CADV_PLAYER_RADIUS, cadv_player_height(p),
                         cadv_v3_scale(p->vel, dt), p->on_ground ? CADV_PLAYER_STEP : 0.0f);
    if (mr.hit_x) {
        p->vel.x = 0.0f;
    }
    if (mr.hit_z) {
        p->vel.z = 0.0f;
    }
    if (mr.hit_ceiling && p->vel.y > 0.0f) {
        p->vel.y = 0.0f;
    }
    p->on_ground = mr.on_ground;
    if (p->on_ground && p->vel.y < 0.0f) {
        if (!was_ground && p->vel.y < -3.0f) {
            ev |= CADV_PEV_LAND;
        }
        p->vel.y = 0.0f;
    }
    if (p->on_ground) {
        p->stride += cadv_sqrtf(p->vel.x * p->vel.x + p->vel.z * p->vel.z) * dt;
        if (p->stride > 1.9f) {
            p->stride = 0.0f;
            ev |= CADV_PEV_STEP;
        }
    }
    return ev;
}

bool cadv_player_damage(cadv_player *p, int amount)
{
    int absorbed;
    if (amount <= 0 || p->health <= 0) {
        return p->health <= 0;
    }
    absorbed = amount / 2 < p->armor ? amount / 2 : p->armor;
    p->armor -= absorbed;
    p->health -= amount - absorbed;
    if (p->health < 0) {
        p->health = 0;
    }
    return p->health == 0;
}
