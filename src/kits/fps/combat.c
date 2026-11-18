/* SPDX-License-Identifier: MIT */
#include "kits/fps/combat.h"

#include <string.h>

static const cadv_weapon_def k_weapons[CADV_FPS_WEAPONS] = {
    /* name           dmg pel spread cool  range ammo proj  speed splash */
    {"Candy Cane",    25, 1, 0.00f, 0.45f, 1.8f, 0, false, 0.0f, 0.0f},
    {"Snow Pistol",   14, 1, 0.02f, 0.30f, 60.f, 0, false, 0.0f, 0.0f},
    {"Scattershot",    8, 7, 0.09f, 0.85f, 30.f, 1, false, 0.0f, 0.0f},
    {"Gift Launcher", 60, 1, 0.00f, 1.00f, 80.f, 1, true, 18.0f, 2.5f},
};

static const cadv_enemy_def k_enemies[CADV_ENEMY_TYPES] = {
    /* name          hp speed  r     h    sight range cool dmg proj */
    {"Grunt",        40, 2.4f, 0.35f, 1.6f, 18.f, 14.f, 1.4f, 6, false},
    {"Charger",      60, 4.2f, 0.45f, 1.5f, 14.f, 1.4f, 0.8f, 12, false},
    {"Lobber",       50, 1.8f, 0.40f, 1.8f, 24.f, 20.f, 2.2f, 15, true},
};

const cadv_weapon_def *cadv_weapon_def_get(int index)
{
    if (index < 0 || index >= CADV_FPS_WEAPONS) {
        return NULL;
    }
    return &k_weapons[index];
}

const cadv_enemy_def *cadv_enemy_def_get(int index)
{
    if (index < 0 || index >= CADV_ENEMY_TYPES) {
        return NULL;
    }
    return &k_enemies[index];
}

void cadv_fps_init(cadv_fps *f, uint64_t seed)
{
    memset(f, 0, sizeof *f);
    cadv_world_clear(&f->world);
    cadv_rng_seed(&f->rng, seed, 7u);
    f->owned[0] = true;
    f->owned[1] = true;
    f->weapon = 1;
    f->damage_scale = 1.0f;
    cadv_player_spawn(&f->player, cadv_v3(0, 0, 0), 0.0f);
}

int cadv_fps_add_enemy(cadv_fps *f, int type, cadv_vec3 pos, float yaw)
{
    cadv_enemy *e;
    if (f->enemy_count >= CADV_FPS_MAX_ENEMIES || !cadv_enemy_def_get(type)) {
        return -1;
    }
    e = &f->enemies[f->enemy_count];
    memset(e, 0, sizeof *e);
    e->type = type;
    e->pos = pos;
    e->yaw = yaw;
    e->health = k_enemies[type].health;
    e->active = true;
    return f->enemy_count++;
}

int cadv_fps_add_pickup(cadv_fps *f, cadv_pickup_kind kind, int value, cadv_vec3 pos)
{
    cadv_pickup *p;
    if (f->pickup_count >= CADV_FPS_MAX_PICKUPS) {
        return -1;
    }
    p = &f->pickups[f->pickup_count];
    p->kind = kind;
    p->value = value;
    p->pos = pos;
    p->active = true;
    return f->pickup_count++;
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

int cadv_fps_alive_enemies(const cadv_fps *f)
{
    int i;
    int n = 0;
    for (i = 0; i < f->enemy_count; ++i) {
        n += f->enemies[i].active && f->enemies[i].state != CADV_ENEMY_DEAD;
    }
    return n;
}

static uint32_t damage_enemy(cadv_fps *f, cadv_enemy *e, int dmg)
{
    if (e->state == CADV_ENEMY_DEAD) {
        return 0;
    }
    e->health -= dmg;
    if (e->health <= 0) {
        e->state = CADV_ENEMY_DEAD;
        e->timer = 0.0f;
        f->kills++;
        return CADV_FPS_EV_ENEMY_HIT | CADV_FPS_EV_ENEMY_KILLED;
    }
    e->state = CADV_ENEMY_PAIN;
    e->timer = 0.25f;
    return CADV_FPS_EV_ENEMY_HIT;
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

static uint32_t explode(cadv_fps *f, cadv_vec3 at, float radius, int dmg, bool hurts_player)
{
    uint32_t ev = CADV_FPS_EV_EXPLOSION;
    int i;
    for (i = 0; i < f->enemy_count; ++i) {
        cadv_enemy *e = &f->enemies[i];
        float d = cadv_v3_dist(at, cadv_v3_add(e->pos, cadv_v3(0, 0.8f, 0)));
        if (e->active && d < radius) {
            ev |= damage_enemy(f, e, (int)((float)dmg * (1.0f - d / radius)) + 1);
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
                             bool from_player)
{
    int i;
    for (i = 0; i < CADV_FPS_MAX_PROJECTILES; ++i) {
        if (!f->proj[i].active) {
            cadv_projectile *p = &f->proj[i];
            p->pos = pos;
            p->vel = vel;
            p->damage = dmg;
            p->splash = splash;
            p->from_player = from_player;
            p->active = true;
            p->life = 6.0f;
            return;
        }
    }
}

uint32_t cadv_fps_fire(cadv_fps *f)
{
    const cadv_weapon_def *w = &k_weapons[f->weapon];
    cadv_vec3 eye = cadv_player_eye(&f->player);
    uint32_t ev = CADV_FPS_EV_SHOT;
    int p;
    if (w->ammo_per_shot > 0) {
        if (f->ammo < w->ammo_per_shot) {
            return 0;
        }
        f->ammo -= w->ammo_per_shot;
    }
    f->cooldown = w->cooldown;
    f->muzzle_flash = w->range > 2.0f ? 0.06f : 0.0f;
    if (w->projectile) {
        cadv_vec3 dir = cadv_player_forward(&f->player);
        spawn_projectile(f, cadv_v3_add(eye, cadv_v3_scale(dir, 0.5f)), cadv_v3_scale(dir, w->proj_speed),
                         w->damage, w->splash, true);
        return ev;
    }
    for (p = 0; p < w->pellets; ++p) {
        float dy = (cadv_rng_float01(&f->rng) - 0.5f) * 2.0f * w->spread;
        float dp = (cadv_rng_float01(&f->rng) - 0.5f) * 2.0f * w->spread;
        cadv_vec3 dir = cadv_v3_from_yaw_pitch(f->player.yaw + dy, f->player.pitch + dp);
        float wall = w->range;
        float best = w->range;
        int target = -1;
        int i;
        float t;
        if (cadv_world_raycast(&f->world, eye, dir, w->range, &t, NULL, NULL)) {
            wall = t;
            best = t;
        }
        for (i = 0; i < f->enemy_count; ++i) {
            const cadv_enemy *e = &f->enemies[i];
            const cadv_enemy_def *d = &k_enemies[e->type];
            if (!e->active || e->state == CADV_ENEMY_DEAD) {
                continue;
            }
            /* generous vertical extent = classic vertical auto-aim */
            if (cadv_ray_cylinder(eye, dir, cadv_v3(e->pos.x, e->pos.y - 0.6f, e->pos.z), d->radius,
                                  d->height + 1.2f, best, &t) &&
                t < best) {
                best = t;
                target = i;
            }
        }
        (void)wall;
        if (target >= 0) {
            ev |= damage_enemy(f, &f->enemies[target], w->damage);
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
        }
    }
    return 0;
}

static uint32_t tick_enemy(cadv_fps *f, cadv_enemy *e, float dt)
{
    const cadv_enemy_def *d = &k_enemies[e->type];
    cadv_vec3 eye = cadv_v3_add(e->pos, cadv_v3(0, d->height * 0.85f, 0));
    cadv_vec3 target = cadv_player_eye(&f->player);
    cadv_vec3 to = cadv_v3_sub(f->player.pos, e->pos);
    float dist = cadv_sqrtf(to.x * to.x + to.z * to.z);
    bool sees = f->player.health > 0 && dist < d->sight && cadv_world_line_of_sight(&f->world, eye, target);
    uint32_t ev = 0;
    e->timer -= dt;
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
    case CADV_ENEMY_CHASE:
        e->yaw = cadv_atan2f(-to.x, -to.z);
        if (dist <= d->attack_range && sees && e->timer <= 0.0f) {
            e->state = CADV_ENEMY_ATTACK;
            e->timer = 0.35f; /* wind-up */
        } else if (dist > d->radius + CADV_PLAYER_RADIUS + 0.2f) {
            cadv_vec3 step = cadv_v3_scale(cadv_v3(to.x / dist, 0.0f, to.z / dist), d->speed * dt);
            cadv_world_move(&f->world, &e->pos, d->radius, d->height, cadv_v3(step.x, -0.05f, step.z),
                            CADV_PLAYER_STEP);
        }
        break;
    case CADV_ENEMY_ATTACK:
        if (e->timer <= 0.0f) {
            if (d->ranged_projectile) {
                cadv_vec3 dir = cadv_v3_norm(cadv_v3_sub(target, eye));
                spawn_projectile(f, cadv_v3_add(eye, cadv_v3_scale(dir, d->radius + 0.2f)),
                                 cadv_v3_scale(dir, 9.0f), d->damage, 1.5f, false);
            } else if (sees && dist <= d->attack_range + 0.3f) {
                /* hitscan / melee with distance-based accuracy */
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
        int k;
        if (!p->active) {
            continue;
        }
        p->life -= dt;
        dir = cadv_v3_scale(p->vel, dt);
        len = cadv_v3_len(dir);
        if (p->life <= 0.0f || len <= 0.0f) {
            p->active = false;
            continue;
        }
        if (cadv_world_raycast(&f->world, p->pos, cadv_v3_scale(dir, 1.0f / len), len, &t, NULL, NULL)) {
            hit = true;
            p->pos = cadv_v3_add(p->pos, cadv_v3_scale(dir, t / len));
        } else {
            p->pos = cadv_v3_add(p->pos, dir);
        }
        if (!hit && p->from_player) {
            for (k = 0; k < f->enemy_count && !hit; ++k) {
                const cadv_enemy *e = &f->enemies[k];
                const cadv_enemy_def *d = &k_enemies[e->type];
                if (e->active && e->state != CADV_ENEMY_DEAD &&
                    cadv_aabb_contains(cadv_aabb_make(cadv_v3(e->pos.x - d->radius, e->pos.y, e->pos.z - d->radius),
                                                      cadv_v3(e->pos.x + d->radius, e->pos.y + d->height, e->pos.z + d->radius)),
                                       p->pos)) {
                    hit = true;
                }
            }
        } else if (!hit) {
            cadv_vec3 pp = f->player.pos;
            hit = cadv_aabb_contains(cadv_aabb_make(cadv_v3(pp.x - CADV_PLAYER_RADIUS, pp.y, pp.z - CADV_PLAYER_RADIUS),
                                                    cadv_v3(pp.x + CADV_PLAYER_RADIUS, pp.y + cadv_player_height(&f->player), pp.z + CADV_PLAYER_RADIUS)),
                                     p->pos);
        }
        if (hit) {
            ev |= explode(f, p->pos, p->splash, p->damage, true);
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
        case CADV_PICKUP_AMMO: f->ammo = cadv_mini(200, f->ammo + p->value); break;
        case CADV_PICKUP_KEY: f->player.keys |= (uint32_t)p->value; break;
        case CADV_PICKUP_WEAPON:
            if (p->value >= 0 && p->value < CADV_FPS_WEAPONS) {
                f->owned[p->value] = true;
                f->weapon = p->value;
                f->ammo = cadv_mini(200, f->ammo + 10);
            }
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
    for (i = 0; i < CADV_FPS_WEAPONS; ++i) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + i)) && f->owned[i]) {
            f->weapon = i;
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        int dir = cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) ? 1 : CADV_FPS_WEAPONS - 1;
        int w = f->weapon;
        for (i = 0; i < CADV_FPS_WEAPONS; ++i) {
            w = (w + dir) % CADV_FPS_WEAPONS;
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
        cadv_player_tick(&f->player, &f->world, in, dt);
        select_weapon(f, in);
        f->cooldown -= dt;
        if (cadv_input_down(in, CADV_ACT_FIRE) && f->cooldown <= 0.0f) {
            ev |= cadv_fps_fire(f);
        }
        if (cadv_input_pressed(in, CADV_ACT_USE)) {
            ev |= cadv_fps_use(f);
        }
    }
    f->muzzle_flash = cadv_maxf(0.0f, f->muzzle_flash - dt);
    for (i = 0; i < f->enemy_count; ++i) {
        if (f->enemies[i].active && f->enemies[i].state != CADV_ENEMY_DEAD) {
            ev |= tick_enemy(f, &f->enemies[i], dt);
        }
    }
    ev |= tick_projectiles(f, dt);
    if (f->player.health > 0) {
        ev |= tick_pickups(f);
    }
    tick_doors(f, dt);
    return ev;
}
