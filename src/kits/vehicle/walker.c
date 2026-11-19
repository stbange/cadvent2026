/* SPDX-License-Identifier: MIT */
#include "kits/vehicle/walker.h"

#include <string.h>

void cadv_walker_spawn(cadv_walker *w, const cadv_walker_def *d, cadv_vec3 pos, float yaw)
{
    int i;
    memset(w, 0, sizeof *w);
    w->pos = pos;
    w->legs_yaw = yaw;
    w->fuel = d->jump_fuel;
    w->on_ground = true;
    for (i = 0; i < CADV_WZ_COUNT; ++i) {
        w->hp[i] = d->armour[i];
    }
}

bool cadv_walker_has(const cadv_walker *w, cadv_walker_zone zone) { return w->hp[zone] > 0; }

bool cadv_walker_alive(const cadv_walker *w)
{
    return w->hp[CADV_WZ_HEAD] > 0 && w->hp[CADV_WZ_CT] > 0 && (w->hp[CADV_WZ_LL] > 0 || w->hp[CADV_WZ_RL] > 0);
}

float cadv_walker_speed_factor(const cadv_walker *w, const cadv_walker_def *d)
{
    float l = (float)w->hp[CADV_WZ_LL] / (float)(d->armour[CADV_WZ_LL] > 0 ? d->armour[CADV_WZ_LL] : 1);
    float r = (float)w->hp[CADV_WZ_RL] / (float)(d->armour[CADV_WZ_RL] > 0 ? d->armour[CADV_WZ_RL] : 1);
    if (w->hp[CADV_WZ_LL] <= 0 || w->hp[CADV_WZ_RL] <= 0) {
        return 0.35f; /* hobbling on one leg */
    }
    return 0.6f + 0.4f * cadv_minf(l, r);
}

float cadv_walker_torso_yaw(const cadv_walker *w) { return cadv_wrap_angle(w->legs_yaw + w->twist); }

cadv_vec3 cadv_walker_eye(const cadv_walker *w, const cadv_walker_def *d) { return cadv_v3(w->pos.x, w->pos.y + d->height, w->pos.z); }

cadv_vec3 cadv_walker_aim(const cadv_walker *w) { return cadv_v3_from_yaw_pitch(cadv_walker_torso_yaw(w), w->pitch); }

void cadv_walker_add_heat(cadv_walker *w, const cadv_walker_def *d, float heat)
{
    w->heat = cadv_clampf(w->heat + heat, 0.0f, d->heat_cap * 1.5f);
    if (!w->shutdown && w->heat >= d->heat_cap) {
        w->shutdown = true;
        w->shutdown_time = 0.0f;
        ++w->shutdowns;
    }
}

static void lose(cadv_walker *w, cadv_walker_zone z)
{
    if (w->hp[z] <= 0) {
        w->hp[z] = 0;
    }
}

bool cadv_walker_damage(cadv_walker *w, cadv_walker_zone zone, int amount)
{
    bool was = cadv_walker_alive(w);
    int left;
    if (amount <= 0 || !was) {
        return false;
    }
    if (w->hp[zone] <= 0) {
        /* already gone: the blow lands on what it was attached to */
        cadv_walker_zone inner = zone == CADV_WZ_LA || zone == CADV_WZ_LL ? CADV_WZ_LT
                                 : zone == CADV_WZ_RA || zone == CADV_WZ_RL ? CADV_WZ_RT
                                                                            : CADV_WZ_CT;
        return cadv_walker_damage(w, inner, amount);
    }
    left = amount - w->hp[zone];
    w->hp[zone] -= amount;
    if (w->hp[zone] <= 0) {
        lose(w, zone);
        ++w->lost;
        if (zone == CADV_WZ_LT) {
            /* the arm on a lost side torso falls with it */
            if (w->hp[CADV_WZ_LA] > 0) {
                w->hp[CADV_WZ_LA] = 0;
                ++w->lost;
            }
        } else if (zone == CADV_WZ_RT && w->hp[CADV_WZ_RA] > 0) {
            w->hp[CADV_WZ_RA] = 0;
            ++w->lost;
        }
        if (left > 0 && zone != CADV_WZ_CT && zone != CADV_WZ_HEAD) {
            cadv_walker_zone inner = zone == CADV_WZ_LA || zone == CADV_WZ_LL ? CADV_WZ_LT
                                     : zone == CADV_WZ_RA || zone == CADV_WZ_RL ? CADV_WZ_RT
                                                                                : CADV_WZ_CT;
            cadv_walker_damage(w, inner, left);
        }
    }
    return !cadv_walker_alive(w);
}

cadv_walker_zone cadv_walker_hit_zone(const cadv_walker *w, const cadv_walker_def *d, cadv_vec3 p)
{
    float ty = cadv_walker_torso_yaw(w);
    /* the torso's right-hand vector (forward is (-sin, -cos)) */
    float rx = cadv_cosf(ty);
    float rz = -cadv_sinf(ty);
    float lat = ((p.x - w->pos.x) * rx + (p.z - w->pos.z) * rz) / 2.5f; /* -1..1 across the body, + = its right */
    float h = (p.y - w->pos.y) / (d->height > 0.1f ? d->height : 1.0f);
    if (h < 0.36f) {
        return lat >= 0.0f ? CADV_WZ_RL : CADV_WZ_LL;
    }
    if (h > 0.92f && cadv_absf(lat) < 0.3f) {
        return CADV_WZ_HEAD;
    }
    if (lat > 0.6f) {
        return CADV_WZ_RA;
    }
    if (lat < -0.6f) {
        return CADV_WZ_LA;
    }
    if (lat > 0.22f) {
        return CADV_WZ_RT;
    }
    if (lat < -0.22f) {
        return CADV_WZ_LT;
    }
    return CADV_WZ_CT;
}

const char *cadv_walker_zone_name(cadv_walker_zone z)
{
    static const char *const names[CADV_WZ_COUNT] = {"HEAD", "CENTRE TORSO", "LEFT TORSO", "RIGHT TORSO",
                                                     "LEFT ARM", "RIGHT ARM", "LEFT LEG", "RIGHT LEG"};
    return (int)z >= 0 && z < CADV_WZ_COUNT ? names[z] : "?";
}

void cadv_walker_tick(cadv_walker *w, const cadv_walker_def *d, const cadv_walker_ctl *c, const cadv_terrain *t,
                      float cooling, float dt)
{
    float lim = cadv_terrain_size(t);
    float target;
    float ground;
    float nx;
    float nz;
    w->blocked = false;
    w->jetting = false;
    /* heat: sinks always work, even while shut down */
    w->heat = cadv_maxf(0.0f, w->heat - d->heat_shed * cooling * dt);
    if (w->shutdown) {
        w->shutdown_time += dt;
        if (w->heat < d->restart_heat && w->shutdown_time >= 2.0f) {
            w->shutdown = false;
        }
    }
    if (!cadv_walker_alive(w)) {
        w->speed = 0.0f;
    } else if (!w->shutdown) {
        float twist_step = cadv_clampf(c->twist, -d->twist_rate * dt, d->twist_rate * dt);
        w->legs_yaw = cadv_wrap_angle(w->legs_yaw - cadv_clampf(c->turn, -1.0f, 1.0f) * d->turn_rate * dt);
        if (c->centre) {
            w->twist -= cadv_clampf(w->twist, -d->twist_rate * dt, d->twist_rate * dt);
        } else {
            w->twist = cadv_clampf(w->twist + twist_step, -d->twist_limit, d->twist_limit);
        }
        w->pitch = cadv_clampf(w->pitch + c->pitch, -0.6f, 0.5f);
        w->throttle = c->stop ? 0.0f : cadv_clampf(w->throttle + c->throttle * d->throttle_rate * dt, -1.0f, 1.0f);
        if (c->jump && w->fuel > 0.0f) {
            w->vy += d->jump_thrust * dt;
            w->fuel = cadv_maxf(0.0f, w->fuel - dt);
            w->jetting = true;
            cadv_walker_add_heat(w, d, d->jump_heat * dt);
        }
    }
    if (!w->jetting && w->on_ground) {
        w->fuel = cadv_minf(d->jump_fuel, w->fuel + dt * 0.5f);
    }
    /* walking speed eases towards the throttle */
    target = (w->shutdown || !cadv_walker_alive(w)) ? 0.0f
             : w->throttle >= 0.0f ? w->throttle * d->max_speed : w->throttle * d->max_speed * d->reverse;
    target *= cadv_walker_speed_factor(w, d);
    if (w->speed < target) {
        w->speed = cadv_minf(target, w->speed + d->accel * dt);
    } else {
        w->speed = cadv_maxf(target, w->speed - d->accel * dt);
    }
    nx = cadv_clampf(w->pos.x - cadv_sinf(w->legs_yaw) * w->speed * dt, 2.0f, lim - 2.0f);
    nz = cadv_clampf(w->pos.z - cadv_cosf(w->legs_yaw) * w->speed * dt, 2.0f, lim - 2.0f);
    ground = cadv_terrain_height(t, nx, nz);
    if (!w->on_ground && ground > w->pos.y) {
        /* airborne into a face: slide down it instead of climbing it */
        nx = w->pos.x;
        nz = w->pos.z;
        ground = cadv_terrain_height(t, nx, nz);
        w->blocked = true;
    } else if (w->on_ground) {
        float step = cadv_sqrtf((nx - w->pos.x) * (nx - w->pos.x) + (nz - w->pos.z) * (nz - w->pos.z));
        if (ground - w->pos.y > d->max_climb * step + 0.01f) {
            /* too steep to walk up: stop at the foot of the slope */
            nx = w->pos.x;
            nz = w->pos.z;
            ground = cadv_terrain_height(t, nx, nz);
            w->speed *= 0.3f;
            w->blocked = true;
        }
    }
    w->pos.x = nx;
    w->pos.z = nz;
    w->stride += cadv_absf(w->speed) * dt * 0.6f;
    /* vertical: stick to the ground unless the jets lift us */
    w->vy -= d->gravity * dt;
    w->pos.y += w->vy * dt;
    if (w->on_ground && !w->jetting && w->pos.y > ground && w->pos.y - ground < 1.5f) {
        w->pos.y = ground; /* walking down a slope */
    }
    if (w->pos.y <= ground) {
        w->pos.y = ground;
        w->vy = w->jetting ? cadv_maxf(w->vy, 0.0f) : 0.0f; /* jets keep pushing off the ground */
        w->on_ground = true;
    } else {
        w->on_ground = false;
    }
}
