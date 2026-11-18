/* SPDX-License-Identifier: MIT */
#include "kits/fps/combat.h"

#include <string.h>

static const cadv_weapon_def k_weapons[CADV_FPS_WEAPONS] = {
    /* name          kind              dmg pel spread cool  range ammo per speed splash */
    {"Candy Cane", CADV_WK_MELEE, 25, 1, 0.00f, 0.45f, 1.8f, 0, 0, 0.0f, 0.0f},
    {"Snow Pistol", CADV_WK_HITSCAN, 14, 1, 0.02f, 0.30f, 60.f, 0, 0, 0.0f, 0.0f},
    {"Scattershot", CADV_WK_HITSCAN, 8, 7, 0.09f, 0.85f, 30.f, 1, 1, 0.0f, 0.0f},
    {"Gift Launcher", CADV_WK_PROJECTILE, 60, 1, 0.00f, 1.00f, 80.f, 2, 1, 18.0f, 2.5f},
};

static const cadv_enemy_def k_enemies[CADV_ENEMY_TYPES] = {
    /* name     hp speed  r     h    sight range cool dmg proj  fly  shrink */
    {"Grunt", 40, 2.4f, 0.35f, 1.6f, 18.f, 14.f, 1.4f, 6, false, 0.0f, true},
    {"Charger", 60, 4.2f, 0.45f, 1.5f, 14.f, 1.4f, 0.8f, 12, false, 0.0f, true},
    {"Lobber", 50, 1.8f, 0.40f, 1.8f, 24.f, 20.f, 2.2f, 15, true, 0.0f, true},
};

const cadv_weapon_def *cadv_weapon_def_get(int index)
{
    return (index < 0 || index >= CADV_FPS_WEAPONS) ? NULL : &k_weapons[index];
}

const cadv_enemy_def *cadv_enemy_def_get(int index)
{
    return (index < 0 || index >= CADV_ENEMY_TYPES) ? NULL : &k_enemies[index];
}

void cadv_fps_init(cadv_fps *f, uint64_t seed)
{
    memset(f, 0, sizeof *f);
    cadv_world_clear(&f->world);
    cadv_rng_seed(&f->rng, seed, 7u);
    cadv_fps_set_tables(f, k_weapons, CADV_FPS_WEAPONS, k_enemies, CADV_ENEMY_TYPES);
    f->owned[0] = true;
    f->owned[1] = true;
    f->weapon = 1;
    f->damage_scale = 1.0f;
    f->use_solid = -1;
    f->last_killed = -1;
    f->last_broken = -1;
    cadv_player_spawn(&f->player, cadv_v3(0, 0, 0), 0.0f);
}

void cadv_fps_set_tables(cadv_fps *f, const cadv_weapon_def *w, int wcount, const cadv_enemy_def *e,
                         int ecount)
{
    f->wdefs = w;
    f->wcount = wcount > CADV_FPS_MAX_WEAPONS ? CADV_FPS_MAX_WEAPONS : wcount;
    f->edefs = e;
    f->ecount = ecount;
}

const cadv_weapon_def *cadv_fps_weapon(const cadv_fps *f, int index)
{
    return (index < 0 || index >= f->wcount) ? NULL : &f->wdefs[index];
}

const cadv_enemy_def *cadv_fps_enemy_def(const cadv_fps *f, int type)
{
    return (type < 0 || type >= f->ecount) ? NULL : &f->edefs[type];
}

int cadv_fps_add_enemy(cadv_fps *f, int type, cadv_vec3 pos, float yaw)
{
    cadv_enemy *e;
    const cadv_enemy_def *d = cadv_fps_enemy_def(f, type);
    if (f->enemy_count >= CADV_FPS_MAX_ENEMIES || !d) {
        return -1;
    }
    e = &f->enemies[f->enemy_count];
    memset(e, 0, sizeof *e);
    e->type = type;
    e->pos = cadv_v3(pos.x, pos.y + d->fly_height, pos.z);
    e->yaw = yaw;
    e->health = d->health;
    e->active = true;
    return f->enemy_count++;
}

int cadv_fps_add_pickup_ex(cadv_fps *f, cadv_pickup_kind kind, int value, int sub, cadv_vec3 pos)
{
    cadv_pickup *p;
    if (f->pickup_count >= CADV_FPS_MAX_PICKUPS) {
        return -1;
    }
    p = &f->pickups[f->pickup_count];
    p->kind = kind;
    p->value = value;
    p->sub = sub;
    p->pos = pos;
    p->active = true;
    return f->pickup_count++;
}

int cadv_fps_add_pickup(cadv_fps *f, cadv_pickup_kind kind, int value, cadv_vec3 pos)
{
    return cadv_fps_add_pickup_ex(f, kind, value, kind == CADV_PICKUP_AMMO ? 1 : 0, pos);
}

int cadv_fps_add_door(cadv_fps *f, cadv_vec3 lo, cadv_vec3 hi, uint32_t key)
{
    cadv_door *d;
    int s;
    if (f->door_count >= CADV_FPS_MAX_DOORS) {
        return -1;
    }
    s = cadv_world_add(&f->world, lo, hi, 100 + f->door_count);
    if (s < 0) {
        return -1;
    }
    d = &f->doors[f->door_count];
    memset(d, 0, sizeof *d);
    d->solid = s;
    d->key = key;
    d->closed_lo = f->world.s[s].box.lo;
    d->closed_hi = f->world.s[s].box.hi;
    return f->door_count++;
}

int cadv_fps_add_breakable(cadv_fps *f, cadv_vec3 lo, cadv_vec3 hi, int hp, int group)
{
    cadv_breakable *b;
    int s;
    if (f->breakable_count >= CADV_FPS_MAX_BREAKABLES) {
        return -1;
    }
    s = cadv_world_add(&f->world, lo, hi, 300 + f->breakable_count);
    if (s < 0) {
        return -1;
    }
    b = &f->breakables[f->breakable_count];
    b->solid = s;
    b->hp = hp;
    b->group = group;
    b->broken = false;
    return f->breakable_count++;
}

int cadv_fps_alive_enemies(const cadv_fps *f)
{
    int i;
    int n = 0;
    for (i = 0; i < f->enemy_count; ++i) {
        n += f->enemies[i].active && f->enemies[i].state != CADV_ENEMY_DEAD;
    }
    return n;
}

int cadv_fps_broken_count(const cadv_fps *f, int group)
{
    int i;
    int n = 0;
    for (i = 0; i < f->breakable_count; ++i) {
        n += f->breakables[i].broken && f->breakables[i].group == group;
    }
    return n;
}

uint32_t cadv_fps_damage_enemy(cadv_fps *f, int index, int dmg)
{
    cadv_enemy *e = &f->enemies[index];
    if (e->state == CADV_ENEMY_DEAD || !e->active) {
        return 0;
    }
    e->awake = true;
    e->health -= dmg;
    if (e->health <= 0) {
        e->state = CADV_ENEMY_DEAD;
        e->timer = 0.0f;
        f->kills++;
        f->last_killed = index;
        return CADV_FPS_EV_ENEMY_HIT | CADV_FPS_EV_ENEMY_KILLED;
    }
    if (e->state != CADV_ENEMY_SHRUNK) {
        e->state = CADV_ENEMY_PAIN;
        e->timer = 0.25f;
    }
    return CADV_FPS_EV_ENEMY_HIT;
}

static uint32_t damage_breakable(cadv_fps *f, int index, int dmg)
{
    cadv_breakable *b = &f->breakables[index];
    if (b->broken) {
        return 0;
    }
    b->hp -= dmg;
    if (b->hp <= 0) {
        b->broken = true;
        f->world.s[b->solid].enabled = false;
        f->last_broken = index;
        return CADV_FPS_EV_BREAK;
    }
    return 0;
}

static int breakable_of_solid(const cadv_fps *f, int solid)
{
    int i;
    for (i = 0; i < f->breakable_count; ++i) {
        if (f->breakables[i].solid == solid) {
            return i;
        }
    }
    return -1;
}

static uint32_t hurt_player(cadv_fps *f, int dmg)
{
    uint32_t ev = CADV_FPS_EV_PLAYER_HURT;
    int scaled = (int)((float)dmg * f->damage_scale + 0.5f);
    if (f->player.health <= 0) {
        return 0;
    }
    if (cadv_player_damage(&f->player, scaled < 1 ? 1 : scaled)) {
        ev |= CADV_FPS_EV_PLAYER_DIED;
    }
    return ev;
}

static float box_distance(cadv_aabb b, cadv_vec3 p)
{
    float dx = cadv_maxf(cadv_maxf(b.lo.x - p.x, 0.0f), p.x - b.hi.x);
    float dy = cadv_maxf(cadv_maxf(b.lo.y - p.y, 0.0f), p.y - b.hi.y);
    float dz = cadv_maxf(cadv_maxf(b.lo.z - p.z, 0.0f), p.z - b.hi.z);
    return cadv_sqrtf(dx * dx + dy * dy + dz * dz);
}

uint32_t cadv_fps_explode(cadv_fps *f, cadv_vec3 at, float radius, int dmg, bool hurts_player)
{
    uint32_t ev = CADV_FPS_EV_EXPLOSION;
    int i;
    if (radius <= 0.0f) {
        radius = 0.5f;
    }
    for (i = 0; i < f->enemy_count; ++i) {
        cadv_enemy *e = &f->enemies[i];
        float d = cadv_v3_dist(at, cadv_v3_add(e->pos, cadv_v3(0, 0.8f, 0)));
        if (e->active && d < radius) {
            ev |= cadv_fps_damage_enemy(f, i, (int)((float)dmg * (1.0f - d / radius)) + 1);
        }
    }
    for (i = 0; i < f->breakable_count; ++i) {
        float d = box_distance(f->world.s[f->breakables[i].solid].box, at);
        if (d < radius) {
            ev |= damage_breakable(f, i, (int)((float)dmg * (1.0f - d / radius)) + 1);
        }
    }
    if (hurts_player) {
        float d = cadv_v3_dist(at, cadv_v3_add(f->player.pos, cadv_v3(0, 0.8f, 0)));
        if (d < radius) {
            ev |= hurt_player(f, (int)((float)dmg * 0.5f * (1.0f - d / radius)) + 1);
        }
    }
    return ev;
}

static void spawn_projectile(cadv_fps *f, cadv_vec3 pos, cadv_vec3 vel, int dmg, float splash,
                             bool from_player, bool remote)
{
    int i;
    for (i = 0; i < CADV_FPS_MAX_PROJECTILES; ++i) {
        if (!f->proj[i].active) {
            cadv_projectile *p = &f->proj[i];
            memset(p, 0, sizeof *p);
            p->pos = pos;
            p->vel = vel;
            p->damage = dmg;
            p->splash = splash;
            p->from_player = from_player;
            p->remote = remote;
            p->gravity = remote ? 12.0f : 0.0f;
            p->active = true;
            p->life = remote ? 600.0f : 6.0f;
            return;
        }
    }
}

uint32_t cadv_fps_detonate(cadv_fps *f)
{
    uint32_t ev = 0;
    int i;
    for (i = 0; i < CADV_FPS_MAX_PROJECTILES; ++i) {
        cadv_projectile *p = &f->proj[i];
        if (p->active && p->remote && p->from_player) {
            ev |= cadv_fps_explode(f, p->pos, p->splash, p->damage, true);
            p->active = false;
        }
    }
    return ev;
}

uint32_t cadv_fps_fire(cadv_fps *f)
{
    const cadv_weapon_def *w = &f->wdefs[f->weapon];
    cadv_vec3 eye = cadv_player_eye(&f->player);
    cadv_vec3 fwd = cadv_player_forward(&f->player);
    uint32_t ev = CADV_FPS_EV_SHOT;
    int p;
    if (w->ammo_per_shot > 0) {
        if (f->ammo[w->ammo_type] < w->ammo_per_shot) {
            return 0;
        }
        f->ammo[w->ammo_type] -= w->ammo_per_shot;
    }
    f->cooldown = w->cooldown;
    f->muzzle_flash = (w->kind == CADV_WK_HITSCAN || w->kind == CADV_WK_PROJECTILE) ? 0.06f : 0.0f;
    if (w->kind == CADV_WK_PROJECTILE || w->kind == CADV_WK_REMOTE) {
        cadv_vec3 vel = cadv_v3_scale(fwd, w->proj_speed);
        if (w->kind == CADV_WK_REMOTE) {
            vel.y += 3.0f; /* lob */
        }
        spawn_projectile(f, cadv_v3_add(eye, cadv_v3_scale(fwd, 0.5f)), vel, w->damage, w->splash, true,
                         w->kind == CADV_WK_REMOTE);
        return ev;
    }
    for (p = 0; p < w->pellets; ++p) {
        float dy = (cadv_rng_float01(&f->rng) - 0.5f) * 2.0f * w->spread;
        float dp = (cadv_rng_float01(&f->rng) - 0.5f) * 2.0f * w->spread;
        cadv_vec3 dir = cadv_v3_from_yaw_pitch(f->player.yaw + dy, f->player.pitch + dp);
        float best = w->range;
        int target = -1;
        int wall_solid = -1;
        int i;
        float t;
        if (cadv_world_raycast(&f->world, eye, dir, w->range, &t, NULL, &wall_solid)) {
            best = t;
        }
        for (i = 0; i < f->enemy_count; ++i) {
            const cadv_enemy *e = &f->enemies[i];
            const cadv_enemy_def *d = &f->edefs[e->type];
            float h = e->state == CADV_ENEMY_SHRUNK ? d->height * 0.3f : d->height;
            if (!e->active || e->state == CADV_ENEMY_DEAD) {
                continue;
            }
            /* generous vertical extent = classic vertical auto-aim */
            if (cadv_ray_cylinder(eye, dir, cadv_v3(e->pos.x, e->pos.y - 0.6f, e->pos.z), d->radius,
                                  h + 1.2f, best, &t) &&
                t < best) {
                best = t;
                target = i;
            }
        }
        if (target >= 0) {
            if (w->kind == CADV_WK_SHRINK) {
                cadv_enemy *e = &f->enemies[target];
                if (f->edefs[e->type].shrinkable && e->state != CADV_ENEMY_SHRUNK) {
                    e->state = CADV_ENEMY_SHRUNK;
                    e->timer = 6.0f;
                    e->awake = true;
                    ev |= CADV_FPS_EV_SHRINK;
                } else if (!f->edefs[e->type].shrinkable) {
                    ev |= cadv_fps_damage_enemy(f, target, w->damage);
                }
            } else {
                ev |= cadv_fps_damage_enemy(f, target, w->damage);
            }
        } else if (wall_solid >= 0 && w->kind != CADV_WK_SHRINK) {
            int b = breakable_of_solid(f, wall_solid);
            if (b >= 0) {
                ev |= damage_breakable(f, b, w->damage);
            }
        }
    }
    return ev;
}

uint32_t cadv_fps_use(cadv_fps *f)
{
    cadv_vec3 eye = cadv_player_eye(&f->player);
    cadv_vec3 dir = cadv_player_forward(&f->player);
    int solid;
    float t;
    int i;
    f->use_solid = -1;
    if (!cadv_world_raycast(&f->world, eye, dir, 2.5f, &t, NULL, &solid)) {
        return 0;
    }
    for (i = 0; i < f->door_count; ++i) {
        cadv_door *d = &f->doors[i];
        if (d->solid == solid) {
            if (d->key && (f->player.keys & d->key) != d->key) {
                return CADV_FPS_EV_DOOR_LOCKED;
            }
            if (!d->opening) {
                d->opening = true;
                return CADV_FPS_EV_DOOR;
            }
            return 0;
        }
    }
    f->use_solid = solid;
    return CADV_FPS_EV_USE_SOLID;
}

static uint32_t tick_enemy(cadv_fps *f, int idx, float dt)
{
    cadv_enemy *e = &f->enemies[idx];
    const cadv_enemy_def *d = &f->edefs[e->type];
    cadv_vec3 eye = cadv_v3_add(e->pos, cadv_v3(0, d->height * 0.85f, 0));
    cadv_vec3 target = cadv_player_eye(&f->player);
    cadv_vec3 to = cadv_v3_sub(f->player.pos, e->pos);
    float dist = cadv_sqrtf(to.x * to.x + to.z * to.z);
    bool sees = f->player.health > 0 && dist < d->sight && cadv_world_line_of_sight(&f->world, eye, target);
    uint32_t ev = 0;
    e->timer -= dt;
    if (sees) {
        e->awake = true;
    }
    switch (e->state) {
    case CADV_ENEMY_IDLE:
        if (sees) {
            e->state = CADV_ENEMY_CHASE;
        }
        break;
    case CADV_ENEMY_PAIN:
        if (e->timer <= 0.0f) {
            e->state = CADV_ENEMY_CHASE;
        }
        break;
    case CADV_ENEMY_SHRUNK:
        if (dist < d->radius * 0.4f + CADV_PLAYER_RADIUS + 0.15f && cadv_absf(f->player.pos.y - e->pos.y) < 0.8f) {
            ev |= cadv_fps_damage_enemy(f, idx, 100000) | CADV_FPS_EV_STOMP;
        } else if (e->timer <= 0.0f) {
            e->state = CADV_ENEMY_CHASE;
        } else if (dist > 0.01f) {
            /* scurry away from the player */
            cadv_vec3 step = cadv_v3_scale(cadv_v3(-to.x / dist, 0.0f, -to.z / dist), d->speed * 0.4f * dt);
            cadv_world_move(&f->world, &e->pos, d->radius * 0.4f, d->height * 0.3f,
                            cadv_v3(step.x, d->fly_height > 0.0f ? 0.0f : -0.05f, step.z), 0.2f);
        }
        break;
    case CADV_ENEMY_CHASE:
        e->yaw = cadv_atan2f(-to.x, -to.z);
        if (dist <= d->attack_range && sees && e->timer <= 0.0f) {
            e->state = CADV_ENEMY_ATTACK;
            e->timer = 0.35f; /* wind-up */
        } else if (dist > d->radius + CADV_PLAYER_RADIUS + 0.2f) {
            cadv_vec3 step = cadv_v3_scale(cadv_v3(to.x / dist, 0.0f, to.z / dist), d->speed * dt);
            cadv_world_move(&f->world, &e->pos, d->radius, d->height,
                            cadv_v3(step.x, d->fly_height > 0.0f ? 0.0f : -0.05f, step.z),
                            d->fly_height > 0.0f ? 0.0f : CADV_PLAYER_STEP);
        }
        break;
    case CADV_ENEMY_ATTACK:
        if (e->timer <= 0.0f) {
            if (d->ranged_projectile) {
                cadv_vec3 dir = cadv_v3_norm(cadv_v3_sub(target, eye));
                spawn_projectile(f, cadv_v3_add(eye, cadv_v3_scale(dir, d->radius + 0.2f)),
                                 cadv_v3_scale(dir, 9.0f), d->damage, 1.5f, false, false);
            } else if (sees && dist <= d->attack_range + 0.3f) {
                float hit_chance = d->attack_range <= 1.6f ? 1.0f : cadv_clampf(1.2f - dist / d->attack_range, 0.25f, 0.9f);
                if (cadv_rng_float01(&f->rng) < hit_chance) {
                    ev |= hurt_player(f, d->damage);
                }
            }
            e->state = CADV_ENEMY_CHASE;
            e->timer = d->attack_cooldown;
        }
        break;
    default:
        break;
    }
    return ev;
}

static bool inside_enemy(const cadv_fps *f, cadv_vec3 p)
{
    int k;
    for (k = 0; k < f->enemy_count; ++k) {
        const cadv_enemy *e = &f->enemies[k];
        const cadv_enemy_def *d = &f->edefs[e->type];
        if (e->active && e->state != CADV_ENEMY_DEAD &&
            cadv_aabb_contains(cadv_aabb_make(cadv_v3(e->pos.x - d->radius, e->pos.y, e->pos.z - d->radius),
                                              cadv_v3(e->pos.x + d->radius, e->pos.y + d->height, e->pos.z + d->radius)),
                               p)) {
            return true;
        }
    }
    return false;
}

static uint32_t tick_projectiles(cadv_fps *f, float dt)
{
    uint32_t ev = 0;
    int i;
    for (i = 0; i < CADV_FPS_MAX_PROJECTILES; ++i) {
        cadv_projectile *p = &f->proj[i];
        cadv_vec3 dir;
        float len;
        float t;
        bool hit = false;
        if (!p->active) {
            continue;
        }
        p->life -= dt;
        if (p->life <= 0.0f) {
            p->active = false;
            continue;
        }
        if (p->stuck) {
            continue;
        }
        p->vel.y -= p->gravity * dt;
        dir = cadv_v3_scale(p->vel, dt);
        len = cadv_v3_len(dir);
        if (len <= 0.0f) {
            p->active = false;
            continue;
        }
        if (cadv_world_raycast(&f->world, p->pos, cadv_v3_scale(dir, 1.0f / len), len, &t, NULL, NULL)) {
            hit = true;
            p->pos = cadv_v3_add(p->pos, cadv_v3_scale(dir, cadv_maxf(0.0f, t - 0.05f) / len));
        } else {
            p->pos = cadv_v3_add(p->pos, dir);
        }
        if (p->remote) {
            if (hit) {
                p->stuck = true; /* waits for the detonator */
                p->vel = cadv_v3(0, 0, 0);
            }
            continue;
        }
        if (!hit) {
            if (p->from_player) {
                hit = inside_enemy(f, p->pos);
            } else {
                cadv_vec3 pp = f->player.pos;
                hit = cadv_aabb_contains(cadv_aabb_make(cadv_v3(pp.x - CADV_PLAYER_RADIUS, pp.y, pp.z - CADV_PLAYER_RADIUS),
                                                        cadv_v3(pp.x + CADV_PLAYER_RADIUS, pp.y + cadv_player_height(&f->player), pp.z + CADV_PLAYER_RADIUS)),
                                         p->pos);
            }
        }
        if (hit) {
            ev |= cadv_fps_explode(f, p->pos, p->splash, p->damage, true);
            p->active = false;
        }
    }
    return ev;
}

static uint32_t tick_pickups(cadv_fps *f)
{
    uint32_t ev = 0;
    int i;
    for (i = 0; i < f->pickup_count; ++i) {
        cadv_pickup *p = &f->pickups[i];
        cadv_vec3 c = cadv_v3_add(f->player.pos, cadv_v3(0, 0.8f, 0));
        if (!p->active || cadv_v3_dist(c, p->pos) > 1.1f) {
            continue;
        }
        switch (p->kind) {
        case CADV_PICKUP_HEALTH:
            if (f->player.health >= 100) {
                continue;
            }
            f->player.health = cadv_mini(100, f->player.health + p->value);
            break;
        case CADV_PICKUP_ARMOR: f->player.armor = cadv_mini(100, f->player.armor + p->value); break;
        case CADV_PICKUP_AMMO:
            if (p->sub >= 0 && p->sub < CADV_AMMO_TYPES) {
                f->ammo[p->sub] = cadv_mini(200, f->ammo[p->sub] + p->value);
            }
            break;
        case CADV_PICKUP_KEY: f->player.keys |= (uint32_t)p->value; break;
        case CADV_PICKUP_WEAPON:
            if (p->value >= 0 && p->value < f->wcount) {
                const cadv_weapon_def *w = &f->wdefs[p->value];
                f->owned[p->value] = true;
                f->weapon = p->value;
                if (w->ammo_per_shot > 0) {
                    f->ammo[w->ammo_type] = cadv_mini(200, f->ammo[w->ammo_type] + 10 * w->ammo_per_shot);
                }
            }
            break;
        case CADV_PICKUP_JETPACK:
            f->jetpack = true;
            f->jet_fuel = CADV_JET_FUEL_MAX;
            break;
        case CADV_PICKUP_FUEL:
            if (f->jet_fuel >= CADV_JET_FUEL_MAX) {
                continue;
            }
            f->jet_fuel = cadv_minf(CADV_JET_FUEL_MAX, f->jet_fuel + (float)p->value);
            break;
        default:
            f->star_taken = true;
            ev |= CADV_FPS_EV_STAR;
            break;
        }
        p->active = false;
        ev |= CADV_FPS_EV_PICKUP;
    }
    return ev;
}

static void tick_doors(cadv_fps *f, float dt)
{
    int i;
    for (i = 0; i < f->door_count; ++i) {
        cadv_door *d = &f->doors[i];
        cadv_solid *s = &f->world.s[d->solid];
        float h;
        if (!d->opening || d->open >= 1.0f) {
            continue;
        }
        d->open = cadv_minf(1.0f, d->open + dt / 0.8f);
        h = d->closed_hi.y - d->closed_lo.y;
        s->box.lo.y = d->closed_lo.y + h * d->open;
        s->box.hi.y = d->closed_hi.y + h * d->open;
        if (d->open >= 1.0f) {
            s->enabled = false;
        }
    }
}

static void select_weapon(cadv_fps *f, const cadv_input_frame *in)
{
    int i;
    for (i = 0; i < 6 && i < f->wcount; ++i) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + i)) && f->owned[i]) {
            f->weapon = i;
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        int dir = cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) ? 1 : f->wcount - 1;
        int w = f->weapon;
        for (i = 0; i < f->wcount; ++i) {
            w = (w + dir) % f->wcount;
            if (f->owned[w]) {
                f->weapon = w;
                break;
            }
        }
    }
}

uint32_t cadv_fps_tick(cadv_fps *f, const cadv_input_frame *in, float dt)
{
    uint32_t ev = 0;
    int i;
    if (f->player.health > 0) {
        const cadv_weapon_def *w;
        cadv_player_tick(&f->player, &f->world, in, dt);
        if (f->jetpack && !f->player.on_ground && cadv_input_down(in, CADV_ACT_JUMP) && f->jet_fuel > 0.0f) {
            f->player.vel.y = cadv_minf(f->player.vel.y + 30.0f * dt, 5.0f);
            f->jet_fuel = cadv_maxf(0.0f, f->jet_fuel - dt);
            ev |= CADV_FPS_EV_JET;
        }
        select_weapon(f, in);
        w = &f->wdefs[f->weapon];
        f->cooldown -= dt;
        if (cadv_input_down(in, CADV_ACT_FIRE) && f->cooldown <= 0.0f) {
            ev |= cadv_fps_fire(f);
        }
        if (cadv_input_pressed(in, CADV_ACT_ALT_FIRE) && w->kind == CADV_WK_REMOTE) {
            ev |= cadv_fps_detonate(f);
        }
        if (cadv_input_pressed(in, CADV_ACT_USE)) {
            ev |= cadv_fps_use(f);
        }
    }
    f->muzzle_flash = cadv_maxf(0.0f, f->muzzle_flash - dt);
    for (i = 0; i < f->enemy_count; ++i) {
        if (f->enemies[i].active && f->enemies[i].state != CADV_ENEMY_DEAD) {
            ev |= tick_enemy(f, i, dt);
        }
    }
    ev |= tick_projectiles(f, dt);
    if (f->player.health > 0) {
        ev |= tick_pickups(f);
    }
    tick_doors(f, dt);
    return ev;
}
