/* SPDX-License-Identifier: MIT */
#include "kits/fps/story.h"

#include <string.h>

void cadv_trigger_make(cadv_trigger *t, cadv_vec3 lo, cadv_vec3 hi, bool once)
{
    memset(t, 0, sizeof *t);
    t->box = cadv_aabb_make(lo, hi);
    t->once = once;
}

bool cadv_trigger_update(cadv_trigger *t, cadv_vec3 p)
{
    bool in = cadv_aabb_contains(t->box, p);
    bool fire = in && !t->inside && (!t->once || t->fired == 0);
    t->inside = in;
    if (fire) {
        ++t->fired;
    }
    return fire;
}

void cadv_follower_init(cadv_follower *f, cadv_vec3 pos, float speed, float keep)
{
    memset(f, 0, sizeof *f);
    f->pos = pos;
    f->speed = speed;
    f->keep = keep;
}

void cadv_follower_toggle(cadv_follower *f)
{
    f->mode = f->mode == CADV_FOLLOW_ON ? CADV_FOLLOW_WAIT : CADV_FOLLOW_ON;
    f->path_len = 0;
}

void cadv_follower_goto(cadv_follower *f, cadv_vec3 goal)
{
    f->mode = CADV_FOLLOW_GOTO;
    f->goal = goal;
    f->path_len = 0;
    f->arrived = false;
}

float cadv_follower_tick(cadv_follower *f, const cadv_nav *nav, const cadv_world *w, cadv_vec3 leader, float dt)
{
    cadv_vec3 target = f->mode == CADV_FOLLOW_GOTO ? f->goal : leader;
    float stop = f->mode == CADV_FOLLOW_GOTO ? 0.3f : f->keep;
    float dx = target.x - f->pos.x;
    float dz = target.z - f->pos.z;
    float dist = cadv_sqrtf(dx * dx + dz * dz);
    cadv_vec3 step;
    if (f->mode == CADV_FOLLOW_WAIT) {
        return dist;
    }
    if (dist <= stop) {
        f->arrived = f->mode == CADV_FOLLOW_GOTO;
        f->path_len = 0;
        return dist;
    }
    if (f->path_len == 0 || f->repath <= 0) {
        int n = cadv_nav_path(nav, cadv_nav_cell_at(nav, f->pos.x, f->pos.z), cadv_nav_cell_at(nav, target.x, target.z), f->path, 64);
        f->path_len = n > 0 ? n : 0;
        f->path_pos = 0;
        f->repath = 45;
    }
    --f->repath;
    /* head for the next cell, or straight for the target on the last one */
    step = target;
    while (f->path_pos < f->path_len) {
        cadv_vec3 c = cadv_nav_centre(nav, f->path[f->path_pos]);
        if (cadv_sqrtf((c.x - f->pos.x) * (c.x - f->pos.x) + (c.z - f->pos.z) * (c.z - f->pos.z)) < 0.5f && f->path_pos + 1 < f->path_len) {
            ++f->path_pos;
        } else {
            step = f->path_pos + 1 < f->path_len ? c : target;
            break;
        }
    }
    {
        float sx = step.x - f->pos.x;
        float sz = step.z - f->pos.z;
        float sl = cadv_sqrtf(sx * sx + sz * sz);
        float v = cadv_minf(f->speed * dt, dist - stop);
        if (sl > 1e-4f) {
            cadv_world_move(w, &f->pos, 0.3f, 1.7f, cadv_v3(sx / sl * v, -0.05f, sz / sl * v), 0.45f);
        }
    }
    return dist;
}

cadv_squad_state cadv_squad_decide(cadv_squad_state current, const cadv_squad_view *v)
{
    (void)current;
    if (v->hp < 0.3f && v->sees_target) {
        return CADV_SQUAD_RETREAT;
    }
    if (!v->sees_target) {
        if (v->has_grenade && v->target_hidden > 2.0f && v->target_hidden < 10.0f && v->target_dist < 25.0f) {
            return CADV_SQUAD_GRENADE; /* flush them out of cover */
        }
        return v->target_hidden < 10.0f ? CADV_SQUAD_FLANK : CADV_SQUAD_PATROL;
    }
    if (v->allies_engaged == 1 && v->flank_open) {
        return CADV_SQUAD_FLANK;
    }
    if (v->allies_engaged >= 2) {
        return CADV_SQUAD_SUPPRESS;
    }
    return CADV_SQUAD_ENGAGE;
}

const char *cadv_squad_state_name(cadv_squad_state s)
{
    static const char *const names[] = {"PATROL", "ENGAGE", "FLANK", "SUPPRESS", "GRENADE", "RETREAT"};
    return (int)s >= 0 && s <= CADV_SQUAD_RETREAT ? names[s] : "?";
}

float cadv_charger_use(cadv_charger *c, float *value, float max, float dt)
{
    float want = cadv_maxf(0.0f, max - *value);
    float give = cadv_minf(cadv_minf(want, c->rate * dt), c->reserve);
    *value += give;
    c->reserve -= give;
    return give;
}
