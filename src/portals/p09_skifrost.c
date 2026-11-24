/* SPDX-License-Identifier: MIT */
/* Day 9 - Skifrost: Capture the Flag on Snowglobe Basin. A short training
 * course (ski, jet, disc), then the Rime Wolves against the Ember Owls, four
 * a side: skiing and jetpacks, slow discs with splash and knockback, a
 * chaingun and a grenade launcher, three armours, generators that power the
 * turrets and inventory stations, flags that drop and return on a timer,
 * bots that ski precomputed downhill lines in roles. The winner carries the
 * trophy star home on a victory lap. */
#include "portals/p09_skifrost.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "gfx/procgen.h"
#include "hub/friends.h"
#include "portals/p09_internal.h"

static const char *const k_weapon_names[P09_WEAPONS] = {"DISC", "CHAINGUN", "GRENADE"};
static const char *const k_tribe[2] = {"RIME WOLVES", "EMBER OWLS"};
static const float k_base_z[2] = {70.0f, 442.0f};
#define BASE_X 256.0f

/* ---- helpers ---------------------------------------------------------------------- */

static void copy_text(char *out, size_t cap, const char *text) { snprintf(out, cap, "%s", text); }

static void say(p09_state *s, const char *text)
{
    copy_text(s->message, sizeof s->message, text);
    s->message_timer = 240;
}

static void feed(p09_state *s, const char *text)
{
    int i;
    for (i = P09_FEED - 1; i > 0; --i) {
        memcpy(s->feed[i], s->feed[i - 1], sizeof s->feed[i]);
        s->feed_t[i] = s->feed_t[i - 1];
    }
    copy_text(s->feed[0], sizeof s->feed[0], text);
    s->feed_t[0] = 360;
}

static void sfx(p09_state *s, cadv_sfx id, float vol, cadv_vec3 at)
{
    if (s->cfg.audio) {
        float d = cadv_v3_dist(at, s->a[0].k.pos);
        cadv_audio_sfx(s->cfg.audio, id, vol * cadv_clampf(1.0f - d / 300.0f, 0.05f, 1.0f), 0.0f);
    }
}

static float flat(cadv_vec3 a, cadv_vec3 b) { return cadv_sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }
static float yaw_to(cadv_vec3 from, cadv_vec3 to) { return cadv_atan2f(-(to.x - from.x), -(to.z - from.z)); }
static float rnd_pm(p09_state *s) { return cadv_rng_float01(&s->rng) * 2.0f - 1.0f; }
static float ground(const p09_state *s, float x, float z) { return cadv_terrain_height(&s->terrain, x, z); }

cadv_vec3 cadv_p09_centre(const p09_agent *a) { return cadv_v3(a->k.pos.x, a->k.pos.y + 1.0f, a->k.pos.z); }

bool cadv_p09_in_halfpipe(cadv_vec3 p) { return cadv_absf(p.x - 130.0f) < 15.0f && cadv_absf(p.z - 256.0f) < 80.0f; }

static bool carrying(const p09_state *s, int i) { return s->flag[1 - s->a[i].team].carrier == i; }

static bool sees(const p09_state *s, cadv_vec3 a, cadv_vec3 b)
{
    return cadv_terrain_line_clear(&s->terrain, a, b) && cadv_world_line_of_sight(&s->world, a, b);
}

/* metres of aim error at 50 m */
float cadv_p09_skill(const p09_state *s, int i)
{
    static const float enemy[3] = {6.0f, 3.8f, 3.0f};
    if (i == 0) {
        return 0.9f;
    }
    if (i < P09_TEAM) {
        return 2.2f;
    }
    return enemy[s->cfg.difficulty];
}

/* ---- the map ---------------------------------------------------------------------- */

static float smoothstep(float e0, float e1, float x)
{
    float t = cadv_clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float basin_height(uint32_t seed, float x, float z)
{
    float h = 12.0f + 14.0f * cadv_fbm2(seed, x / 90.0f, z / 90.0f, 64, 4);
    float r = cadv_sqrtf((x - 256.0f) * (x - 256.0f) + (z - 256.0f) * (z - 256.0f));
    int b;
    /* the rim of the snowglobe */
    if (r > 195.0f) {
        h += (r - 195.0f) * 0.75f;
    }
    /* a low ridge across the middle, so every run needs a hop */
    h += 11.0f * (1.0f - smoothstep(0.0f, 34.0f, cadv_absf(z - 256.0f)));
    /* the two base hills: flat tops, long slopes for skiing */
    for (b = 0; b < 2; ++b) {
        float d = cadv_sqrtf((x - BASE_X) * (x - BASE_X) + (z - k_base_z[b]) * (z - k_base_z[b]));
        h += 44.0f * (1.0f - smoothstep(24.0f, 96.0f, d));
    }
    /* the halfpipe: a steep trench on the west side */
    {
        float dx = cadv_absf(x - 130.0f);
        float w = 1.0f - smoothstep(66.0f, 84.0f, cadv_absf(z - 256.0f));
        if (dx < 16.0f) {
            h -= 15.0f * (1.0f - (dx / 16.0f) * (dx / 16.0f)) * w;
        }
    }
    return h;
}

static void add_box(p09_state *s, cadv_vec3 c, cadv_vec3 half)
{
    cadv_world_add(&s->world, cadv_v3_sub(c, half), cadv_v3_add(c, half), 0);
}

static void build_map(p09_state *s)
{
    int x;
    int z;
    int b;
    uint32_t seed = 909u;
    cadv_terrain_init(&s->terrain, P09_N, P09_CELL, 0.0f, 260.0f, -50.0f);
    for (z = 0; z < P09_N; ++z) {
        for (x = 0; x < P09_N; ++x) {
            s->terrain.h[z * P09_N + x] = basin_height(seed, (float)x * P09_CELL, (float)z * P09_CELL);
        }
    }
    cadv_world_clear(&s->world);
    for (b = 0; b < 2; ++b) {
        p09_base *pb = &s->base[b];
        float sg = b == 0 ? 1.0f : -1.0f; /* towards the enemy along z */
        float zb = k_base_z[b];
        float top = ground(s, BASE_X, zb);
        int k;
        /* flatten the plateau to the height at its centre */
        for (z = 0; z < P09_N; ++z) {
            for (x = 0; x < P09_N; ++x) {
                float d = cadv_sqrtf(((float)x * P09_CELL - BASE_X) * ((float)x * P09_CELL - BASE_X) + ((float)z * P09_CELL - zb) * ((float)z * P09_CELL - zb));
                if (d < 22.0f) {
                    s->terrain.h[z * P09_N + x] = top;
                }
            }
        }
        pb->face = b == 0 ? CADV_PI : 0.0f;
        /* a low stand: skiers glide over it and snatch the flag at speed */
        pb->stand = cadv_v3(BASE_X, top + 0.25f, zb);
        add_box(s, cadv_v3(BASE_X, top - 0.375f, zb), cadv_v3(1.5f, 0.625f, 1.5f));
        pb->gen = cadv_v3(BASE_X, top + 1.6f, zb - sg * 16.0f);
        add_box(s, pb->gen, cadv_v3(1.5f, 1.6f, 1.5f));
        /* a low bunker wall in front of the generator */
        add_box(s, cadv_v3(BASE_X, top + 1.0f, zb - sg * 12.5f), cadv_v3(4.0f, 1.0f, 0.4f));
        pb->turret.pos = cadv_v3(BASE_X + 12.0f, top + 6.6f, zb + sg * 7.0f);
        pb->turret.range = 62.0f;
        pb->turret.cooldown = 3.0f;
        add_box(s, cadv_v3(BASE_X + 12.0f, top + 3.0f, zb + sg * 7.0f), cadv_v3(0.5f, 3.0f, 0.5f));
        pb->station = cadv_v3(BASE_X - 12.0f, top, zb - sg * 6.0f);
        add_box(s, cadv_v3(BASE_X - 12.0f, top + 1.1f, zb - sg * 6.0f), cadv_v3(0.8f, 1.1f, 0.8f));
        for (k = 0; k < P09_TEAM; ++k) {
            pb->spawn[k] = cadv_v3(BASE_X + (k % 2 == 0 ? -11.0f : 11.0f), top, zb - sg * (k < 2 ? 16.0f : 19.0f));
        }
        cadv_generator_init(&pb->power, 500.0f, 50.0f);
        pb->turret_target = -1;
        cadv_flag_init(&s->flag[b], b, pb->stand);
    }
    cadv_terrain_mark_all_dirty(&s->terrain);
}

/* Two ski lines from base 0 to base 1 down the valleys: the middle, and the
 * west flank through the halfpipe. Each point picks the lowest ground near
 * its lane, kept smooth. */
static void build_routes(p09_state *s)
{
    int r;
    int i;
    for (r = 0; r < 2; ++r) {
        float prev = BASE_X;
        for (i = 0; i < P09_ROUTE; ++i) {
            float f = (float)i / (float)(P09_ROUTE - 1);
            float z = cadv_lerpf(k_base_z[0], k_base_z[1], f);
            float lane = r == 0 ? BASE_X : cadv_lerpf(BASE_X, 132.0f, cadv_minf(1.0f, cadv_minf(f, 1.0f - f) * 4.0f));
            float best = lane;
            float best_cost = 1e9f;
            float x;
            if (i == 0 || i == P09_ROUTE - 1) {
                s->route[r][i] = s->base[i == 0 ? 0 : 1].stand;
                continue;
            }
            for (x = lane - 40.0f; x <= lane + 40.0f; x += 4.0f) {
                float c = ground(s, x, z) + 0.02f * (x - prev) * (x - prev) + 0.01f * (x - lane) * (x - lane);
                if (c < best_cost) {
                    best_cost = c;
                    best = x;
                }
            }
            prev = best;
            s->route[r][i] = cadv_v3(best, ground(s, best, z), z);
        }
    }
}

/* ---- agents ------------------------------------------------------------------------- */

static cadv_armour_class role_armour(int role)
{
    return role == P09_ROLE_CAP ? CADV_ARMOUR_LIGHT : role == P09_ROLE_HOFF ? CADV_ARMOUR_HEAVY : CADV_ARMOUR_MEDIUM;
}

static void spawn(p09_state *s, int i)
{
    p09_agent *a = &s->a[i];
    const p09_base *b = &s->base[a->team];
    cadv_armour_class arm = i == 0 && a->deaths + a->kills + a->caps > 0 ? a->k.armour : role_armour(a->role);
    bool pack = i == 0 ? (a->deaths + a->kills + a->caps > 0 ? a->k.energy_pack : true) : arm != CADV_ARMOUR_HEAVY;
    cadv_skier_init(&a->k, arm, b->spawn[i % P09_TEAM], b->face);
    a->k.energy_pack = pack;
    a->k.on_ground = true;
    a->alive = true;
    a->invuln = 2.0f;
    a->weapon = P09_DISC;
    a->cd = 0.0f;
    a->spin = 0.0f;
    a->carry_max = 0.0f;
    a->target = -1;
    a->route = a->role == P09_ROLE_CAP ? 1 : 0; /* cappers take the flank, the heavy the middle */
    a->route_pos = 0;
    a->jet_ok = true;
    a->stuck = 0;
    a->last_hit_by = -1;
}

static void drop_flag(p09_state *s, int i, cadv_vec3 extra)
{
    int t = 1 - s->a[i].team;
    if (s->flag[t].carrier == i) {
        s->drop_agent[t] = i;
        s->drop_lock[t] = 60; /* no catching your own toss */
        cadv_flag_drop(&s->flag[t], cadv_p09_centre(&s->a[i]), cadv_v3_add(s->a[i].k.vel, extra));
        s->flag[t].vel = cadv_v3_add(s->flag[t].vel, cadv_v3(0, 4.0f, 0));
    }
}

static void kill(p09_state *s, int victim, int killer, int weapon)
{
    p09_agent *v = &s->a[victim];
    char buf[96];
    bool special = weapon == P09_DISC && !v->k.on_ground && v->k.airtime > 0.5f && killer >= 0 && killer != victim;
    v->alive = false;
    v->respawn = P09_RESPAWN;
    ++v->deaths;
    drop_flag(s, victim, cadv_v3(0, 0, 0));
    if (killer >= 0 && killer != victim) {
        ++s->a[killer].kills;
        snprintf(buf, sizeof buf, "%s [%s] %s", s->names[killer], k_weapon_names[weapon], s->names[victim]);
    } else if (killer < -1) {
        snprintf(buf, sizeof buf, "TURRET [DISC] %s", s->names[victim]);
    } else {
        snprintf(buf, sizeof buf, "%s BLEW THEMSELVES UP", s->names[victim]);
    }
    feed(s, buf);
    if (special) {
        /* the kill-feed announcer's name for a mid-air disc */
        snprintf(buf, sizeof buf, "%s!", s->mark_text[0]);
        feed(s, buf);
        s->marks_found |= 1u;
    }
    sfx(s, CADV_SFX_HIT, 0.8f, v->k.pos);
}

static void hurt(p09_state *s, int victim, int attacker, float dmg, int weapon)
{
    p09_agent *v = &s->a[victim];
    if (!v->alive || v->invuln > 0.0f || dmg <= 0.0f || s->phase != P09_PH_MATCH) {
        return;
    }
    v->k.hp -= dmg;
    v->last_hit_by = attacker;
    if (victim == 0) {
        s->dmg_taken[attacker >= 0 ? attacker : P09_AGENTS + (-2 - attacker)] += dmg;
    }
    v->calm = 0.0f;
    if (victim == 0) {
        s->hurt_flash = 12;
    }
    if (attacker == 0 && victim != 0) {
        s->hit_mark = 10;
    }
    if (v->k.hp <= 0.0f) {
        kill(s, victim, attacker, weapon);
    }
}

/* ---- weapons ------------------------------------------------------------------------ */

static bool ray_sphere(cadv_vec3 o, cadv_vec3 d, cadv_vec3 c, float r, float max_d, float *t)
{
    cadv_vec3 oc = cadv_v3_sub(o, c);
    float b = cadv_v3_dot(oc, d);
    float cc = cadv_v3_dot(oc, oc) - r * r;
    float disc = b * b - cc;
    float tt;
    if (disc < 0.0f) {
        return false;
    }
    tt = -b - cadv_sqrtf(disc);
    if (tt < 0.0f) {
        tt = -b + cadv_sqrtf(disc);
    }
    if (tt < 0.0f || tt > max_d) {
        return false;
    }
    *t = tt;
    return true;
}

static void launch(p09_state *s, int kind, int owner, int team, cadv_vec3 pos, cadv_vec3 vel)
{
    int k;
    for (k = 0; k < P09_PROJ; ++k) {
        p09_proj *p = &s->proj[k];
        if (!p->active) {
            p->active = true;
            p->kind = kind;
            p->owner = owner;
            p->team = team;
            p->pos = pos;
            p->vel = vel;
            p->fuse = kind == P09_PJ_GRENADE ? 2.0f : 4.0f;
            return;
        }
    }
}

static void explode(p09_state *s, const p09_proj *p, cadv_vec3 at, int direct)
{
    float radius = p->kind == P09_PJ_GRENADE ? 7.5f : 7.0f;
    float dmg = p->kind == P09_PJ_GRENADE ? 55.0f : p->kind == P09_PJ_TURRET ? 20.0f : 46.0f;
    float knock = p->kind == P09_PJ_GRENADE ? 18.0f : 26.0f;
    int weapon = p->kind == P09_PJ_GRENADE ? P09_GRENADE : P09_DISC;
    int i;
    for (i = 0; i < P09_AGENTS; ++i) {
        p09_agent *a = &s->a[i];
        cadv_vec3 c = cadv_p09_centre(a);
        float d = cadv_v3_dist(c, at);
        float f;
        if (!a->alive || (d >= radius && i != direct)) {
            continue;
        }
        f = i == direct ? 1.0f : 1.0f - d / radius;
        if (a->team == p->team && i != p->owner) {
            continue; /* no team damage, no team pushes */
        }
        if (s->phase == P09_PH_MATCH || i == p->owner) {
            cadv_vec3 dir = cadv_v3_norm(cadv_v3_add(cadv_v3_sub(c, at), cadv_v3(0, 0.6f, 0)));
            cadv_skier_push(&a->k, cadv_v3_scale(dir, knock * f));
        }
        hurt(s, i, p->owner, dmg * f * (i == p->owner ? 0.35f : 1.0f), weapon);
    }
    for (i = 0; i < 2; ++i) {
        p09_base *b = &s->base[i];
        float d = cadv_v3_dist(b->gen, at) - 1.5f;
        if (i != p->team && d < radius && s->phase == P09_PH_MATCH && cadv_generator_damage(&b->power, dmg * (1.0f - cadv_maxf(0.0f, d) / radius) * 1.4f)) {
            char buf[80];
            snprintf(buf, sizeof buf, "THE %s' GENERATOR IS DOWN!", k_tribe[i]);
            say(s, buf);
            sfx(s, CADV_SFX_EXPLOSION, 1.0f, b->gen);
        }
    }
    if (s->phase == P09_PH_TRAIN && s->train == P09_TR_DISC && p->owner == 0 && cadv_v3_dist(at, s->drone) < 3.5f) {
        s->train = P09_TR_DONE;
        say(s, "DIRECT HIT! THE MATCH IS ABOUT TO START");
    }
    s->boom_pos = at;
    s->boom_t = 12;
    sfx(s, CADV_SFX_EXPLOSION, 0.6f, at);
}

static void tick_proj(p09_state *s)
{
    int k;
    for (k = 0; k < P09_PROJ; ++k) {
        p09_proj *p = &s->proj[k];
        cadv_vec3 prev;
        cadv_vec3 d;
        float len;
        float best;
        int hit = -1;
        int i;
        if (!p->active) {
            continue;
        }
        prev = p->pos;
        if (p->kind == P09_PJ_GRENADE) {
            p->vel.y -= CADV_SKIER_GRAVITY * P09_DT;
        }
        p->pos = cadv_v3_add(p->pos, cadv_v3_scale(p->vel, P09_DT));
        p->fuse -= P09_DT;
        d = cadv_v3_sub(p->pos, prev);
        len = cadv_v3_len(d);
        best = len;
        if (len > 1e-5f) {
            float t;
            cadv_vec3 n;
            d = cadv_v3_scale(d, 1.0f / len);
            for (i = 0; i < P09_AGENTS; ++i) {
                const p09_agent *a = &s->a[i];
                if (a->alive && i != p->owner && (a->team != p->team || p->kind != P09_PJ_TURRET) && ray_sphere(prev, d, cadv_p09_centre(a), 1.0f, best, &t)) {
                    best = t;
                    hit = i;
                }
            }
            if (s->phase == P09_PH_TRAIN && ray_sphere(prev, d, s->drone, 1.2f, best, &t)) {
                best = t;
                hit = -2;
            }
            if (cadv_world_raycast(&s->world, prev, d, best, &t, &n, NULL)) {
                if (p->kind == P09_PJ_GRENADE) {
                    float into = cadv_v3_dot(p->vel, n);
                    p->pos = cadv_v3_add(prev, cadv_v3_scale(d, t - 0.05f));
                    p->vel = cadv_v3_scale(cadv_v3_sub(p->vel, cadv_v3_scale(n, 2.0f * into)), 0.45f);
                    continue;
                }
                explode(s, p, cadv_v3_add(prev, cadv_v3_scale(d, t)), -1);
                p->active = false;
                continue;
            }
            if (hit != -1) {
                explode(s, p, cadv_v3_add(prev, cadv_v3_scale(d, best)), hit);
                p->active = false;
                continue;
            }
        }
        {
            float h = ground(s, p->pos.x, p->pos.z);
            if (p->pos.y <= h) {
                if (p->kind == P09_PJ_GRENADE && p->fuse > 0.0f) {
                    cadv_vec3 n = cadv_terrain_normal(&s->terrain, p->pos.x, p->pos.z);
                    float into = cadv_v3_dot(p->vel, n);
                    p->pos.y = h + 0.05f;
                    p->vel = cadv_v3_scale(cadv_v3_sub(p->vel, cadv_v3_scale(n, 2.0f * into)), 0.45f);
                } else {
                    explode(s, p, cadv_v3(p->pos.x, h, p->pos.z), -1);
                    p->active = false;
                    continue;
                }
            }
        }
        if (p->fuse <= 0.0f) {
            explode(s, p, p->pos, -1);
            p->active = false;
        }
    }
}

static void chaingun(p09_state *s, int i)
{
    p09_agent *a = &s->a[i];
    cadv_vec3 o = cadv_skier_eye(&a->k);
    cadv_vec3 f = cadv_skier_forward(&a->k);
    float best = 150.0f;
    int hit = -1;
    int k;
    float t;
    f = cadv_v3_norm(cadv_v3(f.x + rnd_pm(s) * 0.045f, f.y + rnd_pm(s) * 0.045f, f.z + rnd_pm(s) * 0.045f));
    if (cadv_world_raycast(&s->world, o, f, best, &t, NULL, NULL)) {
        best = t;
    }
    {
        cadv_vec3 hp;
        if (cadv_terrain_raycast(&s->terrain, o, f, best, &hp)) {
            best = cadv_v3_dist(o, hp);
        }
    }
    for (k = 0; k < P09_AGENTS; ++k) {
        if (k != i && s->a[k].alive && s->a[k].team != a->team && ray_sphere(o, f, cadv_p09_centre(&s->a[k]), 0.9f, best, &t)) {
            best = t;
            hit = k;
        }
    }
    if (hit >= 0) {
        hurt(s, hit, i, 2.6f, P09_CHAIN);
    } else if (s->phase == P09_PH_MATCH) {
        const p09_base *b = &s->base[1 - a->team];
        cadv_aabb box = cadv_aabb_make(cadv_v3_sub(b->gen, cadv_v3(1.5f, 1.6f, 1.5f)), cadv_v3_add(b->gen, cadv_v3(1.5f, 1.6f, 1.5f)));
        if (cadv_ray_aabb(o, f, box, best + 0.1f, &t, NULL)) {
            cadv_generator_damage(&s->base[1 - a->team].power, 3.5f);
        }
    }
}

static void fire(p09_state *s, int i, bool held)
{
    p09_agent *a = &s->a[i];
    cadv_vec3 eye = cadv_skier_eye(&a->k);
    cadv_vec3 f = cadv_skier_forward(&a->k);
    if (a->weapon == P09_CHAIN) {
        a->spin = held ? cadv_minf(1.0f, a->spin + P09_DT * 2.5f) : cadv_maxf(0.0f, a->spin - P09_DT * 1.5f);
        if (held && a->spin >= 1.0f && a->cd <= 0.0f) {
            a->cd = 0.07f;
            chaingun(s, i);
            if (s->ticks % 3 == 0) {
                sfx(s, CADV_SFX_SHOT, 0.25f, a->k.pos);
            }
        }
        return;
    }
    a->spin = cadv_maxf(0.0f, a->spin - P09_DT * 1.5f);
    if (!held || a->cd > 0.0f) {
        return;
    }
    if (a->weapon == P09_DISC) {
        a->cd = 1.25f;
        launch(s, P09_PJ_DISC, i, a->team, cadv_v3_add(eye, cadv_v3_scale(f, 0.8f)), cadv_v3_add(cadv_v3_scale(f, P09_DISC_SPEED), cadv_v3_scale(a->k.vel, 0.5f)));
    } else {
        a->cd = 1.0f;
        launch(s, P09_PJ_GRENADE, i, a->team, cadv_v3_add(eye, cadv_v3_scale(f, 0.8f)), cadv_v3_add(cadv_v3_scale(f, 40.0f), cadv_v3_scale(a->k.vel, 0.5f)));
    }
    sfx(s, CADV_SFX_SHOT, 0.5f, a->k.pos);
}

static int slots(const p09_agent *a) { return cadv_armour_get(a->k.armour)->slots; }

static void agent_step(p09_state *s, int i, const cadv_input_frame *in)
{
    p09_agent *a = &s->a[i];
    cadv_skier_input si;
    int w;
    if (!a->alive) {
        return;
    }
    a->k.yaw = cadv_wrap_angle(a->k.yaw + in->look_x);
    a->k.pitch = cadv_clampf(a->k.pitch + in->look_y, -1.45f, 1.45f);
    si.move_x = in->move_x;
    si.move_y = in->move_y;
    si.ski = cadv_input_down(in, CADV_ACT_JUMP);
    si.jump = cadv_input_pressed(in, CADV_ACT_JUMP);
    si.jet = cadv_input_down(in, CADV_ACT_ALT_FIRE);
    cadv_skier_step(&a->k, &s->terrain, &s->world, &si, P09_DT);
    /* keep inside the globe */
    a->k.pos.x = cadv_clampf(a->k.pos.x, 4.0f, 508.0f);
    a->k.pos.z = cadv_clampf(a->k.pos.z, 4.0f, 508.0f);
    a->cd = cadv_maxf(0.0f, a->cd - P09_DT);
    a->invuln = cadv_maxf(0.0f, a->invuln - P09_DT);
    /* armour patches itself after a while out of the fight */
    a->calm += P09_DT;
    if (a->calm > 3.0f) {
        a->k.hp = cadv_minf(cadv_armour_get(a->k.armour)->hp, a->k.hp + 9.0f * P09_DT);
    }
    for (w = 0; w < P09_WEAPONS; ++w) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + w)) && w < slots(a)) {
            a->weapon = w;
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON)) {
        a->weapon = (a->weapon + 1) % slots(a);
    }
    if (cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        a->weapon = (a->weapon + slots(a) - 1) % slots(a);
    }
    if (a->weapon >= slots(a)) {
        a->weapon = P09_DISC;
    }
    if (s->phase == P09_PH_MATCH || (s->phase == P09_PH_TRAIN && i == 0)) {
        fire(s, i, cadv_input_down(in, CADV_ACT_FIRE));
    }
    if (carrying(s, i)) {
        a->carry_max = cadv_maxf(a->carry_max, cadv_skier_speed(&a->k));
        if (cadv_input_pressed(in, CADV_ACT_CROUCH)) {
            drop_flag(s, i, cadv_v3_scale(cadv_skier_forward(&a->k), 12.0f));
        }
    }
    /* the inventory station: change armour, refilled */
    if (cadv_input_pressed(in, CADV_ACT_USE) && s->phase == P09_PH_MATCH && flat(a->k.pos, s->base[a->team].station) < 3.5f) {
        if (!cadv_generator_powered(&s->base[a->team].power)) {
            if (i == 0) {
                say(s, "THE STATION IS DARK - THE GENERATOR IS DOWN");
            }
        } else {
            cadv_armour_class next = (cadv_armour_class)((a->k.armour + 1) % CADV_ARMOURS);
            cadv_skier_set_armour(&a->k, next);
            a->k.energy_pack = next != CADV_ARMOUR_HEAVY;
            a->k.hp = cadv_armour_get(next)->hp;
            a->k.energy = cadv_armour_get(next)->energy_max;
            if (a->weapon >= slots(a)) {
                a->weapon = P09_DISC;
            }
            if (i == 0) {
                char buf[64];
                snprintf(buf, sizeof buf, "%s ARMOUR%s", cadv_armour_get(next)->name, a->k.energy_pack ? " + ENERGY PACK" : "");
                say(s, buf);
            }
            sfx(s, CADV_SFX_SOCKET, 0.7f, a->k.pos);
        }
    }
}

/* ---- flags, turrets, the match --------------------------------------------------------- */

static void tick_flags(p09_state *s)
{
    int t;
    int i;
    for (t = 0; t < 2; ++t) {
        cadv_flag *f = &s->flag[t];
        if (s->drop_lock[t] > 0) {
            --s->drop_lock[t];
        }
        if (f->state == CADV_FLAG_CARRIED) {
            f->pos = cadv_p09_centre(&s->a[f->carrier]);
        } else if (f->state == CADV_FLAG_DROPPED) {
            float h = ground(s, f->pos.x, f->pos.z);
            f->vel.y -= CADV_SKIER_GRAVITY * P09_DT;
            f->pos = cadv_v3_add(f->pos, cadv_v3_scale(f->vel, P09_DT));
            if (f->pos.y <= h) {
                f->pos.y = h;
                f->vel = cadv_v3(0, 0, 0);
            }
            if (cadv_flag_tick(f, P09_DT)) {
                char buf[64];
                snprintf(buf, sizeof buf, "THE %s' FLAG WENT HOME", k_tribe[t]);
                feed(s, buf);
            }
        }
    }
    for (i = 0; i < P09_AGENTS; ++i) {
        p09_agent *a = &s->a[i];
        if (!a->alive) {
            continue;
        }
        for (t = 0; t < 2; ++t) {
            cadv_flag *f = &s->flag[t];
            float dy = a->k.pos.y + 1.0f - f->pos.y;
            if (f->state != CADV_FLAG_CARRIED && flat(a->k.pos, f->pos) < CADV_FLAG_TOUCH && dy > -2.0f && dy < 3.5f && (s->drop_lock[t] == 0 || s->drop_agent[t] != i)) {
                cadv_flag_event e = cadv_flag_touch(f, i, a->team);
                char buf[80];
                if (e == CADV_FLAG_TAKEN) {
                    a->carry_max = cadv_skier_speed(&a->k);
                    snprintf(buf, sizeof buf, "%s HAS THE %s' FLAG", s->names[i], k_tribe[t]);
                    feed(s, buf);
                    sfx(s, CADV_SFX_PICKUP, 0.8f, a->k.pos);
                } else if (e == CADV_FLAG_RETURNED) {
                    snprintf(buf, sizeof buf, "%s RETURNED THE FLAG", s->names[i]);
                    feed(s, buf);
                    sfx(s, CADV_SFX_BELL, 0.5f, a->k.pos);
                }
            }
        }
        if (carrying(s, i) && cadv_flag_try_capture(&s->flag[1 - a->team], &s->flag[a->team], a->k.pos) == CADV_FLAG_CAPTURED) {
            char buf[80];
            ++s->score[a->team];
            ++a->caps;
            if (i == 0 && a->carry_max > P09_FAST) {
                s->fast_cap = true;
            }
            snprintf(buf, sizeof buf, "%s CAPTURED THE FLAG! %d km/h", s->names[i], (int)(a->carry_max * 3.6f));
            feed(s, buf);
            snprintf(buf, sizeof buf, "%s SCORE!  %d - %d", k_tribe[a->team], s->score[0], s->score[1]);
            say(s, buf);
            sfx(s, CADV_SFX_STAR, 0.7f, a->k.pos);
            if (a->team == 0 && s->score[0] < P09_TO_WIN && !s->sudden) {
                s->checkpoint = 1 + s->score[0]; /* your captures are kept */
            }
        }
    }
}

static void tick_turrets(p09_state *s)
{
    int b;
    for (b = 0; b < 2; ++b) {
        p09_base *pb = &s->base[b];
        int i;
        if (s->ticks % 15 == b) {
            float best = pb->turret.range;
            pb->turret_target = -1;
            for (i = 0; i < P09_AGENTS; ++i) {
                const p09_agent *a = &s->a[i];
                float d = cadv_v3_dist(cadv_p09_centre(a), pb->turret.pos);
                if (a->alive && a->team != b && d < best && sees(s, pb->turret.pos, cadv_p09_centre(a))) {
                    best = d;
                    pb->turret_target = i;
                }
            }
        }
        if (pb->turret_target >= 0 && s->a[pb->turret_target].alive) {
            const p09_agent *a = &s->a[pb->turret_target];
            cadv_vec3 c = cadv_p09_centre(a);
            if (cadv_turret_ready(&pb->turret, &pb->power, cadv_v3_dist(c, pb->turret.pos), P09_DT)) {
                /* lead the target, with an error like a bot of that side */
                float err = (b == 0 ? 2.2f : cadv_p09_skill(s, P09_AGENTS - 1)) * 1.3f * (1.0f + cadv_skier_speed(&a->k) / 30.0f);
                float t = cadv_v3_dist(c, pb->turret.pos) / P09_DISC_SPEED;
                cadv_vec3 aim = cadv_v3_add(c, cadv_v3_scale(a->k.vel, t));
                cadv_vec3 dir;
                aim = cadv_v3_add(aim, cadv_v3(rnd_pm(s) * err * t, rnd_pm(s) * err * t * 0.5f, rnd_pm(s) * err * t));
                dir = cadv_v3_norm(cadv_v3_sub(aim, pb->turret.pos));
                launch(s, P09_PJ_TURRET, -2 - b, b, cadv_v3_add(pb->turret.pos, cadv_v3_scale(dir, 1.5f)), cadv_v3_scale(dir, P09_DISC_SPEED));
                sfx(s, CADV_SFX_SHOT, 0.4f, pb->turret.pos);
            }
        } else {
            pb->turret.reload = cadv_maxf(0.0f, pb->turret.reload - P09_DT);
        }
        if (cadv_generator_tick(&pb->power, P09_DT)) {
            char buf[80];
            snprintf(buf, sizeof buf, "THE %s' GENERATOR IS BACK ONLINE", k_tribe[b]);
            feed(s, buf);
        }
    }
}

static void start_match(p09_state *s)
{
    int i;
    s->phase = P09_PH_COUNT;
    s->timer = P09_COUNT;
    for (i = 0; i < P09_PROJ; ++i) {
        s->proj[i].active = false;
    }
    for (i = 0; i < P09_AGENTS; ++i) {
        spawn(s, i);
    }
    cadv_flag_return(&s->flag[0]);
    cadv_flag_return(&s->flag[1]);
    if (s->checkpoint < 1) {
        s->checkpoint = 1;
    }
}

static void begin_lap(p09_state *s)
{
    int i;
    s->phase = P09_PH_LAP;
    s->won = true;
    s->star_pos = cadv_v3(BASE_X, ground(s, BASE_X, 256.0f) + 1.4f, 256.0f);
    s->checkpoint = P09_LAP_CP + s->score[1];
    cadv_flag_return(&s->flag[0]);
    cadv_flag_return(&s->flag[1]);
    for (i = 0; i < P09_PROJ; ++i) {
        s->proj[i].active = false;
    }
    if (!s->a[0].alive) {
        spawn(s, 0);
    }
    say(s, "VICTORY! THE TROPHY STAR WAITS AT CENTRE FIELD - CARRY IT HOME");
    sfx(s, CADV_SFX_BELL, 1.0f, s->a[0].k.pos);
}

static void lose(p09_state *s, const char *why)
{
    say(s, why);
    s->end_timer = 240;
    s->won = false;
}

/* ---- friends' marks -------------------------------------------------------------------- */

static void check_marks(p09_state *s)
{
    int i;
    const p09_agent *me = &s->a[0];
    if (!me->alive || s->phase != P09_PH_MATCH || s->ticks % 10 != 0) {
        return;
    }
    for (i = 1; i < P09_AGENTS; ++i) {
        p09_agent *a = &s->a[i];
        int m = a->name == 2 ? 1 : a->name == 3 ? 2 : -1;
        if (m < 0 || (s->marks_found & (1u << m)) || !a->alive) {
            continue;
        }
        if (a->team == 0 ? flat(a->k.pos, me->k.pos) < 25.0f : (flat(a->k.pos, me->k.pos) < 60.0f && sees(s, cadv_skier_eye(&me->k), cadv_p09_centre(a)))) {
            char buf[96];
            s->marks_found |= 1u << m;
            snprintf(buf, sizeof buf, "%s!", s->mark_text[m]);
            feed(s, buf);
        }
    }
    if (!(s->marks_found & 8u) && cadv_p09_in_halfpipe(me->k.pos)) {
        char buf[96];
        s->marks_found |= 8u;
        snprintf(buf, sizeof buf, "THE %s", s->mark_text[3]);
        say(s, buf);
    }
}

/* ---- bots ------------------------------------------------------------------------------- */

/* Drives towards a goal: steers the velocity, skis when fast or downhill,
 * jets over rises with some energy kept in hand. */
static void steer(p09_state *s, int i, cadv_vec3 goal, cadv_input_frame *out, float yaw_after, bool arrive)
{
    p09_agent *a = &s->a[i];
    cadv_vec3 p = a->k.pos;
    cadv_vec3 hv = cadv_v3(a->k.vel.x, 0, a->k.vel.z);
    float sp = cadv_v3_len(hv);
    float dist = flat(p, goal);
    cadv_vec3 dir = dist > 0.01f ? cadv_v3((goal.x - p.x) / dist, 0, (goal.z - p.z) / dist) : cadv_v3(0, 0, 0);
    cadv_vec3 wish = dir;
    float h0 = ground(s, p.x, p.z);
    float rise;
    if (sp > 4.0f && dist > 6.0f) {
        /* aim the velocity, not the body: cancel the sideways drift */
        cadv_vec3 err = cadv_v3_sub(cadv_v3_scale(dir, cadv_maxf(sp, 30.0f)), hv);
        wish = cadv_v3_norm(err);
    }
    {
        float fx = -cadv_sinf(yaw_after);
        float fz = -cadv_cosf(yaw_after);
        out->move_y = wish.x * fx + wish.z * fz;
        out->move_x = wish.x * -fz + wish.z * fx;
    }
    {
        cadv_vec3 ahead = cadv_v3_add(p, cadv_v3_scale(dir, cadv_minf(dist, 26.0f)));
        rise = ground(s, ahead.x, ahead.z) - cadv_maxf(h0, a->k.on_ground ? h0 : p.y - 2.0f);
    }
    if (a->k.energy < 8.0f) {
        a->jet_ok = false;
    } else if (a->k.energy > cadv_armour_get(a->k.armour)->energy_max * 0.5f) {
        a->jet_ok = true;
    }
    if (sp > 7.0f || rise < -1.0f) {
        out->held |= CADV_ACT_BIT(CADV_ACT_JUMP); /* ski */
    }
    if (arrive && dist < 90.0f) {
        /* the arrival: where will we be when we get there? Jet when coming in
         * too low, brake (no skis) when the slope would fling us over it */
        float t = dist / cadv_maxf(sp, 6.0f);
        float y_then = cadv_maxf(p.y + a->k.vel.y * t - 0.5f * CADV_SKIER_GRAVITY * t * t, ground(s, goal.x, goal.z));
        if (a->jet_ok && y_then < goal.y - 1.5f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
        }
        if (y_then > goal.y + 1.5f && dist < 70.0f) {
            cadv_vec3 b = cadv_v3_norm(cadv_v3_sub(cadv_v3_scale(dir, 10.0f), hv));
            float fx = -cadv_sinf(yaw_after);
            float fz = -cadv_cosf(yaw_after);
            out->move_y = b.x * fx + b.z * fz;
            out->move_x = b.x * -fz + b.z * fx;
            out->held &= ~CADV_ACT_BIT(CADV_ACT_JUMP);
        }
    } else if (a->jet_ok && ((rise > 2.0f && (sp < 45.0f || rise > 6.0f)) || (!a->k.on_ground && p.y - h0 < 1.5f && rise > 0.5f))) {
        out->held |= CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
    }
    /* stuck against something: hop and jet */
    if (sp < 1.0f && dist > 4.0f) {
        if (++a->stuck > 90) {
            out->pressed |= CADV_ACT_BIT(CADV_ACT_JUMP);
            out->held |= CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
            if (a->stuck > 150) {
                a->stuck = 0;
            }
        }
    } else {
        a->stuck = 0;
    }
}

/* the next point on the agent's ski line towards the given base */
static cadv_vec3 route_goal(p09_state *s, int i, int to_base)
{
    p09_agent *a = &s->a[i];
    const cadv_vec3 *r = s->route[a->route];
    int step = to_base == 1 ? 1 : -1;
    int end = to_base == 1 ? P09_ROUTE - 1 : 0;
    int k;
    /* pick up the line at the nearest point ahead */
    if (a->route_pos < 0 || a->route_pos >= P09_ROUTE) {
        a->route_pos = 0;
    }
    {
        int best = a->route_pos;
        float bd = 1e9f;
        for (k = 0; k < P09_ROUTE; ++k) {
            float d = flat(r[k], a->k.pos);
            if (d < bd) {
                bd = d;
                best = k;
            }
        }
        a->route_pos = best;
    }
    k = a->route_pos;
    while (k != end && flat(r[k], a->k.pos) < 30.0f) {
        k += step;
    }
    return r[k];
}

static float aim_skill_err(p09_state *s, int i, float dist, float tspeed)
{
    return cadv_p09_skill(s, i) * (dist / 50.0f) * (1.0f + tspeed / 35.0f);
}

/* Picks a target, leads it, turns and shoots. Returns true when aiming. */
static bool bot_fight(p09_state *s, int i, cadv_input_frame *out, float range)
{
    p09_agent *a = &s->a[i];
    cadv_vec3 eye = cadv_skier_eye(&a->k);
    const p09_agent *t;
    cadv_vec3 tc;
    float dist;
    float want_yaw;
    float want_pitch;
    float dy;
    float dp;
    if ((s->ticks + i) % 10 == 0) {
        int k;
        float best = range;
        a->target = -1;
        for (k = 0; k < P09_AGENTS; ++k) {
            const p09_agent *o = &s->a[k];
            float d = cadv_v3_dist(cadv_p09_centre(o), eye);
            /* the enemy flag carrier is worth a longer look */
            if (carrying(s, k)) {
                d *= 0.6f;
            }
            if (o->alive && o->team != a->team && d < best && sees(s, eye, cadv_p09_centre(o))) {
                best = d;
                a->target = k;
            }
        }
    }
    if (a->target < 0 || !s->a[a->target].alive) {
        a->target = -1;
        a->react = 0.0f;
        return false;
    }
    t = &s->a[a->target];
    tc = cadv_p09_centre(t);
    dist = cadv_v3_dist(tc, eye);
    a->react += P09_DT;
    a->weapon = dist < 22.0f ? P09_CHAIN : P09_DISC;
    if (a->weapon == P09_DISC) {
        /* lead it: the disc keeps half the shooter's speed */
        cadv_vec3 rel = cadv_v3_sub(t->k.vel, cadv_v3_scale(a->k.vel, 0.5f));
        float tt = dist / P09_DISC_SPEED;
        int it;
        cadv_vec3 aim = tc;
        for (it = 0; it < 3; ++it) {
            aim = cadv_v3_add(tc, cadv_v3_scale(rel, tt));
            tt = cadv_v3_dist(aim, eye) / P09_DISC_SPEED;
        }
        if (t->k.on_ground) {
            aim.y = ground(s, aim.x, aim.z) + 0.6f; /* at the feet for the splash */
        }
        tc = cadv_v3_add(aim, cadv_v3(a->aim_err_x, a->aim_err_y * 0.5f, -a->aim_err_x * 0.5f));
    } else {
        /* tracking a fast skier with a chaingun is hard: the aim wanders */
        if ((s->ticks + i * 7) % 18 == 0) {
            /* a fast skier crosses the view quickly: the wander grows with its speed */
            float e = cadv_p09_skill(s, i) * (0.1f + 0.06f * cadv_skier_speed(&t->k));
            a->aim_err_x = rnd_pm(s) * e;
            a->aim_err_y = rnd_pm(s) * e;
        }
        tc = cadv_v3_add(tc, cadv_v3(a->aim_err_x, a->aim_err_y, -a->aim_err_x * 0.5f));
    }
    want_yaw = yaw_to(eye, tc);
    want_pitch = cadv_atan2f(tc.y - eye.y, flat(eye, tc));
    dy = cadv_wrap_angle(want_yaw - a->k.yaw);
    dp = want_pitch - a->k.pitch;
    out->look_x = cadv_clampf(dy, -0.12f, 0.12f);
    out->look_y = cadv_clampf(dp, -0.12f, 0.12f);
    if (a->react > (i == 0 ? 0.15f : 0.45f - 0.08f * (float)s->cfg.difficulty) && cadv_absf(dy) < 0.04f && cadv_absf(dp) < 0.04f && (a->weapon == P09_CHAIN || a->cd <= 0.0f)) {
        out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
        if (a->weapon == P09_DISC) {
            float e = aim_skill_err(s, i, dist, cadv_skier_speed(&t->k));
            a->aim_err_x = rnd_pm(s) * e;
            a->aim_err_y = rnd_pm(s) * e;
        }
    }
    if (a->weapon == P09_CHAIN) {
        out->held |= CADV_ACT_BIT(CADV_ACT_FIRE); /* keep it spinning */
    }
    return true;
}

static void look_along(p09_state *s, int i, cadv_vec3 goal, cadv_input_frame *out)
{
    p09_agent *a = &s->a[i];
    float want = yaw_to(a->k.pos, goal);
    out->look_x = cadv_clampf(cadv_wrap_angle(want - a->k.yaw), -0.1f, 0.1f);
    out->look_y = cadv_clampf(-0.05f - a->k.pitch, -0.05f, 0.05f);
}

static void bot_brain(p09_state *s, int i, cadv_input_frame *out)
{
    p09_agent *a = &s->a[i];
    int own = a->team;
    int foe = 1 - own;
    const cadv_flag *mine = &s->flag[own];
    const cadv_flag *theirs = &s->flag[foe];
    cadv_vec3 goal;
    bool aiming;
    float range = 85.0f;
    int role = a->role;
    bool arrive = true;
    if (role == P09_ROLE_HOFF && !cadv_generator_powered(&s->base[foe].power)) {
        role = P09_ROLE_CAP;
    }
    if (carrying(s, i)) {
        arrive = flat(a->k.pos, s->base[own].stand) < 70.0f;
        goal = arrive ? s->base[own].stand : route_goal(s, i, own);
        range = 40.0f;
    } else if (mine->state == CADV_FLAG_DROPPED && (role != P09_ROLE_CAP || flat(mine->pos, a->k.pos) < 80.0f)) {
        goal = mine->pos;
    } else if (mine->state == CADV_FLAG_CARRIED && (role != P09_ROLE_CAP || (i == 0 && flat(s->a[mine->carrier].k.pos, a->k.pos) < 150.0f))) {
        /* hunt the carrier (the player's bot does it too when close) */
        const p09_agent *c = &s->a[mine->carrier];
        goal = cadv_v3_add(c->k.pos, cadv_v3_scale(c->k.vel, 1.2f));
    } else if (role == P09_ROLE_CAP || role == P09_ROLE_HOFF) {
        if (theirs->state == CADV_FLAG_DROPPED) {
            goal = theirs->pos;
        } else if (theirs->state == CADV_FLAG_CARRIED) {
            /* escort the carrier home */
            goal = s->a[theirs->carrier].k.pos;
        } else if (role == P09_ROLE_HOFF) {
            const p09_base *b = &s->base[foe];
            cadv_vec3 stand_off = cadv_v3_add(b->gen, cadv_v3(0, 0, (foe == 0 ? -1.0f : 1.0f) * 20.0f));
            goal = flat(a->k.pos, b->gen) < 120.0f ? stand_off : route_goal(s, i, foe);
            arrive = false;
            if (flat(a->k.pos, b->gen) < 45.0f) {
                /* lob grenades at the generator */
                cadv_vec3 eye = cadv_skier_eye(&a->k);
                float d = flat(eye, b->gen);
                float pitch = 0.5f * cadv_atan2f(CADV_SKIER_GRAVITY * d, 40.0f * 40.0f) + cadv_atan2f(b->gen.y - eye.y, d) * 0.5f;
                float dyaw = cadv_wrap_angle(yaw_to(eye, b->gen) - a->k.yaw);
                a->weapon = P09_GRENADE;
                out->look_x = cadv_clampf(dyaw, -0.12f, 0.12f);
                out->look_y = cadv_clampf(pitch - a->k.pitch, -0.1f, 0.1f);
                if (cadv_absf(dyaw) < 0.05f && cadv_absf(pitch - a->k.pitch) < 0.05f) {
                    out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
                }
                if (bot_fight(s, i, out, 30.0f)) {
                    steer(s, i, goal, out, a->k.yaw + out->look_x, arrive);
                    return;
                }
                a->weapon = P09_GRENADE;
                steer(s, i, goal, out, a->k.yaw + out->look_x, arrive);
                return;
            }
        } else {
            /* someone guarding it? duel them from range first */
            int k;
            bool guarded = false;
            for (k = 0; k < P09_AGENTS; ++k) {
                if (s->a[k].alive && s->a[k].team == foe && flat(s->a[k].k.pos, theirs->home) < 25.0f) {
                    guarded = true;
                }
            }
            a->siege = flat(a->k.pos, theirs->home) < 95.0f ? a->siege + P09_DT : 0.0f;
            if (a->k.hp < cadv_armour_get(a->k.armour)->hp * 0.4f && theirs->state == CADV_FLAG_HOME) {
                a->retreat = true;
            } else if (a->k.hp > cadv_armour_get(a->k.armour)->hp * 0.9f) {
                a->retreat = false;
            }
            if (a->retreat) {
                goal = s->route[a->route][own == 0 ? 5 : P09_ROUTE - 6];
                arrive = false;
            } else if (guarded && a->siege > 0.0f && a->siege < 20.0f) {
                /* circle at duelling range */
                float ang = (float)s->ticks * 0.012f + (float)i;
                goal = cadv_v3_add(theirs->home, cadv_v3(-62.0f, 0, (foe == 1 ? -1.0f : 1.0f) * (15.0f + 15.0f * cadv_sinf(ang))));
                arrive = false;
            } else {
                arrive = flat(a->k.pos, theirs->pos) < 70.0f;
                goal = arrive ? theirs->pos : route_goal(s, i, foe);
            }
        }
    } else if (role == P09_ROLE_D) {
        float side = i % 2 == 0 ? 0.0f : (i == 3 ? -1.0f : 1.0f);
        goal = cadv_v3_add(s->base[own].stand, cadv_v3(side * 14.0f + 6.0f * cadv_sinf((float)s->ticks * 0.005f), 0, (own == 0 ? 1.0f : -1.0f) * (9.0f - cadv_absf(side) * 3.0f)));
        range = 75.0f;
        arrive = false;
    } else {
        arrive = false;
        /* midfield: at the foot of our hill, on the middle line */
        goal = s->route[0][own == 0 ? 4 : P09_ROUTE - 5];
        range = 65.0f;
    }
    aiming = bot_fight(s, i, out, range);
    if (!aiming) {
        look_along(s, i, goal, out);
    }
    /* holding positions: stop skiing, mill about */
    if ((role == P09_ROLE_D || role == P09_ROLE_MID) && mine->state == CADV_FLAG_HOME && flat(a->k.pos, goal) < 8.0f) {
        float st = cadv_sinf((float)(s->ticks + i * 40) * 0.03f);
        out->move_x = st * 0.6f;
        out->move_y = 0.0f;
        if (aiming && (s->ticks + i * 13) % 50 < 6) {
            out->pressed |= CADV_ACT_BIT(CADV_ACT_JUMP);
        }
        return;
    }
    steer(s, i, goal, out, a->k.yaw + out->look_x, arrive);
}

/* ---- training ------------------------------------------------------------------------ */

static void train_tick(p09_state *s, const cadv_input_frame *in)
{
    p09_agent *a = &s->a[0];
    ++s->train_t;
    agent_step(s, 0, in);
    tick_proj(s);
    s->drone_a += P09_DT * 0.9f;
    s->drone = cadv_v3(s->drone_c.x + 10.0f * cadv_cosf(s->drone_a), s->drone_c.y, s->drone_c.z + 10.0f * cadv_sinf(s->drone_a));
    if (s->train == P09_TR_SKI && cadv_skier_speed(&a->k) > 27.8f && a->k.skiing) {
        cadv_vec3 f = cadv_v3(-cadv_sinf(a->k.yaw), 0, -cadv_cosf(a->k.yaw));
        cadv_vec3 b = cadv_v3_add(a->k.pos, cadv_v3_scale(f, 50.0f));
        s->train = P09_TR_JET;
        s->beacon = cadv_v3(b.x, ground(s, b.x, b.z) + 22.0f, b.z);
        say(s, "100 KM/H! NOW HOLD RIGHT MOUSE TO JET UP TO THE BEACON");
        sfx(s, CADV_SFX_BELL, 0.6f, a->k.pos);
    } else if (s->train == P09_TR_JET && cadv_v3_dist(cadv_p09_centre(a), s->beacon) < 7.0f) {
        cadv_vec3 f = cadv_v3(-cadv_sinf(a->k.yaw), 0, -cadv_cosf(a->k.yaw));
        cadv_vec3 c = cadv_v3_add(a->k.pos, cadv_v3_scale(f, 45.0f));
        s->train = P09_TR_DISC;
        s->drone_c = cadv_v3(c.x, ground(s, c.x, c.z) + 6.0f, c.z);
        say(s, "THE DISC IS SLOW: LEAD THE TARGET DRONE AND HIT IT");
        sfx(s, CADV_SFX_BELL, 0.6f, a->k.pos);
    }
    if (s->train == P09_TR_DONE || s->train_t > P09_TRAIN_MAX || (cadv_input_pressed(in, CADV_ACT_USE) && s->train_t > 30)) {
        s->train = P09_TR_DONE;
        start_match(s);
        say(s, "SNOWGLOBE BASIN - FIRST TO FIVE CAPTURES");
    }
}

/* ---- tick ---------------------------------------------------------------------------- */

void cadv_p09_tick(void *state, const cadv_input_frame *in)
{
    p09_state *s = (p09_state *)state;
    int i;
    if (s->outcome != CADV_OUTCOME_RUNNING) {
        return;
    }
    ++s->ticks;
    s->map_held = cadv_input_down(in, CADV_ACT_MAP);
    if (s->message_timer > 0) {
        --s->message_timer;
    }
    for (i = 0; i < P09_FEED; ++i) {
        if (s->feed_t[i] > 0) {
            --s->feed_t[i];
        }
    }
    if (s->hurt_flash > 0) {
        --s->hurt_flash;
    }
    if (s->hit_mark > 0) {
        --s->hit_mark;
    }
    if (s->boom_t > 0) {
        --s->boom_t;
    }
    if (s->end_timer > 0) {
        if (--s->end_timer == 0) {
            s->outcome = s->won ? CADV_OUTCOME_WON : CADV_OUTCOME_LOST;
        }
        agent_step(s, 0, in);
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_PAUSE)) {
        s->outcome = CADV_OUTCOME_QUIT;
        return;
    }
    if (s->phase == P09_PH_TRAIN) {
        train_tick(s, in);
        return;
    }
    if (s->phase == P09_PH_COUNT) {
        if (--s->timer <= 0) {
            s->phase = P09_PH_MATCH;
            s->timer = P09_MATCH;
            say(s, "GO! SKI, JET, CAPTURE");
            sfx(s, CADV_SFX_BELL, 1.0f, s->a[0].k.pos);
        }
        agent_step(s, 0, in); /* look around while waiting */
        s->a[0].k.pos = s->base[0].spawn[0];
        s->a[0].k.vel = cadv_v3(0, 0, 0);
        return;
    }
    if (s->phase == P09_PH_LAP) {
        p09_agent *a = &s->a[0];
        agent_step(s, 0, in);
        if (!s->star_carried && flat(a->k.pos, s->star_pos) < 2.5f && cadv_absf(a->k.pos.y + 1.0f - s->star_pos.y) < 3.0f) {
            s->star_carried = true;
            say(s, "THE TROPHY STAR! CARRY IT HOME TO YOUR FLAG STAND");
            sfx(s, CADV_SFX_PICKUP, 1.0f, a->k.pos);
        }
        if (s->star_carried) {
            s->star_pos = cadv_v3_add(cadv_p09_centre(a), cadv_v3(0, 1.4f, 0));
            if (flat(a->k.pos, s->base[0].stand) < 3.0f && cadv_absf(a->k.pos.y - s->base[0].stand.y) < 3.0f) {
                s->star_taken = true;
                s->end_timer = 240;
                say(s, "THE SHINING STAR OF THE NORTHERN REACH!");
                sfx(s, CADV_SFX_STAR, 1.0f, a->k.pos);
            }
        }
        return;
    }
    /* the match */
    for (i = 0; i < P09_AGENTS; ++i) {
        p09_agent *a = &s->a[i];
        cadv_input_frame bot;
        if (!a->alive) {
            if (--a->respawn <= 0) {
                spawn(s, i);
            }
            continue;
        }
        if (i == 0) {
            agent_step(s, 0, in);
            continue;
        }
        memset(&bot, 0, sizeof bot);
        if (!s->bench) {
            bot_brain(s, i, &bot);
        }
        agent_step(s, i, &bot);
    }
    tick_proj(s);
    tick_turrets(s);
    tick_flags(s);
    check_marks(s);
    if (s->score[0] >= P09_TO_WIN || (s->sudden && s->score[0] > s->score[1])) {
        begin_lap(s);
        return;
    }
    if (s->score[1] >= P09_TO_WIN || (s->sudden && s->score[1] > s->score[0])) {
        lose(s, "THE EMBER OWLS TAKE THE FESTIVAL GAMES");
        return;
    }
    if (--s->timer <= 0 && !s->sudden) {
        if (s->score[0] > s->score[1]) {
            begin_lap(s);
        } else if (s->score[0] < s->score[1]) {
            lose(s, "TIME! THE EMBER OWLS LEAD");
        } else {
            s->sudden = true;
            say(s, "TIME! SUDDEN DEATH - THE NEXT CAPTURE WINS");
        }
    }
}

/* ---- init ---------------------------------------------------------------------------- */

bool cadv_p09_init(void *state, const cadv_portal_cfg *cfg)
{
    p09_state *s = (p09_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    static const int roster[P09_AGENTS] = {-1, 2, -2, -3, 3, 4, -4, -5};
    static const int roles[P09_TEAM] = {P09_ROLE_CAP, P09_ROLE_HOFF, P09_ROLE_D, P09_ROLE_MID};
    static const char *const filler[5] = {"YOU", "TUNDRA", "FLINT", "SLEET", "HOARFROST"};
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    cadv_rng_seed(&s->rng, cfg->seed, 9u);
    cadv_friends_default(&fr);
    if (cfg->friends) {
        for (i = 0; i < cfg->friend_count && i < CADV_MAX_FRIENDS; ++i) {
            snprintf(fr.name[i], CADV_FRIEND_NAME, "%s", cfg->friends[i]);
        }
        fr.count = cfg->friend_count < CADV_MAX_FRIENDS ? cfg->friend_count : CADV_MAX_FRIENDS;
    }
    for (i = 0; d && i < d->mark_count && i < 4; ++i) {
        cadv_friends_mark_text(&fr, &d->marks[i], s->mark_text[i], sizeof s->mark_text[i]);
    }
    build_map(s);
    build_routes(s);
    for (i = 0; i < P09_AGENTS; ++i) {
        p09_agent *a = &s->a[i];
        int r = roster[i];
        a->team = i < P09_TEAM ? 0 : 1;
        a->role = roles[i % P09_TEAM];
        if (i == 3) {
            a->role = P09_ROLE_D; /* the Wolves play two at home */
        }
        if (r > 0) {
            char up[CADV_FRIEND_NAME];
            int k;
            const char *n = fr.name[r < fr.count ? r : 0];
            for (k = 0; n[k] && k + 1 < (int)sizeof up; ++k) {
                up[k] = (char)(n[k] >= 'a' && n[k] <= 'z' ? n[k] - 32 : n[k]);
            }
            up[k] = '\0';
            copy_text(s->names[i], sizeof s->names[i], up);
            a->name = r;
        } else {
            copy_text(s->names[i], sizeof s->names[i], filler[-r - 1]);
            a->name = 10 - r;
        }
        spawn(s, i);
    }
    s->checkpoint = cfg->checkpoint;
    if (cfg->checkpoint >= P09_LAP_CP) {
        s->score[0] = P09_TO_WIN;
        s->score[1] = cadv_mini(cfg->checkpoint - P09_LAP_CP, P09_TO_WIN - 1);
        s->train = P09_TR_DONE;
        begin_lap(s);
        s->a[0].k.pos = s->base[0].spawn[0];
    } else if (cfg->checkpoint > 0) {
        /* your captures so far; the Owls start again from nothing */
        s->score[0] = cadv_mini(cfg->checkpoint - 1, P09_TO_WIN - 1);
        s->train = P09_TR_DONE;
        start_match(s);
    } else {
        s->phase = P09_PH_TRAIN;
        s->train = P09_TR_SKI;
        say(s, "TRAINING: HOLD JUMP TO SKI DOWN THE HILL - REACH 100 KM/H");
    }
    return true;
}

/* ---- results & scripting -------------------------------------------------------------- */

cadv_portal_result cadv_p09_result(const void *state)
{
    const p09_state *s = (const p09_state *)state;
    cadv_portal_result res;
    memset(&res, 0, sizeof res);
    res.outcome = s->outcome;
    res.star_collected = s->star_taken;
    res.checkpoint = s->checkpoint;
    res.marks_found = s->marks_found;
    res.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        res.snowflakes = 1 + (s->score[1] == 0 ? 1 : 0) + (s->fast_cap ? 1 : 0);
    }
    return res;
}

bool cadv_p09_query(const void *state, const char *key, double *out)
{
    const p09_state *s = (const p09_state *)state;
    const p09_agent *p = &s->a[0];
    cadv_vec3 e = cadv_skier_eye(&p->k);
    struct kv {
        const char *k;
        double v;
    };
    const struct kv table[] = {
        {"player.x", e.x},
        {"player.y", e.y},
        {"player.z", e.z},
        {"player.yaw", p->k.yaw},
        {"player.health", p->alive ? p->k.hp : 0},
        {"energy", p->k.energy},
        {"speed", cadv_skier_speed(&p->k) * 3.6f},
        {"armour", p->k.armour},
        {"weapon", p->weapon},
        {"phase", s->phase},
        {"train", s->train},
        {"score.us", s->score[0]},
        {"score.them", s->score[1]},
        {"flag.ours", s->flag[0].state},
        {"flag.theirs", s->flag[1].state},
        {"carrying", carrying(s, 0)},
        {"caps", p->caps},
        {"kills", p->kills},
        {"gen.ours", cadv_generator_powered(&s->base[0].power)},
        {"gen.theirs", cadv_generator_powered(&s->base[1].power)},
        {"sudden", s->sudden},
        {"time", s->phase == P09_PH_MATCH ? s->timer / 60 : 0},
        {"fast", s->fast_cap},
        {"star.carried", s->star_carried},
        {"star.taken", s->star_taken},
        {"outcome", (double)s->outcome},
        {"marks", s->marks_found},
        {"ticks", s->ticks},
    };
    size_t i;
    for (i = 0; i < sizeof table / sizeof table[0]; ++i) {
        if (strcmp(key, table[i].k) == 0) {
            *out = table[i].v;
            return true;
        }
    }
    return false;
}

void cadv_p09_teleport(void *state, float x, float y, float z, float yaw)
{
    p09_state *s = (p09_state *)state;
    s->a[0].k.pos = cadv_v3(x, cadv_maxf(y - 1.7f, ground(s, x, z)), z);
    s->a[0].k.vel = cadv_v3(0, 0, 0);
    s->a[0].k.yaw = yaw;
    s->a[0].k.pitch = 0.0f;
}

/* ---- completion bot -------------------------------------------------------------------- */

void cadv_p09_bot(const void *state, cadv_input_frame *out)
{
    p09_state *s = (p09_state *)(uintptr_t)state; /* the brain keeps its notes in the agent */
    p09_agent *a = &s->a[0];
    memset(out, 0, sizeof *out);
    if (s->end_timer > 0 || !a->alive || s->phase == P09_PH_COUNT) {
        return;
    }
    if (s->phase == P09_PH_TRAIN) {
        if (s->train == P09_TR_SKI) {
            cadv_vec3 goal = s->route[0][5];
            look_along(s, 0, goal, out);
            steer(s, 0, goal, out, a->k.yaw + out->look_x, false);
            out->held |= CADV_ACT_BIT(CADV_ACT_JUMP);
            out->held &= ~CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
        } else if (s->train == P09_TR_JET) {
            /* skate under it, then rise straight up */
            float d = flat(a->k.pos, s->beacon);
            look_along(s, 0, s->beacon, out);
            steer(s, 0, s->beacon, out, a->k.yaw + out->look_x, false);
            out->held &= ~CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
            if (d < 12.0f && a->jet_ok && a->k.pos.y + 1.0f < s->beacon.y) {
                out->held |= CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
            }
            if (d < 12.0f && !a->k.on_ground) {
                out->held &= ~CADV_ACT_BIT(CADV_ACT_JUMP);
            }
        } else {
            /* stand still and lead the drone */
            cadv_vec3 eye = cadv_skier_eye(&a->k);
            float tt = cadv_v3_dist(eye, s->drone) / P09_DISC_SPEED;
            int it;
            cadv_vec3 aim = s->drone;
            float dy;
            float dp;
            for (it = 0; it < 3; ++it) {
                float ang = s->drone_a + 0.9f * tt;
                cadv_vec3 rel = cadv_v3_scale(a->k.vel, 0.5f);
                aim = cadv_v3(s->drone_c.x + 10.0f * cadv_cosf(ang) - rel.x * tt, s->drone_c.y - rel.y * tt, s->drone_c.z + 10.0f * cadv_sinf(ang) - rel.z * tt);
                tt = cadv_v3_dist(eye, aim) / P09_DISC_SPEED;
            }
            dy = cadv_wrap_angle(yaw_to(eye, aim) - a->k.yaw);
            dp = cadv_atan2f(aim.y - eye.y, flat(eye, aim)) - a->k.pitch;
            out->look_x = cadv_clampf(dy, -0.12f, 0.12f);
            out->look_y = cadv_clampf(dp, -0.12f, 0.12f);
            if (cadv_absf(dy) < 0.01f && cadv_absf(dp) < 0.01f && a->cd <= 0.0f && cadv_skier_speed(&a->k) < 3.0f) {
                out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
            }
        }
        return;
    }
    if (s->phase == P09_PH_LAP) {
        bool arrive = !s->star_carried || flat(a->k.pos, s->base[0].stand) < 70.0f;
        cadv_vec3 goal = s->star_carried ? (arrive ? s->base[0].stand : route_goal(s, 0, 0)) : s->star_pos;
        look_along(s, 0, goal, out);
        steer(s, 0, goal, out, a->k.yaw + out->look_x, arrive);
        return;
    }
    bot_brain(s, 0, out);
}
