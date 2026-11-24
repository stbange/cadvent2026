/* SPDX-License-Identifier: MIT */
#include "kits/fps/ctf.h"

#include <string.h>

void cadv_flag_init(cadv_flag *f, int team, cadv_vec3 home)
{
    memset(f, 0, sizeof *f);
    f->team = team;
    f->home = home;
    f->pos = home;
    f->carrier = -1;
}

void cadv_flag_return(cadv_flag *f)
{
    f->state = CADV_FLAG_HOME;
    f->pos = f->home;
    f->vel = cadv_v3(0, 0, 0);
    f->carrier = -1;
    f->return_t = 0.0f;
}

cadv_flag_event cadv_flag_touch(cadv_flag *f, int agent, int team)
{
    if (f->state == CADV_FLAG_CARRIED) {
        return CADV_FLAG_NONE;
    }
    if (team != f->team) {
        f->state = CADV_FLAG_CARRIED;
        f->carrier = agent;
        f->return_t = 0.0f;
        return CADV_FLAG_TAKEN;
    }
    if (f->state == CADV_FLAG_DROPPED) {
        cadv_flag_return(f);
        return CADV_FLAG_RETURNED;
    }
    return CADV_FLAG_NONE;
}

void cadv_flag_drop(cadv_flag *f, cadv_vec3 pos, cadv_vec3 vel)
{
    f->state = CADV_FLAG_DROPPED;
    f->carrier = -1;
    f->pos = pos;
    f->vel = cadv_v3_scale(vel, 0.5f);
    f->return_t = CADV_FLAG_RETURN;
}

bool cadv_flag_tick(cadv_flag *f, float dt)
{
    if (f->state != CADV_FLAG_DROPPED) {
        return false;
    }
    f->return_t -= dt;
    if (f->return_t <= 0.0f) {
        cadv_flag_return(f);
        return true;
    }
    return false;
}

cadv_flag_event cadv_flag_try_capture(cadv_flag *enemy, const cadv_flag *own, cadv_vec3 carrier_pos)
{
    float dx = carrier_pos.x - own->home.x;
    float dz = carrier_pos.z - own->home.z;
    float dy = carrier_pos.y - own->home.y;
    if (enemy->state != CADV_FLAG_CARRIED || own->state != CADV_FLAG_HOME) {
        return CADV_FLAG_NONE;
    }
    if (dx * dx + dz * dz > CADV_FLAG_TOUCH * CADV_FLAG_TOUCH || dy < -1.5f || dy > 3.5f) {
        return CADV_FLAG_NONE;
    }
    cadv_flag_return(enemy);
    return CADV_FLAG_CAPTURED;
}

void cadv_generator_init(cadv_generator *g, float hp, float repair_time)
{
    g->hp = hp;
    g->max_hp = hp;
    g->repair_time = repair_time;
    g->down_t = 0.0f;
}

bool cadv_generator_powered(const cadv_generator *g) { return g->hp > 0.0f; }

bool cadv_generator_damage(cadv_generator *g, float amount)
{
    if (g->hp <= 0.0f || amount <= 0.0f) {
        return false;
    }
    g->hp -= amount;
    if (g->hp <= 0.0f) {
        g->hp = 0.0f;
        g->down_t = g->repair_time;
        return true;
    }
    return false;
}

bool cadv_generator_tick(cadv_generator *g, float dt)
{
    if (g->hp > 0.0f) {
        return false;
    }
    g->down_t -= dt;
    if (g->down_t <= 0.0f) {
        g->down_t = 0.0f;
        g->hp = g->max_hp;
        return true;
    }
    return false;
}

bool cadv_turret_ready(cadv_turret *t, const cadv_generator *g, float dist, float dt)
{
    t->reload = cadv_maxf(0.0f, t->reload - dt);
    if (!cadv_generator_powered(g) || dist > t->range || t->reload > 0.0f) {
        return false;
    }
    t->reload = t->cooldown;
    return true;
}
