/* SPDX-License-Identifier: MIT */
#include "kits/vehicle/skier.h"

#include <string.h>

static const cadv_armour_def k_armour[CADV_ARMOURS] = {
    /* name     run    mass  energy jet    drain regen hp     slots */
    {"LIGHT", 11.0f, 1.0f, 100.0f, 34.0f, 30.0f, 16.0f, 70.0f, 2},
    {"MEDIUM", 9.0f, 1.5f, 120.0f, 31.0f, 28.0f, 15.0f, 110.0f, 3},
    {"HEAVY", 7.0f, 2.2f, 150.0f, 27.0f, 26.0f, 13.0f, 180.0f, 3},
};

#define AIR_DRAG 0.004f   /* per second, fraction of speed: skiing is almost free */
#define MAX_SPEED 110.0f
#define AIR_CONTROL 9.0f  /* m/s^2 of steering in the air or on skis */

const cadv_armour_def *cadv_armour_get(cadv_armour_class c)
{
    return &k_armour[(int)c >= 0 && c < CADV_ARMOURS ? c : CADV_ARMOUR_LIGHT];
}

void cadv_skier_init(cadv_skier *k, cadv_armour_class c, cadv_vec3 pos, float yaw)
{
    memset(k, 0, sizeof *k);
    k->armour = c;
    k->pos = pos;
    k->yaw = yaw;
    k->energy = cadv_armour_get(c)->energy_max;
    k->hp = cadv_armour_get(c)->hp;
}

void cadv_skier_set_armour(cadv_skier *k, cadv_armour_class c)
{
    const cadv_armour_def *o = cadv_armour_get(k->armour);
    const cadv_armour_def *n = cadv_armour_get(c);
    k->hp = k->hp / o->hp * n->hp;
    k->energy = k->energy / o->energy_max * n->energy_max;
    k->armour = c;
}

float cadv_skier_regen(const cadv_skier *k) { return cadv_armour_get(k->armour)->regen * (k->energy_pack ? 1.5f : 1.0f); }

float cadv_skier_speed(const cadv_skier *k) { return cadv_v3_len(k->vel); }

void cadv_skier_push(cadv_skier *k, cadv_vec3 impulse)
{
    k->vel = cadv_v3_add(k->vel, cadv_v3_scale(impulse, 1.0f / cadv_armour_get(k->armour)->mass));
    if (impulse.y > 0.5f) {
        k->on_ground = false;
    }
}

cadv_vec3 cadv_skier_eye(const cadv_skier *k) { return cadv_v3(k->pos.x, k->pos.y + 1.7f, k->pos.z); }

cadv_vec3 cadv_skier_forward(const cadv_skier *k)
{
    float cp = cadv_cosf(k->pitch);
    return cadv_v3(-cadv_sinf(k->yaw) * cp, cadv_sinf(k->pitch), -cadv_cosf(k->yaw) * cp);
}

/* the wish direction on the ground plane */
static cadv_vec3 wish(const cadv_skier *k, const cadv_skier_input *in)
{
    float fx = -cadv_sinf(k->yaw);
    float fz = -cadv_cosf(k->yaw);
    cadv_vec3 d = cadv_v3(fx * in->move_y - fz * in->move_x, 0, fz * in->move_y + fx * in->move_x);
    float l = cadv_v3_len(d);
    return l > 1.0f ? cadv_v3_scale(d, 1.0f / l) : d;
}

void cadv_skier_step(cadv_skier *k, const cadv_terrain *t, const cadv_world *w, const cadv_skier_input *in, float dt)
{
    const cadv_armour_def *a = cadv_armour_get(k->armour);
    cadv_vec3 dir = wish(k, in);
    cadv_vec3 n = cadv_terrain_normal(t, k->pos.x, k->pos.z);
    float ground = cadv_terrain_height(t, k->pos.x, k->pos.z);
    bool grounded = k->on_ground;
    float sp;
    /* energy and the jets */
    k->jet_lock = cadv_maxf(0.0f, k->jet_lock - dt);
    k->jetting = in->jet && k->energy > 0.0f && k->jet_lock <= 0.0f;
    if (k->jetting) {
        k->energy -= a->jet_drain * dt;
        if (k->energy <= 0.0f) {
            k->energy = 0.0f;
            k->jet_lock = 0.6f; /* coughs out: a short wait before it relights */
        }
    } else {
        k->energy = cadv_minf(a->energy_max, k->energy + cadv_skier_regen(k) * dt);
    }
    if (k->jetting) {
        grounded = false; /* lifting off */
    } else if (grounded && in->jump) {
        k->vel.y = cadv_maxf(k->vel.y, 0.0f) + 7.0f;
        grounded = false;
    }
    k->skiing = grounded && in->ski;
    if (grounded && !k->skiing && n.y > 0.7f) {
        /* on foot: friction pulls the velocity along the slope to the wish */
        cadv_vec3 want = cadv_v3_scale(dir, a->run_speed);
        cadv_vec3 tv = cadv_v3_sub(k->vel, cadv_v3_scale(n, cadv_v3_dot(k->vel, n)));
        cadv_vec3 diff = cadv_v3_sub(cadv_v3(want.x, tv.y, want.z), tv);
        float dl = cadv_v3_len(diff);
        float acc = 60.0f * dt;
        if (dl > acc) {
            diff = cadv_v3_scale(diff, acc / dl);
        }
        k->vel = cadv_v3_add(tv, diff);
    } else {
        /* skiing, sliding down something steep, or flying: gravity, a little steering */
        cadv_vec3 hv = cadv_v3(k->vel.x, 0, k->vel.z);
        float along = cadv_v3_dot(hv, dir);
        k->vel.y -= CADV_SKIER_GRAVITY * dt;
        if (along > a->run_speed) {
            /* faster than you can run: the input only steers, it cannot push */
            float hl = cadv_v3_len(hv);
            cadv_vec3 u = cadv_v3_scale(hv, 1.0f / hl);
            dir = cadv_v3_sub(dir, cadv_v3_scale(u, cadv_v3_dot(dir, u)));
        }
        k->vel = cadv_v3_add(k->vel, cadv_v3_scale(dir, AIR_CONTROL * dt));
        k->vel = cadv_v3_scale(k->vel, 1.0f - AIR_DRAG * dt);
    }
    if (k->jetting) {
        /* mostly up, a share of the thrust goes where you steer */
        float side = cadv_v3_len(dir) > 0.1f ? 0.35f : 0.0f;
        k->vel.y += a->jet_accel * (1.0f - side * 0.5f) * dt;
        k->vel = cadv_v3_add(k->vel, cadv_v3_scale(dir, a->jet_accel * side * dt));
    }
    sp = cadv_v3_len(k->vel);
    if (sp > MAX_SPEED) {
        k->vel = cadv_v3_scale(k->vel, MAX_SPEED / sp);
    }
    /* move against the solids, then the terrain */
    {
        cadv_move_result r = cadv_world_move(w, &k->pos, CADV_SKIER_RADIUS, CADV_SKIER_HEIGHT, cadv_v3_scale(k->vel, dt), 0.3f);
        bool on_solid = r.on_ground && k->vel.y <= 0.0f;
        if (r.hit_x) {
            k->vel.x = 0.0f;
        }
        if (r.hit_z) {
            k->vel.z = 0.0f;
        }
        if (r.hit_ceiling && k->vel.y > 0.0f) {
            k->vel.y = 0.0f;
        }
        ground = cadv_terrain_height(t, k->pos.x, k->pos.z);
        n = cadv_terrain_normal(t, k->pos.x, k->pos.z);
        if (k->pos.y <= ground + 0.02f) {
            /* touch down: keep the momentum along the surface, lose what goes into it */
            float into = cadv_v3_dot(k->vel, n);
            k->pos.y = ground;
            if (into < 0.0f) {
                k->vel = cadv_v3_sub(k->vel, cadv_v3_scale(n, into));
            }
            k->on_ground = true;
        } else if (on_solid) {
            if (k->vel.y < 0.0f) {
                k->vel.y = 0.0f;
            }
            k->on_ground = true;
        } else {
            /* stay glued when skiing over a crest at modest speed */
            k->on_ground = grounded && k->pos.y < ground + 0.25f && k->vel.y <= 0.0f;
            if (k->on_ground) {
                k->pos.y = ground;
            }
        }
    }
    k->airtime = k->on_ground ? 0.0f : k->airtime + dt;
}
