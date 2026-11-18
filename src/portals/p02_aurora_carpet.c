/* SPDX-License-Identifier: MIT */
#include "portals/p02_aurora_carpet.h"

#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "core/mathx.h"
#include "gfx/font.h"
#include "hub/friends.h"
#include "portals/p02_internal.h"

/* ---- world layout (fixed so every player flies the same fjords) ---------------- */

#define LAYOUT_SEED 2026u

static const cadv_island k_islands[] = {
    /*  x      z      r     h     flat */
    {62.f, 190.f, 40.f, 14.f, 0.55f},  /* 0 home isle (first castle plateau) */
    {196.f, 62.f, 38.f, 18.f, 0.45f},  /* 1 Vael's isle */
    {200.f, 192.f, 36.f, 12.f, 0.30f}, /* 2 east isle: mana nodes */
    {58.f, 60.f, 34.f, 20.f, 0.25f},   /* 3 west isle: frozen crater */
    {128.f, 126.f, 32.f, 10.f, 0.72f}, /* 4 middle isle: giant runes */
};
#define ISLANDS ((int)(sizeof k_islands / sizeof k_islands[0]))

static const int k_level_mana[P02_MAX_CASTLE_LEVEL] = {0, 150, 350, 600};
static const int k_spell_cost[P02_SPELLS] = {0, 0, 5, 5, 0, 120};
static const char *const k_spell_name[P02_SPELLS] = {"FIREBALL", "CLAIM", "RAISE LAND", "LOWER LAND", "CASTLE", "VOLCANO"};

const char *cadv_p02_spell_name(int spell) { return k_spell_name[spell]; }

int cadv_p02_castle_level_for(int mana)
{
    int lvl = 1;
    int i;
    for (i = 1; i < P02_MAX_CASTLE_LEVEL; ++i) {
        if (mana >= k_level_mana[i]) {
            lvl = i + 1;
        }
    }
    return lvl;
}

int cadv_p02_spell_cost(int spell) { return k_spell_cost[spell]; }

static void say(p02_state *s, const char *text)
{
    snprintf(s->message, sizeof s->message, "%s", text);
    s->message_timer = 200;
}

static void quip(p02_state *s, const char *text)
{
    snprintf(s->quip, sizeof s->quip, "%s", text);
    s->quip_timer = 240;
}

static void sfx(p02_state *s, cadv_sfx id, float vol)
{
    if (s->cfg.audio) {
        cadv_audio_sfx(s->cfg.audio, id, vol, 0.0f);
    }
}

static void flash(p02_state *s, cadv_vec3 pos)
{
    int i;
    int best = 0;
    for (i = 0; i < 16; ++i) {
        if (s->flash_ttl[i] < s->flash_ttl[best]) {
            best = i;
        }
    }
    s->flash_pos[best] = pos;
    s->flash_ttl[best] = 18;
}

static float hdist(cadv_vec3 a, cadv_vec3 b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return cadv_sqrtf(dx * dx + dz * dz);
}

static cadv_vec3 on_surface(const p02_state *s, float x, float z, float above)
{
    return cadv_v3(x, cadv_terrain_surface(&s->terrain, x, z) + above, z);
}

/* ---- mana bookkeeping ------------------------------------------------------------ */

int cadv_p02_add_orb(p02_state *s, cadv_vec3 pos, int value, int owner)
{
    int i;
    for (i = 0; i < s->norbs; ++i) {
        if (s->orbs[i].state == P02_ORB_GONE) {
            break;
        }
    }
    if (i == s->norbs) {
        if (s->norbs >= P02_MAX_ORBS) {
            return -1;
        }
        ++s->norbs;
    }
    s->orbs[i].pos = pos;
    s->orbs[i].value = value;
    s->orbs[i].owner = owner;
    s->orbs[i].state = P02_ORB_FREE;
    s->orbs[i].balloon = -1;
    return i;
}

int cadv_p02_owned_mana(const p02_state *s, int owner)
{
    int sum = s->castle[owner].mana;
    int i;
    for (i = 0; i < s->norbs; ++i) {
        if (s->orbs[i].state == P02_ORB_CARRIED && s->orbs[i].owner == owner) {
            sum += s->orbs[i].value;
        }
    }
    return sum;
}

int cadv_p02_total_mana(const p02_state *s)
{
    int sum = s->castle[P02_PLAYER].mana + s->castle[P02_VAEL].mana + s->wild_pool;
    int i;
    for (i = 0; i < s->norbs; ++i) {
        if (s->orbs[i].state != P02_ORB_GONE) {
            sum += s->orbs[i].value;
        }
    }
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        if (s->creatures[i].active) {
            sum += s->creatures[i].mana;
        }
    }
    return sum;
}

float cadv_p02_share(const p02_state *s, int owner)
{
    int total = cadv_p02_total_mana(s);
    return total > 0 ? (float)cadv_p02_owned_mana(s, owner) / (float)total : 0.0f;
}

static void update_castle_level(p02_state *s, int owner)
{
    p02_castle *c = &s->castle[owner];
    if (!c->exists) {
        c->level = 0;
        return;
    }
    c->level = cadv_p02_castle_level_for(c->mana);
    if (c->level > c->peak_level) {
        c->peak_level = c->level;
    }
}

static void spend(p02_state *s, int owner, int amount)
{
    amount = cadv_mini(amount, s->castle[owner].mana);
    s->castle[owner].mana -= amount;
    s->wild_pool += amount; /* spent mana drifts back into the wild */
    update_castle_level(s, owner);
}

static void drop_orb_of_balloon(p02_state *s, int b)
{
    p02_balloon *bl = &s->balloons[b];
    if (bl->orb >= 0) {
        p02_orb *o = &s->orbs[bl->orb];
        if (o->state == P02_ORB_CARRIED) {
            o->state = P02_ORB_FREE;
            o->pos = on_surface(s, bl->pos.x, bl->pos.z, 0.8f);
        }
        o->balloon = -1;
    }
    bl->active = false;
    bl->orb = -1;
}

int cadv_p02_damage_castle(p02_state *s, int owner, int amount)
{
    p02_castle *c = &s->castle[owner];
    int released = 0;
    int i;
    bool hit = amount > 0;
    if (!c->exists) {
        return 0;
    }
    amount = cadv_mini(amount, c->mana);
    while (released < amount) {
        int v = cadv_mini(10, amount - released);
        float a = cadv_rng_float01(&s->rng) * 2.0f * CADV_PI;
        float r = 9.0f + 6.0f * cadv_rng_float01(&s->rng);
        if (cadv_p02_add_orb(s, on_surface(s, c->pos.x + r * cadv_cosf(a), c->pos.z + r * cadv_sinf(a), 0.8f), v, P02_NONE) < 0) {
            break;
        }
        released += v;
    }
    c->mana -= released;
    c->hit_flash = 12;
    if (c->mana <= 0 && hit) {
        c->exists = false;
        c->destroyed = true;
        for (i = 0; i < P02_MAX_BALLOONS; ++i) {
            if (s->balloons[i].active && s->balloons[i].owner == owner) {
                drop_orb_of_balloon(s, i);
            }
        }
    }
    update_castle_level(s, owner);
    return released;
}

/* ---- creatures ------------------------------------------------------------------- */

int cadv_p02_spawn_creature(p02_state *s, int type, cadv_vec3 pos, int mana)
{
    int i;
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        p02_creature *c = &s->creatures[i];
        if (!c->active) {
            memset(c, 0, sizeof *c);
            c->active = true;
            c->type = type;
            c->home = pos;
            c->target = pos;
            c->hp = type == P02_WOLF ? 30 : 50;
            c->mana = mana;
            c->pos = type == P02_WOLF ? on_surface(s, pos.x, pos.z, 0.0f) : on_surface(s, pos.x, pos.z, 14.0f);
            return i;
        }
    }
    return -1;
}

static void kill_creature(p02_state *s, int i)
{
    p02_creature *c = &s->creatures[i];
    cadv_p02_add_orb(s, on_surface(s, c->pos.x, c->pos.z, 0.8f), c->mana, P02_NONE);
    c->active = false;
    c->mana = 0;
    s->last_events |= P02_EV_KILL;
}

static void hurt_player(p02_state *s, int dmg)
{
    float k = s->cfg.difficulty == CADV_DIFF_STORY ? 0.4f : (s->cfg.difficulty == CADV_DIFF_HARD ? 1.4f : 1.0f);
    int d = (int)((float)dmg * k + 0.5f);
    if (s->win_timer > 0 || s->hp <= 0) {
        return;
    }
    s->hp -= cadv_maxi(1, d);
    s->hurt_cd = 4.0f;
    s->hurt_flash = 10;
    s->last_events |= P02_EV_HURT;
}

static bool far_from_castles(const p02_state *s, cadv_vec3 p, float r)
{
    int o;
    for (o = P02_PLAYER; o <= P02_VAEL; ++o) {
        if (s->castle[o].exists && hdist(s->castle[o].pos, p) < r) {
            return false;
        }
    }
    return true;
}

static cadv_vec3 random_land(p02_state *s, int island)
{
    const cadv_island *is = &k_islands[island];
    int tries;
    cadv_vec3 p = cadv_v3(is->x, 0, is->z);
    for (tries = 0; tries < 16; ++tries) {
        float a = cadv_rng_float01(&s->rng) * 2.0f * CADV_PI;
        float r = is->radius * 0.6f * cadv_rng_float01(&s->rng);
        float x = is->x + r * cadv_cosf(a);
        float z = is->z + r * cadv_sinf(a);
        if (cadv_terrain_height(&s->terrain, x, z) > s->terrain.sea + 1.0f) {
            p = cadv_v3(x, 0, z);
            break;
        }
    }
    p.y = cadv_terrain_surface(&s->terrain, p.x, p.z);
    return p;
}

static void tick_creatures(p02_state *s)
{
    cadv_vec3 pp = s->player.pos;
    int i;
    int alive = 0;
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        p02_creature *c = &s->creatures[i];
        cadv_vec3 goal;
        cadv_vec3 to;
        float speed;
        float d;
        float alt_above;
        if (!c->active) {
            continue;
        }
        ++alive;
        if (c->hurt > 0) {
            --c->hurt;
        }
        c->cooldown -= P02_DT;
        c->wander -= P02_DT;
        d = cadv_v3_dist(c->pos, pp);
        alt_above = pp.y - cadv_terrain_surface(&s->terrain, pp.x, pp.z);
        if (c->wander <= 0.0f) {
            float a = cadv_rng_float01(&s->rng) * 2.0f * CADV_PI;
            float r = 6.0f + 14.0f * cadv_rng_float01(&s->rng);
            c->target = cadv_v3(c->home.x + r * cadv_cosf(a), 0, c->home.z + r * cadv_sinf(a));
            c->wander = 3.0f + 3.0f * cadv_rng_float01(&s->rng);
        }
        goal = c->target;
        speed = c->type == P02_WOLF ? 3.0f : 6.0f;
        if (s->hp > 0 && hdist(c->pos, pp) < (c->type == P02_WOLF ? 18.0f : 32.0f) &&
            (c->type == P02_GRIFFIN || alt_above < 12.0f)) {
            goal = pp;
            speed = c->type == P02_WOLF ? 6.0f : 9.0f;
            if (d < 3.2f && c->cooldown <= 0.0f) {
                hurt_player(s, c->type == P02_WOLF ? 6 : 9);
                c->cooldown = c->type == P02_WOLF ? 1.2f : 1.6f;
            }
        }
        to = cadv_v3_sub(goal, c->pos);
        to.y = 0.0f;
        if (cadv_v3_len(to) > 0.3f) {
            cadv_vec3 step = cadv_v3_scale(cadv_v3_norm(to), speed * P02_DT);
            cadv_vec3 next = cadv_v3_add(c->pos, step);
            c->yaw = cadv_atan2f(-to.x, -to.z);
            if (c->type == P02_GRIFFIN || cadv_terrain_height(&s->terrain, next.x, next.z) > s->terrain.sea + 0.5f) {
                c->pos.x = next.x;
                c->pos.z = next.z;
            } else {
                c->wander = 0.0f; /* wolves do not swim */
            }
        }
        if (c->type == P02_WOLF) {
            c->pos.y = cadv_terrain_surface(&s->terrain, c->pos.x, c->pos.z);
        } else {
            float want = goal.y > 0.0f && cadv_v3_dist(goal, pp) < 0.01f ? pp.y : cadv_terrain_surface(&s->terrain, c->pos.x, c->pos.z) + 14.0f;
            c->pos.y += cadv_clampf(want - c->pos.y, -6.0f * P02_DT, 6.0f * P02_DT);
            c->pos.y = cadv_maxf(c->pos.y, cadv_terrain_surface(&s->terrain, c->pos.x, c->pos.z) + 1.0f);
        }
    }
    /* the wild reclaims spent mana as new creatures */
    s->spawn_cd -= P02_DT;
    if (s->spawn_cd <= 0.0f) {
        s->spawn_cd = 20.0f;
        if (s->wild_pool >= 15 && alive < 20) {
            int isl = 2 + (int)cadv_rng_below(&s->rng, 3);
            bool griffin = s->wild_pool >= 30 && cadv_rng_below(&s->rng, 3) == 0;
            cadv_vec3 p = random_land(s, isl);
            if (far_from_castles(s, p, 35.0f) && cadv_p02_spawn_creature(s, griffin ? P02_GRIFFIN : P02_WOLF, p, griffin ? 30 : 15) >= 0) {
                s->wild_pool -= griffin ? 30 : 15;
            }
        }
    }
}

/* ---- castles & balloons --------------------------------------------------------- */

bool cadv_p02_place_castle(p02_state *s, int owner, cadv_vec3 at)
{
    p02_castle *c = &s->castle[owner];
    int other = owner == P02_PLAYER ? P02_VAEL : P02_PLAYER;
    float avg;
    if (c->exists) {
        return false;
    }
    if (cadv_terrain_height(&s->terrain, at.x, at.z) < s->terrain.sea + 1.0f ||
        cadv_terrain_roughness(&s->terrain, at.x, at.z, 6.0f) > 5.0f ||
        (s->castle[other].exists && hdist(s->castle[other].pos, at) < 50.0f)) {
        return false;
    }
    avg = cadv_terrain_height(&s->terrain, at.x, at.z);
    cadv_terrain_flatten(&s->terrain, at.x, at.z, 9.0f, avg, 100.0f);
    c->exists = true;
    c->destroyed = false;
    c->pos = cadv_v3(at.x, avg, at.z);
    c->balloon_cd = 1.0f;
    update_castle_level(s, owner);
    return true;
}

static cadv_vec3 castle_top(const p02_castle *c)
{
    return cadv_v3_add(c->pos, cadv_v3(0, 4.0f + 2.0f * (float)c->level, 0));
}

static int count_balloons(const p02_state *s, int owner)
{
    int i;
    int n = 0;
    for (i = 0; i < P02_MAX_BALLOONS; ++i) {
        n += s->balloons[i].active && s->balloons[i].owner == owner;
    }
    return n;
}

static void spawn_balloons(p02_state *s, int owner)
{
    p02_castle *c = &s->castle[owner];
    int i;
    int b;
    int best = -1;
    float bd = 1e9f;
    if (!c->exists) {
        return;
    }
    c->balloon_cd -= P02_DT;
    if (c->balloon_cd > 0.0f || count_balloons(s, owner) >= 2 + c->level) {
        return;
    }
    for (i = 0; i < s->norbs; ++i) {
        const p02_orb *o = &s->orbs[i];
        float d;
        if (o->state != P02_ORB_FREE || o->owner != owner || o->balloon >= 0) {
            continue;
        }
        d = hdist(o->pos, c->pos);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    if (best < 0) {
        return;
    }
    for (b = 0; b < P02_MAX_BALLOONS; ++b) {
        if (!s->balloons[b].active) {
            p02_balloon *bl = &s->balloons[b];
            bl->active = true;
            bl->owner = owner;
            bl->pos = castle_top(c);
            bl->orb = best;
            bl->carrying = false;
            s->orbs[best].balloon = b;
            c->balloon_cd = 2.5f;
            return;
        }
    }
}

static void tick_balloons(p02_state *s)
{
    int i;
    for (i = 0; i < P02_MAX_BALLOONS; ++i) {
        p02_balloon *b = &s->balloons[i];
        p02_castle *home = &s->castle[b->owner];
        cadv_vec3 target;
        cadv_vec3 to;
        float hd;
        float want_y;
        float cruise;
        cadv_vec3 dir;
        if (!b->active) {
            continue;
        }
        if (!home->exists) {
            drop_orb_of_balloon(s, i);
            continue;
        }
        if (!b->carrying && (b->orb < 0 || s->orbs[b->orb].state != P02_ORB_FREE || s->orbs[b->orb].owner != b->owner)) {
            if (b->orb >= 0 && s->orbs[b->orb].balloon == i) {
                s->orbs[b->orb].balloon = -1;
            }
            b->orb = -1; /* stolen or gone: fly home empty */
        }
        target = b->carrying || b->orb < 0 ? castle_top(home) : s->orbs[b->orb].pos;
        to = cadv_v3_sub(target, b->pos);
        hd = cadv_sqrtf(to.x * to.x + to.z * to.z);
        dir = hd > 0.01f ? cadv_v3(to.x / hd, 0, to.z / hd) : cadv_v3(0, 0, 0);
        /* cruise above whatever land lies ahead (raised walls included) */
        cruise = cadv_maxf(cadv_terrain_surface(&s->terrain, b->pos.x, b->pos.z),
                           cadv_maxf(cadv_terrain_surface(&s->terrain, b->pos.x + dir.x * 8.0f, b->pos.z + dir.z * 8.0f),
                                     cadv_terrain_surface(&s->terrain, b->pos.x + dir.x * 16.0f, b->pos.z + dir.z * 16.0f))) + 9.0f;
        want_y = hd > 6.0f ? cadv_maxf(cruise, target.y + 4.0f) : target.y + 1.0f;
        b->pos.y += cadv_clampf(want_y - b->pos.y, -8.0f * P02_DT, 10.0f * P02_DT);
        b->pos.y = cadv_maxf(b->pos.y, cadv_terrain_surface(&s->terrain, b->pos.x, b->pos.z) + 1.5f);
        /* only drift forward when not blocked by a rise right ahead */
        if (b->pos.y > cruise - 4.0f || hd <= 6.0f) {
            float step = cadv_minf(hd, 13.0f * P02_DT);
            b->pos.x += dir.x * step;
            b->pos.z += dir.z * step;
        }
        if (b->carrying) {
            s->orbs[b->orb].pos = cadv_v3_sub(b->pos, cadv_v3(0, 1.6f, 0));
            if (hd < 3.0f && cadv_absf(to.y) < 4.0f) {
                home->mana += s->orbs[b->orb].value;
                s->orbs[b->orb].state = P02_ORB_GONE;
                s->orbs[b->orb].balloon = -1;
                b->active = false;
                update_castle_level(s, b->owner);
                if (b->owner == P02_PLAYER) {
                    s->last_events |= P02_EV_BANK;
                }
            }
        } else if (b->orb >= 0) {
            if (cadv_v3_dist(b->pos, target) < 2.5f) {
                b->carrying = true;
                s->orbs[b->orb].state = P02_ORB_CARRIED;
            }
        } else if (hd < 3.0f && cadv_absf(to.y) < 4.0f) {
            b->active = false;
        }
    }
}

static void tick_turrets(p02_state *s, int owner)
{
    p02_castle *c = &s->castle[owner];
    cadv_vec3 top;
    cadv_vec3 best_p = cadv_v3(0, 0, 0);
    float bd = 34.0f;
    int i;
    if (!c->exists || c->level < 2) {
        return;
    }
    c->turret_cd -= P02_DT;
    if (c->turret_cd > 0.0f) {
        return;
    }
    top = castle_top(c);
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        const p02_creature *cr = &s->creatures[i];
        if (cr->active && cadv_v3_dist(cr->pos, top) < bd) {
            bd = cadv_v3_dist(cr->pos, top);
            best_p = cadv_v3_add(cr->pos, cadv_v3(0, 0.8f, 0));
        }
    }
    for (i = 0; i < P02_MAX_BALLOONS; ++i) {
        const p02_balloon *b = &s->balloons[i];
        if (b->active && b->owner != owner && cadv_v3_dist(b->pos, top) < bd) {
            bd = cadv_v3_dist(b->pos, top);
            best_p = b->pos;
        }
    }
    if (owner == P02_PLAYER && s->vael.alive && cadv_v3_dist(s->vael.fly.pos, top) < bd + 6.0f) {
        bd = cadv_v3_dist(s->vael.fly.pos, top);
        best_p = s->vael.fly.pos;
    }
    if (owner == P02_VAEL && s->hp > 0 && cadv_v3_dist(s->player.pos, top) < bd) {
        bd = cadv_v3_dist(s->player.pos, top);
        best_p = s->player.pos;
    }
    if (bd < 34.0f + 6.0f && (best_p.x != 0.0f || best_p.z != 0.0f)) {
        int j;
        for (j = 0; j < P02_MAX_PROJ; ++j) {
            if (!s->proj[j].active) {
                p02_proj *p = &s->proj[j];
                p->active = true;
                p->kind = P02_PROJ_FIREBALL;
                p->owner = owner;
                p->pos = top;
                p->vel = cadv_v3_scale(cadv_v3_norm(cadv_v3_sub(best_p, top)), 36.0f);
                p->life = 1.6f;
                break;
            }
        }
        c->turret_cd = owner == P02_VAEL && s->cfg.difficulty == CADV_DIFF_STORY ? 3.0f : 1.8f;
    }
}

/* ---- spells ---------------------------------------------------------------------- */

static bool spawn_proj(p02_state *s, int kind, int owner, cadv_vec3 from, cadv_vec3 dir)
{
    int i;
    for (i = 0; i < P02_MAX_PROJ; ++i) {
        if (!s->proj[i].active) {
            p02_proj *p = &s->proj[i];
            p->active = true;
            p->kind = kind;
            p->owner = owner;
            p->pos = from;
            p->vel = cadv_v3_scale(cadv_v3_norm(dir), kind == P02_PROJ_CLAIM ? 60.0f : 42.0f);
            p->life = kind == P02_PROJ_CLAIM ? 1.2f : 2.5f;
            return true;
        }
    }
    return false;
}

static bool near_any_castle(const p02_state *s, cadv_vec3 p, float r)
{
    return !far_from_castles(s, p, r);
}

bool cadv_p02_cast(p02_state *s, int caster, int spell, cadv_vec3 from, cadv_vec3 dir)
{
    p02_castle *c = &s->castle[caster];
    int cost = k_spell_cost[spell];
    cadv_vec3 hit;
    bool landed;
    if (c->mana < cost) {
        return false;
    }
    landed = cadv_terrain_raycast(&s->terrain, from, dir, 110.0f, &hit);
    switch (spell) {
    case P02_SPELL_FIREBALL:
        if (!spawn_proj(s, P02_PROJ_FIREBALL, caster, from, dir)) {
            return false;
        }
        break;
    case P02_SPELL_CLAIM:
        if (!spawn_proj(s, P02_PROJ_CLAIM, caster, from, dir)) {
            return false;
        }
        break;
    case P02_SPELL_RAISE:
    case P02_SPELL_LOWER:
        if (!landed || near_any_castle(s, hit, 11.0f)) {
            return false;
        }
        cadv_terrain_deform(&s->terrain, hit.x, hit.z, 8.0f, spell == P02_SPELL_RAISE ? 4.0f : -4.0f);
        s->last_events |= P02_EV_DEFORM;
        break;
    case P02_SPELL_CASTLE:
        if (!landed || !cadv_p02_place_castle(s, caster, hit)) {
            return false;
        }
        s->last_events |= P02_EV_CASTLE;
        break;
    default: /* volcano */
        if (!landed || s->volcano_ticks > 0) {
            return false;
        }
        s->volcano_pos = hit;
        s->volcano_ticks = 180;
        break;
    }
    spend(s, caster, cost);
    if (caster == P02_PLAYER) {
        s->last_events |= P02_EV_CAST;
    }
    return true;
}

static void damage_vael(p02_state *s, int dmg)
{
    if (!s->vael.alive) {
        return;
    }
    s->vael.hp -= dmg;
    if (s->vael.hp <= 0) {
        s->vael.alive = false;
        s->vael.respawn = 20 * 60;
        cadv_p02_add_orb(s, on_surface(s, s->vael.fly.pos.x, s->vael.fly.pos.z, 0.8f), cadv_mini(20, s->castle[P02_VAEL].mana), P02_NONE);
        s->castle[P02_VAEL].mana -= cadv_mini(20, s->castle[P02_VAEL].mana);
        update_castle_level(s, P02_VAEL);
        say(s, "VAEL TUMBLES FROM HIS CARPET!");
        s->last_events |= P02_EV_KILL;
    }
}

static void explode(p02_state *s, const p02_proj *p)
{
    int o;
    flash(s, p->pos);
    if (p->owner == P02_PLAYER || p->owner == P02_VAEL) {
        if (cadv_terrain_height(&s->terrain, p->pos.x, p->pos.z) >= p->pos.y - 0.5f && !near_any_castle(s, p->pos, 11.0f)) {
            cadv_terrain_deform(&s->terrain, p->pos.x, p->pos.z, 3.0f, -0.6f);
        }
    }
    for (o = P02_PLAYER; o <= P02_VAEL; ++o) {
        p02_castle *c = &s->castle[o];
        /* Vael's ward holds while he lives, and until nightfall */
        bool warded = o == P02_VAEL && (s->vael.alive || s->phase < P02_PHASE_NIGHT);
        if (o != p->owner && c->exists && !warded &&
            cadv_v3_dist(p->pos, cadv_v3_add(c->pos, cadv_v3(0, 2.0f + (float)c->level, 0))) < 6.0f + (float)c->level) {
            cadv_p02_damage_castle(s, o, 8);
        }
    }
}

static void tick_proj(p02_state *s)
{
    int i;
    int j;
    for (i = 0; i < P02_MAX_PROJ; ++i) {
        p02_proj *p = &s->proj[i];
        bool done = false;
        if (!p->active) {
            continue;
        }
        p->pos = cadv_v3_add(p->pos, cadv_v3_scale(p->vel, P02_DT));
        p->life -= P02_DT;
        if (p->kind == P02_PROJ_CLAIM) {
            for (j = 0; j < s->norbs && !done; ++j) {
                p02_orb *o = &s->orbs[j];
                if (o->state == P02_ORB_FREE && o->owner != p->owner && cadv_v3_dist(o->pos, p->pos) < 3.5f) {
                    o->owner = p->owner;
                    if (o->balloon >= 0) {
                        s->orbs[j].balloon = -1;
                    }
                    if (p->owner == P02_PLAYER) {
                        s->last_events |= P02_EV_CLAIM;
                    }
                    done = true;
                }
            }
        } else {
            for (j = 0; j < P02_MAX_CREATURES && !done; ++j) {
                p02_creature *c = &s->creatures[j];
                if (c->active && cadv_v3_dist(cadv_v3_add(c->pos, cadv_v3(0, 0.8f, 0)), p->pos) < 1.8f) {
                    c->hp -= p->owner == P02_PLAYER ? 34 : 25;
                    c->hurt = 8;
                    if (p->owner == P02_PLAYER) {
                        s->last_events |= P02_EV_HIT;
                    }
                    if (c->hp <= 0) {
                        kill_creature(s, j);
                    }
                    done = true;
                }
            }
            for (j = 0; j < P02_MAX_BALLOONS && !done; ++j) {
                if (s->balloons[j].active && s->balloons[j].owner != p->owner && cadv_v3_dist(s->balloons[j].pos, p->pos) < 1.8f) {
                    drop_orb_of_balloon(s, j);
                    done = true;
                }
            }
            if (!done && p->owner != P02_PLAYER && s->hp > 0 && cadv_v3_dist(s->player.pos, p->pos) < 1.6f) {
                hurt_player(s, 10);
                done = true;
            }
            if (!done && p->owner != P02_VAEL && s->vael.alive && cadv_v3_dist(s->vael.fly.pos, p->pos) < 2.0f) {
                damage_vael(s, p->owner == P02_PLAYER ? 30 : 20);
                if (p->owner == P02_PLAYER) {
                    s->last_events |= P02_EV_HIT;
                }
                done = true;
            }
            if (!done) {
                int o;
                for (o = P02_PLAYER; o <= P02_VAEL && !done; ++o) {
                    const p02_castle *c = &s->castle[o];
                    if (o != p->owner && c->exists &&
                        cadv_v3_dist(p->pos, cadv_v3_add(c->pos, cadv_v3(0, 2.0f + (float)c->level, 0))) < 3.0f + (float)c->level) {
                        done = true;
                    }
                }
            }
            if (done || p->pos.y <= cadv_terrain_height(&s->terrain, p->pos.x, p->pos.z)) {
                explode(s, p);
                done = true;
            }
        }
        if (done || p->life <= 0.0f || (p->kind == P02_PROJ_CLAIM && p->pos.y <= cadv_terrain_height(&s->terrain, p->pos.x, p->pos.z))) {
            p->active = false;
        }
    }
}

static void tick_volcano(p02_state *s)
{
    int o;
    int i;
    if (s->volcano_ticks <= 0) {
        return;
    }
    cadv_terrain_deform(&s->terrain, s->volcano_pos.x, s->volcano_pos.z, 14.0f, 0.08f);
    s->shake = cadv_maxi(s->shake, 4);
    if (--s->volcano_ticks > 0) {
        return;
    }
    s->shake = 60;
    s->last_events |= P02_EV_ERUPT;
    flash(s, cadv_v3_add(s->volcano_pos, cadv_v3(0, 14, 0)));
    for (o = P02_PLAYER; o <= P02_VAEL; ++o) {
        if (s->castle[o].exists && hdist(s->castle[o].pos, s->volcano_pos) < 22.0f) {
            cadv_p02_damage_castle(s, o, 160);
        }
    }
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        if (s->creatures[i].active && hdist(s->creatures[i].pos, s->volcano_pos) < 16.0f) {
            kill_creature(s, i);
        }
    }
}

/* ---- Vael, the Frost Sorcerer ------------------------------------------------------ */

static void steer(const cadv_flyer *f, const cadv_terrain *t, cadv_vec3 target, float alt, float stop, cadv_input_frame *out)
{
    cadv_vec3 to = cadv_v3_sub(target, f->pos);
    float want_yaw = cadv_atan2f(-to.x, -to.z);
    float dy = cadv_wrap_angle(want_yaw - f->yaw);
    float hd = cadv_sqrtf(to.x * to.x + to.z * to.z);
    float floor_y = cadv_maxf(cadv_terrain_surface(t, f->pos.x, f->pos.z),
                              cadv_terrain_surface(t, f->pos.x - cadv_sinf(f->yaw) * 12.0f, f->pos.z - cadv_cosf(f->yaw) * 12.0f));
    float want_y = cadv_maxf(floor_y + alt, target.y);
    out->look_x = cadv_clampf(dy, -0.08f, 0.08f);
    out->look_y = cadv_clampf(-f->pitch, -0.05f, 0.05f);
    out->move_y = hd > stop ? (cadv_absf(dy) < 1.2f ? 1.0f : 0.2f) : 0.0f;
    if (f->pos.y < want_y - 1.0f) {
        out->held |= CADV_ACT_BIT(CADV_ACT_JUMP);
    } else if (f->pos.y > want_y + 3.0f) {
        out->held |= CADV_ACT_BIT(CADV_ACT_CROUCH);
    }
}

static int nearest_orb(const p02_state *s, cadv_vec3 from, int not_owner, float max_d)
{
    int i;
    int best = -1;
    float bd = max_d;
    for (i = 0; i < s->norbs; ++i) {
        const p02_orb *o = &s->orbs[i];
        float d;
        if (o->state != P02_ORB_FREE || o->owner == not_owner) {
            continue;
        }
        d = hdist(o->pos, from) + (o->owner == P02_NONE ? 0.0f : 25.0f); /* prefer wild mana */
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

static void vael_cast_at(p02_state *s, int spell, cadv_vec3 target)
{
    p02_wizard *v = &s->vael;
    cadv_vec3 from = cadv_v3_add(v->fly.pos, cadv_v3(0, 0.5f, 0));
    float rate = s->cfg.difficulty == CADV_DIFF_STORY ? 2.4f : (s->cfg.difficulty == CADV_DIFF_HARD ? 0.9f : 1.4f);
    if (v->cast_cd > 0.0f) {
        return;
    }
    if (cadv_p02_cast(s, P02_VAEL, spell, from, cadv_v3_sub(target, from))) {
        v->cast_cd = rate;
    }
}

static void tick_vael(p02_state *s)
{
    p02_wizard *v = &s->vael;
    p02_castle *home = &s->castle[P02_VAEL];
    cadv_input_frame in;
    cadv_flyer_def def = s->fdef;
    float dp;
    memset(&in, 0, sizeof in);
    if (!v->alive) {
        if (v->respawn > 0 && --v->respawn == 0 && home->exists) {
            v->alive = true;
            v->hp = P02_VAEL_HP;
            cadv_flyer_spawn(&v->fly, cadv_v3_add(castle_top(home), cadv_v3(0, 6, 0)), 0.0f);
        }
        return;
    }
    v->cast_cd -= P02_DT;
    def.max_speed = s->cfg.difficulty == CADV_DIFF_STORY ? 9.0f : (s->cfg.difficulty == CADV_DIFF_HARD ? 15.0f : 12.0f);
    dp = cadv_v3_dist(v->fly.pos, s->player.pos);
    if (v->hp < P02_VAEL_HP * 3 / 10 && home->exists) {
        v->mode = 3;
    } else if (v->mode == 3 && (v->hp >= P02_VAEL_HP || !home->exists)) {
        v->mode = 0;
    } else if (v->mode != 3) {
        v->mode = s->phase == P02_PHASE_SIEGE && s->castle[P02_PLAYER].exists ? 2 : 0;
    }
    if (v->mode == 3) {
        steer(&v->fly, &s->terrain, castle_top(home), 8.0f, 4.0f, &in);
        if (cadv_v3_dist(v->fly.pos, castle_top(home)) < 16.0f && s->ticks % 6 == 0) {
            v->hp = cadv_mini(P02_VAEL_HP, v->hp + 1);
        }
    } else if (s->hp > 0 && dp < 30.0f && cadv_terrain_line_clear(&s->terrain, v->fly.pos, s->player.pos)) {
        /* duel the apprentice when she comes close */
        steer(&v->fly, &s->terrain, s->player.pos, 8.0f, 18.0f, &in);
        vael_cast_at(s, P02_SPELL_FIREBALL, s->player.pos);
    } else if (v->mode == 2) {
        cadv_vec3 tgt = castle_top(&s->castle[P02_PLAYER]);
        steer(&v->fly, &s->terrain, tgt, 14.0f, 28.0f, &in);
        if (cadv_v3_dist(v->fly.pos, tgt) < 45.0f && cadv_terrain_line_clear(&s->terrain, v->fly.pos, tgt)) {
            vael_cast_at(s, P02_SPELL_FIREBALL, cadv_v3_sub(tgt, cadv_v3(0, 2, 0)));
        }
    } else {
        if (v->target_orb < 0 || s->orbs[v->target_orb].state != P02_ORB_FREE || s->orbs[v->target_orb].owner == P02_VAEL) {
            v->target_orb = nearest_orb(s, v->fly.pos, P02_VAEL, 400.0f);
        }
        if (v->target_orb >= 0) {
            cadv_vec3 o = s->orbs[v->target_orb].pos;
            steer(&v->fly, &s->terrain, o, 12.0f, 18.0f, &in);
            if (cadv_v3_dist(v->fly.pos, o) < 26.0f && cadv_terrain_line_clear(&s->terrain, cadv_v3_add(v->fly.pos, cadv_v3(0, 0.5f, 0)), cadv_v3_add(o, cadv_v3(0, 1, 0)))) {
                vael_cast_at(s, P02_SPELL_CLAIM, o);
            }
        } else {
            /* nothing to steal: hunt creatures for their mana */
            int i;
            int best = -1;
            float bd = 1e9f;
            for (i = 0; i < P02_MAX_CREATURES; ++i) {
                if (s->creatures[i].active && cadv_v3_dist(s->creatures[i].pos, v->fly.pos) < bd) {
                    bd = cadv_v3_dist(s->creatures[i].pos, v->fly.pos);
                    best = i;
                }
            }
            if (best >= 0) {
                cadv_vec3 c = cadv_v3_add(s->creatures[best].pos, cadv_v3(0, 0.8f, 0));
                steer(&v->fly, &s->terrain, c, 10.0f, 16.0f, &in);
                if (bd < 28.0f && cadv_terrain_line_clear(&s->terrain, v->fly.pos, c)) {
                    vael_cast_at(s, P02_SPELL_FIREBALL, c);
                }
            } else if (home->exists) {
                steer(&v->fly, &s->terrain, castle_top(home), 12.0f, 10.0f, &in);
            }
        }
    }
    cadv_flyer_tick(&v->fly, &def, &in, &s->terrain, P02_DT);
}

/* ---- setup ----------------------------------------------------------------------- */

static void stamp_line(cadv_terrain *t, float x0, float z0, float x1, float z1, float r, float h)
{
    float len = cadv_sqrtf((x1 - x0) * (x1 - x0) + (z1 - z0) * (z1 - z0));
    int n = (int)(len / 2.0f) + 1;
    int i;
    for (i = 0; i <= n; ++i) {
        float k = (float)i / (float)n;
        cadv_terrain_raise_to(t, x0 + (x1 - x0) * k, z0 + (z1 - z0) * k, r, h);
    }
}

static void stamp_runes(p02_state *s, const char *text)
{
    int len = (int)strlen(text);
    int i;
    int x;
    int y;
    float ox;
    float oz = s->rune_centre.z - 7.0f;
    if (len > 5) {
        len = 5;
    }
    ox = s->rune_centre.x - (float)(len * 6 - 1) * P02_CELL * 0.5f;
    for (i = 0; i < len; ++i) {
        for (y = 0; y < 7; ++y) {
            for (x = 0; x < 5; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x, y)) {
                    cadv_terrain_deform(&s->terrain, ox + (float)(i * 6 + x) * P02_CELL, oz + (float)y * P02_CELL,
                                        P02_CELL * 0.9f, 1.6f);
                }
            }
        }
    }
}

static void build_world(p02_state *s)
{
    cadv_rng layout;
    int i;
    int k;
    cadv_terrain_init(&s->terrain, P02_N, P02_CELL, 0.0f, 64.0f, 6.0f);
    cadv_terrain_islands(&s->terrain, k_islands, ISLANDS, LAYOUT_SEED);
    /* Messerjocke's Rock: a dagger-shaped crag in the southern sea */
    stamp_line(&s->terrain, 104.f, 236.f, 146.f, 236.f, 5.0f, 22.0f);   /* blade */
    stamp_line(&s->terrain, 148.f, 226.f, 148.f, 246.f, 4.0f, 16.0f);   /* cross-guard */
    stamp_line(&s->terrain, 150.f, 236.f, 160.f, 236.f, 4.0f, 14.0f);   /* grip */
    s->rock_pos = cadv_v3(130.f, cadv_terrain_height(&s->terrain, 130.f, 236.f), 236.f);
    /* the dagger's tip dips into the sea */
    cadv_terrain_raise_to(&s->terrain, 100.f, 236.f, 3.0f, 10.0f);
    /* the frozen crater on the west isle */
    cadv_terrain_deform(&s->terrain, 58.f, 60.f, 9.0f, -8.0f);
    s->scroll_pos = cadv_v3(58.f, cadv_terrain_height(&s->terrain, 58.f, 60.f) + 2.0f, 60.f);
    s->rune_centre = cadv_v3(k_islands[4].x, 0, k_islands[4].z);
    stamp_runes(s, s->mark_text[1]);
    s->rune_centre.y = cadv_terrain_height(&s->terrain, s->rune_centre.x, s->rune_centre.z);
    s->home_spot = cadv_v3(k_islands[0].x, cadv_terrain_height(&s->terrain, k_islands[0].x, k_islands[0].z), k_islands[0].z);
    /* Vael's castle */
    cadv_p02_place_castle(s, P02_VAEL, cadv_v3(k_islands[1].x, 0, k_islands[1].z));
    s->castle[P02_VAEL].mana = 200;
    update_castle_level(s, P02_VAEL);
    s->castle[P02_PLAYER].mana = 30;
    /* free mana: 4 near home for the tutorial, the rest across the fjords */
    cadv_rng_seed(&layout, LAYOUT_SEED, 7u);
    for (i = 0; i < 40; ++i) {
        int isl = i < 4 ? 0 : 1 + (i % (ISLANDS - 1));
        const cadv_island *is = &k_islands[isl];
        cadv_vec3 p = cadv_v3(is->x, 0, is->z);
        for (k = 0; k < 24; ++k) {
            float a = cadv_rng_float01(&layout) * 2.0f * CADV_PI;
            float r = (isl == 0 ? 10.0f : 6.0f) + is->radius * 0.55f * cadv_rng_float01(&layout);
            float x = is->x + r * cadv_cosf(a);
            float z = is->z + r * cadv_sinf(a);
            if (cadv_terrain_height(&s->terrain, x, z) > s->terrain.sea + 1.0f && (isl != 1 || r > 14.0f)) {
                p = cadv_v3(x, 0, z);
                break;
            }
        }
        cadv_p02_add_orb(s, on_surface(s, p.x, p.z, 0.8f), 10, P02_NONE);
    }
    /* creatures carry the rest */
    for (i = 0; i < 12; ++i) {
        int isl = 2 + i % 3;
        cadv_vec3 p = cadv_v3(k_islands[isl].x + (float)((i * 37) % 21 - 10), 0, k_islands[isl].z + (float)((i * 53) % 21 - 10));
        cadv_p02_spawn_creature(s, P02_WOLF, p, 15);
    }
    for (i = 0; i < 6; ++i) {
        int isl = 2 + i % 3;
        cadv_p02_spawn_creature(s, P02_GRIFFIN, cadv_v3(k_islands[isl].x + 8.0f, 0, k_islands[isl].z - 8.0f), 30);
    }
    s->wild_pool = CADV_P02_WORLD_MANA - cadv_p02_total_mana(s);
}

bool cadv_p02_init(void *state, const cadv_portal_cfg *cfg)
{
    p02_state *s = (p02_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    s->checkpoint = cfg->checkpoint;
    cadv_rng_seed(&s->rng, cfg->seed, 2u);
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
    build_world(s);
    s->fdef.max_speed = 18.0f;
    s->fdef.accel = 32.0f;
    s->fdef.drag = 1.6f;
    s->fdef.climb = 10.0f;
    s->fdef.clearance = 2.5f;
    s->fdef.ceiling = 110.0f;
    s->fdef.boost = 1.6f;
    s->hp = P02_PLAYER_HP;
    s->spell = P02_SPELL_FIREBALL;
    s->spawn_cd = 20.0f;
    s->vael.alive = true;
    s->vael.hp = P02_VAEL_HP;
    s->vael.target_orb = -1;
    cadv_flyer_spawn(&s->vael.fly, cadv_v3_add(castle_top(&s->castle[P02_VAEL]), cadv_v3(0, 8, 0)), 0.0f);
    cadv_flyer_spawn(&s->player, on_surface(s, 34.f, 214.f, 9.0f), cadv_atan2f(-(s->home_spot.x - 34.f), -(s->home_spot.z - 214.f)));
    if (s->checkpoint >= 1) {
        cadv_p02_place_castle(s, P02_PLAYER, s->home_spot);
        s->phase = P02_PHASE_EXPANSION;
        s->player.pos = cadv_v3_add(castle_top(&s->castle[P02_PLAYER]), cadv_v3(0, 8, 10));
    }
    if (s->checkpoint >= 2) {
        /* resume at nightfall with the mana an apprentice would have banked */
        for (i = 0; i < s->norbs && s->castle[P02_PLAYER].mana < 300; ++i) {
            if (s->orbs[i].owner == P02_NONE && s->orbs[i].state == P02_ORB_FREE) {
                s->castle[P02_PLAYER].mana += s->orbs[i].value;
                s->orbs[i].state = P02_ORB_GONE;
            }
        }
        update_castle_level(s, P02_PLAYER);
        s->reached_level2 = s->castle_kept = true;
        s->phase = P02_PHASE_NIGHT;
        s->scroll_active = true;
        s->ticks = 8 * 60 * 60;
    }
    quip(s, s->checkpoint >= 1 ? "Grandmother's carpet remembers the way."
                               : "Steady, carpet. Let's bring the aurora home.");
    say(s, s->checkpoint >= 1 ? "CLAIM MANA - YOUR BALLOONS WILL FETCH IT" : "FLY TO THE PLATEAU AND CAST CASTLE (5)");
    return true;
}

/* ---- per-tick rules ---------------------------------------------------------------- */

static void check_marks(p02_state *s)
{
    if (!(s->marks_found & 2u) && s->player.pos.y > 50.0f && hdist(s->player.pos, s->rune_centre) < 60.0f) {
        s->marks_found |= 2u;
        say(s, "GIANT RUNES BELOW - A FRIEND'S MARK!");
    }
    if (!(s->marks_found & 4u) && cadv_v3_dist(s->player.pos, s->rock_pos) < 34.0f) {
        s->marks_found |= 4u;
        say(s, "A DAGGER-SHAPED ROCK - A FRIEND'S MARK!");
    }
}

/* True when no unclaimed orb and no creature is left: the fjords are harvested. */
static bool wild_mana_gone(const p02_state *s)
{
    int i;
    for (i = 0; i < s->norbs; ++i) {
        if (s->orbs[i].state == P02_ORB_FREE && s->orbs[i].owner == P02_NONE) {
            return false;
        }
    }
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        if (s->creatures[i].active) {
            return false;
        }
    }
    return true;
}

static void advance_phase(p02_state *s)
{
    float share = cadv_p02_share(s, P02_PLAYER);
    if (s->phase == P02_PHASE_DUSK && (s->castle[P02_PLAYER].exists || s->ticks > 3 * 3600)) {
        s->phase = P02_PHASE_EXPANSION;
        if (s->castle[P02_PLAYER].exists && s->checkpoint < 1) {
            s->checkpoint = 1;
        }
        say(s, "CLAIM MANA (ALT) - YOUR BALLOONS WILL FETCH IT");
    } else if (s->phase == P02_PHASE_EXPANSION && (s->ticks > 8 * 3600 || (s->ticks > 4 * 3600 && wild_mana_gone(s)))) {
        s->phase = P02_PHASE_NIGHT;
        s->scroll_active = !s->has_volcano;
        if (s->checkpoint < 2 && s->castle[P02_PLAYER].exists) {
            s->checkpoint = 2;
        }
        say(s, "NIGHT FALLS - A SCROLL GLOWS IN THE WEST CRATER");
        quip(s, "Only the aurora and my fireballs to see by.");
    } else if (s->phase == P02_PHASE_NIGHT && (s->ticks > 13 * 3600 || share >= 0.5f)) {
        char buf[80];
        s->phase = P02_PHASE_SIEGE;
        snprintf(buf, sizeof buf, "VAEL: %s", s->mark_text[0]);
        quip(s, buf);
        say(s, "VAEL LAYS SIEGE TO YOUR CASTLE!");
        s->marks_found |= 1u;
    }
}

static void player_controls(p02_state *s, const cadv_input_frame *in)
{
    int i;
    cadv_vec3 eye = cadv_v3_add(s->player.pos, cadv_v3(0, 0.9f, 0));
    cadv_vec3 fwd = cadv_flyer_forward(&s->player);
    for (i = 0; i < P02_SPELLS; ++i) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + i)) && (i != P02_SPELL_VOLCANO || s->has_volcano)) {
            s->spell = i;
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        int n = s->has_volcano ? P02_SPELLS : P02_SPELLS - 1;
        s->spell = (s->spell + (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) ? 1 : n - 1)) % n;
    }
    s->cast_cd -= P02_DT;
    if (s->cast_cd > 0.0f) {
        return;
    }
    if (cadv_input_down(in, CADV_ACT_ALT_FIRE)) {
        if (cadv_p02_cast(s, P02_PLAYER, P02_SPELL_CLAIM, eye, fwd)) {
            s->cast_cd = 0.25f;
        }
    } else if (cadv_input_down(in, CADV_ACT_FIRE)) {
        if (cadv_p02_cast(s, P02_PLAYER, s->spell, eye, fwd)) {
            s->cast_cd = s->spell == P02_SPELL_FIREBALL ? 0.35f : 0.6f;
        } else {
            s->cast_cd = 0.4f;
            s->last_events |= P02_EV_DENIED;
            if (s->spell == P02_SPELL_CASTLE) {
                say(s, s->castle[P02_PLAYER].exists ? "YOU ALREADY HAVE A CASTLE" : "NEED FLAT DRY LAND, AWAY FROM VAEL");
            } else if (s->spell == P02_SPELL_RAISE || s->spell == P02_SPELL_LOWER) {
                say(s, s->castle[P02_PLAYER].mana < 5 ? "NOT ENOUGH MANA" : "CASTLE WARDS PROTECT THAT GROUND");
            } else {
                say(s, "NOT ENOUGH MANA");
            }
        }
    }
}

static void player_death(p02_state *s)
{
    p02_castle *c = &s->castle[P02_PLAYER];
    ++s->deaths;
    s->hp = P02_PLAYER_HP;
    if (c->exists) {
        cadv_p02_damage_castle(s, P02_PLAYER, c->mana / 10);
    }
    if (c->exists) {
        cadv_flyer_spawn(&s->player, cadv_v3_add(castle_top(c), cadv_v3(0, 8, 10)), 0.0f);
    } else {
        cadv_flyer_spawn(&s->player, on_surface(s, 34.f, 214.f, 9.0f), 0.0f);
    }
    say(s, "THE CARPET CARRIES YOU HOME...");
}

void cadv_p02_tick(void *state, const cadv_input_frame *in)
{
    static const char *const kill_lines[] = {"Back to the wild with you.", "Sorry, wolf. The aurora needs it.",
                                             "Grandmother would be proud. Mostly.", "Ooh, sparkly."};
    p02_state *s = (p02_state *)state;
    p02_castle *mine = &s->castle[P02_PLAYER];
    int i;
    if (s->outcome != CADV_OUTCOME_RUNNING) {
        return;
    }
    ++s->ticks;
    s->last_events = 0;
    if (s->message_timer > 0) {
        --s->message_timer;
    }
    if (s->quip_timer > 0) {
        --s->quip_timer;
    }
    if (s->shake > 0) {
        --s->shake;
    }
    if (s->hurt_flash > 0) {
        --s->hurt_flash;
    }
    for (i = 0; i < 16; ++i) {
        if (s->flash_ttl[i] > 0) {
            --s->flash_ttl[i];
        }
    }
    for (i = P02_PLAYER; i <= P02_VAEL; ++i) {
        if (s->castle[i].hit_flash > 0) {
            --s->castle[i].hit_flash;
        }
        update_castle_level(s, i);
    }
    if (s->win_timer > 0) {
        if (--s->win_timer == 0 || (s->win_timer < 180 && cadv_input_pressed(in, CADV_ACT_USE))) {
            s->outcome = CADV_OUTCOME_WON;
        }
        return;
    }
    if (s->lose_timer > 0) {
        if (--s->lose_timer == 0) {
            s->outcome = CADV_OUTCOME_LOST;
        }
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_PAUSE)) {
        s->outcome = CADV_OUTCOME_QUIT;
        return;
    }
    cadv_flyer_tick(&s->player, &s->fdef, in, &s->terrain, P02_DT);
    player_controls(s, in);
    tick_proj(s);
    tick_volcano(s);
    tick_creatures(s);
    tick_vael(s);
    spawn_balloons(s, P02_PLAYER);
    spawn_balloons(s, P02_VAEL);
    tick_balloons(s);
    tick_turrets(s, P02_PLAYER);
    tick_turrets(s, P02_VAEL);
    if (s->hurt_cd > 0.0f) {
        s->hurt_cd -= P02_DT;
    } else if (s->ticks % 30 == 0 && s->hp < P02_PLAYER_HP) {
        ++s->hp;
    }
    if (s->hp <= 0) {
        player_death(s);
    }
    /* castle bookkeeping for snowflakes */
    if (mine->exists && mine->level >= 2 && !s->reached_level2) {
        s->reached_level2 = true;
        s->castle_kept = true;
        quip(s, "The castle grows! Turrets, even.");
    }
    if (s->reached_level2 && (!mine->exists || mine->level < 2)) {
        s->castle_kept = false;
    }
    if (s->scroll_active && cadv_v3_dist(s->player.pos, s->scroll_pos) < 6.0f) {
        s->scroll_active = false;
        s->has_volcano = true;
        s->spell = P02_SPELL_VOLCANO;
        say(s, "VOLCANO SCROLL! (6) - AIM IT AT VAEL'S CASTLE");
        sfx(s, CADV_SFX_PICKUP, 1.0f);
    }
    /* the Shining Star */
    if (!s->star_active && !s->star_taken) {
        if (cadv_p02_share(s, P02_PLAYER) >= CADV_P02_WIN_SHARE) {
            s->star_active = true;
            s->star_pos = cadv_v3(128.f, 96.f, 128.f);
            say(s, "THE AURORA SHINES! FLY UP TO CATCH THE STAR");
        } else if (s->castle[P02_VAEL].destroyed) {
            s->star_active = true;
            s->star_pos = cadv_v3_add(s->castle[P02_VAEL].pos, cadv_v3(0, 6, 0));
            say(s, "A STAR RISES FROM VAEL'S RUINS!");
        }
    }
    if (s->star_active) {
        if (s->star_pos.y < 40.0f && s->castle[P02_VAEL].destroyed) {
            s->star_pos.y += 2.0f * P02_DT;
        }
        if (cadv_v3_dist(s->player.pos, s->star_pos) < 6.0f) {
            s->star_active = false;
            s->star_taken = true;
            s->win_timer = 300;
            s->last_events |= P02_EV_STAR;
            say(s, "SHINING STAR! THE AURORA RETURNS");
            sfx(s, CADV_SFX_STAR, 1.0f);
            return;
        }
    }
    if (cadv_p02_share(s, P02_VAEL) >= CADV_P02_WIN_SHARE) {
        s->lose_timer = 180;
        say(s, "VAEL HOARDS THE NORTH'S LIGHT...");
        return;
    }
    advance_phase(s);
    check_marks(s);
    /* sounds and remarks */
    if (s->last_events & P02_EV_CAST) {
        sfx(s, CADV_SFX_SHOT, 0.6f);
    }
    if (s->last_events & (P02_EV_HIT | P02_EV_HURT)) {
        sfx(s, CADV_SFX_HIT, 0.8f);
    }
    if (s->last_events & P02_EV_ERUPT) {
        sfx(s, CADV_SFX_EXPLOSION, 1.0f);
        say(s, "THE MOUNTAIN ROARS!");
    }
    if (s->last_events & P02_EV_BANK) {
        sfx(s, CADV_SFX_PICKUP, 0.5f);
    }
    if (s->last_events & P02_EV_CASTLE) {
        sfx(s, CADV_SFX_BELL, 1.0f);
        quip(s, "A castle of my own. Grandmother had three.");
    }
    if (s->last_events & P02_EV_DENIED) {
        sfx(s, CADV_SFX_DENIED, 0.6f);
    }
    if ((s->last_events & P02_EV_KILL) && cadv_rng_below(&s->rng, 3) == 0) {
        quip(s, kill_lines[cadv_rng_below(&s->rng, 4)]);
    }
}

/* ---- results & scripting ------------------------------------------------------------- */

cadv_portal_result cadv_p02_result(const void *state)
{
    const p02_state *s = (const p02_state *)state;
    cadv_portal_result r;
    memset(&r, 0, sizeof r);
    r.outcome = s->outcome;
    r.star_collected = s->star_taken;
    r.checkpoint = s->checkpoint;
    r.marks_found = s->marks_found;
    r.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        r.snowflakes = 1;
        if (s->reached_level2 && s->castle_kept) {
            r.snowflakes++;
        }
        if (s->ticks < 15 * 3600 && s->cfg.difficulty != CADV_DIFF_STORY) {
            r.snowflakes++;
        }
    }
    return r;
}

static int count_free_orbs(const p02_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < s->norbs; ++i) {
        n += s->orbs[i].state == P02_ORB_FREE;
    }
    return n;
}

static int count_orbs(const p02_state *s, int owner, int state)
{
    int i;
    int n = 0;
    for (i = 0; i < s->norbs; ++i) {
        n += s->orbs[i].state == state && s->orbs[i].owner == owner;
    }
    return n;
}

static int count_creatures(const p02_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        n += s->creatures[i].active;
    }
    return n;
}

bool cadv_p02_query(const void *state, const char *key, double *out)
{
    const p02_state *s = (const p02_state *)state;
    struct kv { const char *k; double v; };
    const struct kv table[] = {
        {"player.x", s->player.pos.x},
        {"player.y", s->player.pos.y},
        {"player.z", s->player.pos.z},
        {"player.yaw", s->player.yaw},
        {"player.health", s->hp},
        {"share", cadv_p02_share(s, P02_PLAYER)},
        {"vael.share", cadv_p02_share(s, P02_VAEL)},
        {"total_mana", cadv_p02_total_mana(s)},
        {"castle.level", s->castle[P02_PLAYER].level},
        {"castle.mana", s->castle[P02_PLAYER].mana},
        {"vael.castle.level", s->castle[P02_VAEL].level},
        {"vael.castle.destroyed", s->castle[P02_VAEL].destroyed},
        {"vael.alive", s->vael.alive},
        {"phase", s->phase},
        {"orbs.free", count_free_orbs(s)},
        {"orbs.mine", count_orbs(s, P02_PLAYER, P02_ORB_FREE)},
        {"orbs.carried", count_orbs(s, P02_PLAYER, P02_ORB_CARRIED)},
        {"balloons", count_balloons(s, P02_PLAYER)},
        {"creatures", count_creatures(s)},
        {"star.active", s->star_active},
        {"star.taken", s->star_taken},
        {"has_volcano", s->has_volcano},
        {"spell", s->spell},
        {"deaths", s->deaths},
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

void cadv_p02_teleport(void *state, float x, float y, float z, float yaw)
{
    p02_state *s = (p02_state *)state;
    s->player.pos = cadv_v3(x, cadv_maxf(y, cadv_terrain_surface(&s->terrain, x, z) + s->fdef.clearance), z);
    s->player.vel = cadv_v3(0, 0, 0);
    s->player.yaw = yaw;
    s->player.pitch = 0.0f;
}

/* ---- completion bot: harvest/defend utility AI ------------------------------------- */

static void bot_aim(const cadv_flyer *f, cadv_vec3 target, cadv_input_frame *out, float *err)
{
    cadv_vec3 eye = cadv_v3_add(f->pos, cadv_v3(0, 0.9f, 0));
    cadv_vec3 to = cadv_v3_sub(target, eye);
    float want_yaw = cadv_atan2f(-to.x, -to.z);
    float flat = cadv_sqrtf(to.x * to.x + to.z * to.z);
    float want_pitch = cadv_atan2f(to.y, flat);
    float dy = cadv_wrap_angle(want_yaw - f->yaw);
    float dp = want_pitch - f->pitch;
    out->look_x = cadv_clampf(dy, -0.12f, 0.12f);
    out->look_y = cadv_clampf(dp, -0.08f, 0.08f);
    *err = cadv_absf(dy) + cadv_absf(dp);
}

static void bot_select(const p02_state *s, int spell, cadv_input_frame *out)
{
    if (s->spell != spell) {
        out->pressed |= CADV_ACT_BIT(CADV_ACT_WEAPON_1 + spell);
        out->held |= CADV_ACT_BIT(CADV_ACT_WEAPON_1 + spell);
    }
}

static void bot_fly(const p02_state *s, cadv_vec3 target, float alt, float stop, cadv_input_frame *out)
{
    float look_x = out->look_x;
    float look_y = out->look_y;
    steer(&s->player, &s->terrain, target, alt, stop, out);
    out->look_y = look_y != 0.0f ? look_y : out->look_y;
    out->look_x = look_x != 0.0f ? look_x : out->look_x;
}

void cadv_p02_bot(const void *state, cadv_input_frame *out)
{
    const p02_state *s = (const p02_state *)state;
    const cadv_flyer *f = &s->player;
    cadv_vec3 eye = cadv_v3_add(f->pos, cadv_v3(0, 0.9f, 0));
    float err = 9.0f;
    int i;
    int threat = -1;
    float td = 26.0f;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0) {
        if (s->win_timer < 170) {
            out->pressed = out->held = CADV_ACT_BIT(CADV_ACT_USE);
        }
        return;
    }
    /* 1. the star */
    if (s->star_active) {
        bot_aim(f, s->star_pos, out, &err);
        bot_fly(s, s->star_pos, 4.0f, 0.0f, out);
        if (s->star_pos.y > f->pos.y + 1.0f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_JUMP);
            out->held &= ~CADV_ACT_BIT(CADV_ACT_CROUCH);
        }
        return;
    }
    /* 2. a castle first */
    if (!s->castle[P02_PLAYER].exists) {
        cadv_vec3 spot = s->home_spot;
        float hd = hdist(f->pos, spot);
        if (hd > 22.0f) {
            bot_fly(s, cadv_v3(spot.x, spot.y + 14.0f, spot.z), 12.0f, 18.0f, out);
        } else {
            bot_select(s, P02_SPELL_CASTLE, out);
            bot_aim(f, spot, out, &err);
            if (err < 0.06f && s->spell == P02_SPELL_CASTLE) {
                out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
            }
            if (hd < 10.0f) {
                out->move_y = -0.6f; /* back off so the plateau is in view */
            }
        }
        return;
    }
    /* 3. threats: Vael close by, then creatures */
    if (s->vael.alive && cadv_v3_dist(s->vael.fly.pos, f->pos) < 40.0f &&
        cadv_terrain_line_clear(&s->terrain, eye, s->vael.fly.pos)) {
        bot_select(s, P02_SPELL_FIREBALL, out);
        bot_aim(f, s->vael.fly.pos, out, &err);
        if (err < 0.08f && s->spell == P02_SPELL_FIREBALL) {
            out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
        }
        out->move_x = (s->ticks / 90) % 2 ? 0.7f : -0.7f; /* dodge */
        return;
    }
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        const p02_creature *c = &s->creatures[i];
        float d;
        if (!c->active) {
            continue;
        }
        d = cadv_v3_dist(c->pos, f->pos);
        if (d < td && cadv_terrain_line_clear(&s->terrain, eye, cadv_v3_add(c->pos, cadv_v3(0, 1.2f, 0)))) {
            td = d;
            threat = i;
        }
    }
    if (threat >= 0) {
        cadv_vec3 c = cadv_v3_add(s->creatures[threat].pos, cadv_v3(0, 0.8f, 0));
        bot_select(s, P02_SPELL_FIREBALL, out);
        bot_aim(f, c, out, &err);
        if (err < 0.07f && s->spell == P02_SPELL_FIREBALL) {
            out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
        }
        if (td < 10.0f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_JUMP); /* climb out of reach */
        }
        return;
    }
    /* 4. the volcano scroll at night, then use it on Vael's castle */
    if (s->scroll_active) {
        bot_aim(f, s->scroll_pos, out, &err);
        bot_fly(s, s->scroll_pos, 3.0f, 0.0f, out);
        if (hdist(f->pos, s->scroll_pos) < 12.0f && f->pos.y > s->scroll_pos.y + 1.0f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_CROUCH);
            out->held &= ~CADV_ACT_BIT(CADV_ACT_JUMP);
        }
        return;
    }
    if (s->has_volcano && s->castle[P02_VAEL].exists && s->castle[P02_PLAYER].mana >= k_spell_cost[P02_SPELL_VOLCANO] &&
        s->volcano_ticks == 0) {
        cadv_vec3 vc = s->castle[P02_VAEL].pos;
        float hd = hdist(f->pos, vc);
        cadv_vec3 aimp = cadv_v3(vc.x + 6.0f, vc.y, vc.z + 6.0f);
        if (hd > 70.0f) {
            bot_fly(s, cadv_v3(vc.x, vc.y + 30.0f, vc.z), 30.0f, 60.0f, out);
        } else {
            bot_select(s, P02_SPELL_VOLCANO, out);
            bot_aim(f, aimp, out, &err);
            if (err < 0.05f && s->spell == P02_SPELL_VOLCANO) {
                out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
            }
            if (hd < 50.0f) {
                out->move_y = -0.8f;
            }
        }
        return;
    }
    /* 5. harvest: the nearest mana that is not ours */
    {
        int o = nearest_orb(s, f->pos, P02_PLAYER, 1e9f);
        if (o >= 0) {
            cadv_vec3 op = cadv_v3_add(s->orbs[o].pos, cadv_v3(0, 0.3f, 0));
            float d = cadv_v3_dist(op, eye);
            if (d < 30.0f && cadv_terrain_line_clear(&s->terrain, eye, op)) {
                bot_aim(f, op, out, &err);
                if (err < 0.05f) {
                    out->held |= CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
                }
                if (d > 16.0f) {
                    out->move_y = 0.5f;
                }
            } else {
                bot_fly(s, op, 10.0f, 14.0f, out);
            }
            return;
        }
    }
    /* 6. nothing free: hunt creatures anywhere for their mana */
    {
        float bd = 1e9f;
        int best = -1;
        for (i = 0; i < P02_MAX_CREATURES; ++i) {
            if (s->creatures[i].active && cadv_v3_dist(s->creatures[i].pos, f->pos) < bd) {
                bd = cadv_v3_dist(s->creatures[i].pos, f->pos);
                best = i;
            }
        }
        if (best >= 0) {
            bot_fly(s, s->creatures[best].pos, 12.0f, 18.0f, out);
        } else if (s->castle[P02_VAEL].exists && !s->vael.alive && s->phase >= P02_PHASE_NIGHT) {
            /* last resort: knock mana loose from Vael's castle while his ward is down */
            cadv_vec3 vc = castle_top(&s->castle[P02_VAEL]);
            if (cadv_v3_dist(vc, f->pos) < 40.0f && cadv_terrain_line_clear(&s->terrain, eye, vc)) {
                bot_select(s, P02_SPELL_FIREBALL, out);
                bot_aim(f, vc, out, &err);
                if (err < 0.06f) {
                    out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
                }
            } else {
                bot_fly(s, vc, 14.0f, 30.0f, out);
            }
        }
    }
}
