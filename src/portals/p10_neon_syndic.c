/* SPDX-License-Identifier: MIT */
/* Day 10 - Neon Syndic: a night in Nivalis. The lead agent and a cell of
 * three AI agents cut Halcyon Corp's cheer-broadcast relays: one in the
 * market among the crowds, one in a factory past a road barrier you ram with
 * a stolen hover-car, one at the end of the Halcyon Tower turret gauntlet,
 * where the antenna casing hides the Shining Star. Then hold the plaza until
 * the VTOL lands. Squad orders on a radial ring, the Charm Beacon that turns
 * civilians (and guards) into a following crowd, overclock sliders that
 * crash when the pool runs dry, alert levels, facades that break in chunks. */
#include "portals/p10_neon_syndic.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "hub/friends.h"
#include "portals/p10_internal.h"

#define CC(c) (((float)(c) + 0.5f) * P10_CELL)

/* streets and plazas (cells, inclusive): x0, z0, x1, z1 */
static const int k_streets[][4] = {
    {2, 56, 12, 60},  /* the alley where the cell starts */
    {8, 44, 12, 60},  /* lane to the market */
    {10, 40, 26, 54}, /* the market plaza */
    {26, 46, 28, 48}, /* passage to the back alley */
    {28, 38, 31, 56}, /* the back alley (graffiti) */
    {10, 32, 33, 39}, /* market north street, the car park */
    {34, 32, 92, 36}, /* the avenue */
    {44, 8, 48, 31},  /* industrial road, barrier across it */
    {49, 11, 51, 13}, /* factory door */
    {60, 37, 64, 48}, /* tower approach */
    {62, 46, 65, 48}, /* tower door */
    {36, 37, 58, 60}, /* the plaza */
};
static const int k_halls[][4] = {
    {52, 4, 86, 20},  /* the factory */
    {66, 44, 90, 50}, /* the Halcyon Tower gauntlet */
};
static const int k_props[][2] = {
    {13, 43}, {16, 43}, {21, 43}, {24, 43}, {13, 51}, {24, 51}, {18, 47},           /* stalls, the relay kiosk last */
    {58, 8},  {62, 8},  {66, 8},  {58, 15}, {62, 15}, {70, 15}, {74, 8}, {82, 8},   /* machines, relay 2 last */
    {89, 47},                                                                         /* relay 3 */
    {40, 44}, {54, 44}, {40, 54}, {54, 50},                                           /* plaza planters */
};
static const char *const k_agent_names[P10_AGENTS] = {"?", "VESPER", "HALO"};
static const float k_guard_dmg[3] = {3.0f, 5.0f, 6.0f};

cadv_vec3 cadv_p10_cell(int cx, int cz) { return cadv_v3(CC(cx), 0.0f, CC(cz)); }
cadv_vec3 cadv_p10_eye(const p10_state *s) { return cadv_player_eye(&s->body); }

/* ---- helpers ---------------------------------------------------------------------- */

static void copy_text(char *out, size_t cap, const char *text) { snprintf(out, cap, "%s", text); }

static void say(p10_state *s, const char *text)
{
    copy_text(s->message, sizeof s->message, text);
    s->message_timer = 240;
}

static void radio(p10_state *s, const char *text)
{
    copy_text(s->radio, sizeof s->radio, text);
    s->radio_timer = 360;
}

static void sfx(p10_state *s, cadv_sfx id, float vol, cadv_vec3 at)
{
    if (s->cfg.audio) {
        float d = cadv_v3_dist(at, s->body.pos);
        cadv_audio_sfx(s->cfg.audio, id, vol * cadv_clampf(1.0f - d / 80.0f, 0.05f, 1.0f), 0.0f);
    }
}

static float flat(cadv_vec3 a, cadv_vec3 b) { return cadv_sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }
static float yaw_to(cadv_vec3 from, cadv_vec3 to) { return cadv_atan2f(-(to.x - from.x), -(to.z - from.z)); }
static float rnd(p10_state *s) { return cadv_rng_float01(&s->rng); }

static bool sees(const p10_state *s, cadv_vec3 a, cadv_vec3 b)
{
    return cadv_world_line_of_sight(&s->world, cadv_v3(a.x, a.y + 1.4f, a.z), cadv_v3(b.x, b.y + 1.4f, b.z));
}

int cadv_p10_agents_alive(const p10_state *s)
{
    int n = 0;
    int i;
    for (i = 0; i < P10_AGENTS; ++i) {
        n += s->ag[i].alive ? 1 : 0;
    }
    return n;
}

bool cadv_p10_hostile(const p10_state *s, int i)
{
    const p10_enemy *e = &s->e[i];
    if (!e->active || e->ally) {
        return false;
    }
    return e->kind != P10_E_GUARD || s->alert >= 40.0f || e->hurt > 0 || s->act >= P10_ACT_PLAZA;
}

int cadv_p10_hostiles(const p10_state *s)
{
    int n = 0;
    int i;
    for (i = 0; i < P10_ENEMIES; ++i) {
        n += cadv_p10_hostile(s, i) ? 1 : 0;
    }
    return n;
}

static void raise_alert(p10_state *s, float amount)
{
    float before = s->alert;
    s->alert = cadv_clampf(s->alert + amount, 0.0f, 100.0f);
    if (before < 80.0f && s->alert >= 80.0f) {
        radio(s, "CHOIR: ALARM IN THE DISTRICT. HALCYON IS SENDING VANS - KEEP MOVING.");
    } else if (before < 40.0f && s->alert >= 40.0f) {
        say(s, "ALERT: SECURITY IS LOOKING FOR YOU");
    }
}

/* ---- the city ----------------------------------------------------------------------- */

static void fill(p10_state *s, const int r[4], uint8_t v)
{
    int x;
    int z;
    for (z = r[1]; z <= r[3]; ++z) {
        for (x = r[0]; x <= r[2]; ++x) {
            s->cells[z * P10_W + x] = v;
        }
    }
}

/* The cells a facade stands on are walkable once its lower chunks are gone. */
static void facade_nav(p10_state *s, int k)
{
    const cadv_facade *f = &s->fac[k];
    float len = f->size * (float)f->w;
    int c0 = (int)((f->axis == 0 ? f->origin.x : f->origin.z) / P10_CELL);
    int c1 = (int)(((f->axis == 0 ? f->origin.x : f->origin.z) + len - 0.01f) / P10_CELL);
    int row = (int)((f->axis == 0 ? f->origin.z + f->depth * 0.5f : f->origin.x + f->depth * 0.5f) / P10_CELL);
    int c;
    for (c = c0; c <= c1; ++c) {
        bool blocked = false;
        int x;
        for (x = 0; x < f->w; ++x) {
            float a = (f->axis == 0 ? f->origin.x : f->origin.z) + ((float)x + 0.5f) * f->size;
            if ((int)(a / P10_CELL) == c && (cadv_facade_alive(f, x, 0) || cadv_facade_alive(f, x, 1))) {
                blocked = true;
            }
        }
        if (f->axis == 0) {
            cadv_nav_set(&s->nav, c, row, !blocked);
        } else {
            cadv_nav_set(&s->nav, row, c, !blocked);
        }
    }
}

static void add_facade(p10_state *s, cadv_vec3 origin, int axis, int cols, int rows, float size, float depth, int hp)
{
    if (s->facades < P10_FACADES && cadv_facade_build(&s->fac[s->facades], &s->world, origin, axis, cols, rows, size, depth, hp)) {
        facade_nav(s, s->facades);
        ++s->facades;
    }
}

static void build_city(p10_state *s)
{
    int x;
    int z;
    int k;
    uint32_t h = 1010u;
    memset(s->cells, 1, sizeof s->cells);
    for (k = 0; k < (int)(sizeof k_streets / sizeof k_streets[0]); ++k) {
        fill(s, k_streets[k], 0);
    }
    for (k = 0; k < (int)(sizeof k_halls / sizeof k_halls[0]); ++k) {
        fill(s, k_halls[k], 3);
    }
    for (k = 0; k < (int)(sizeof k_props / sizeof k_props[0]); ++k) {
        s->cells[k_props[k][1] * P10_W + k_props[k][0]] = 2;
    }
    /* building heights by 6x6 block: towers to the east */
    for (z = 0; z < P10_H; ++z) {
        for (x = 0; x < P10_W; ++x) {
            uint32_t v = h ^ (uint32_t)((x / 6) * 73856093u) ^ (uint32_t)((z / 6) * 19349663u);
            v = v * 2654435761u;
            s->height[z * P10_W + x] = (uint8_t)(10 + (v >> 24) % 18 + (x > 60 ? 10 : 0));
            if (x >= 66 && x <= 90 && z >= 41 && z <= 53) {
                s->height[z * P10_W + x] = 60; /* Halcyon Tower */
            }
        }
    }
    cadv_nav_init(&s->nav, P10_W, P10_H, P10_CELL);
    cadv_world_clear(&s->world);
    cadv_world_add(&s->world, cadv_v3(0, -1.0f, 0), cadv_v3((float)P10_W * P10_CELL, 0.0f, (float)P10_H * P10_CELL), 0); /* the street */
    for (z = 0; z < P10_H; ++z) {
        for (x = 0; x < P10_W;) {
            int x1 = x;
            uint8_t c = s->cells[z * P10_W + x];
            if (c == 0 || c == 3) {
                cadv_nav_set(&s->nav, x, z, true);
                ++x;
                continue;
            }
            while (x1 + 1 < P10_W && s->cells[z * P10_W + x1 + 1] == c) {
                ++x1;
            }
            cadv_world_add(&s->world, cadv_v3((float)x * P10_CELL, 0, (float)z * P10_CELL),
                           cadv_v3((float)(x1 + 1) * P10_CELL, c == 2 ? 1.3f : 8.0f, (float)(z + 1) * P10_CELL), c);
            x = x1 + 1;
        }
    }
    /* the road barrier, a shop front onto the back alley, the factory side
     * wall, and the gauntlet pillars: all break in chunks */
    add_facade(s, cadv_v3(44.0f * P10_CELL, 0, 29.0f * P10_CELL + 0.6f), 0, 8, 3, 1.25f, 0.8f, 160);
    add_facade(s, cadv_v3(26.0f * P10_CELL + 0.6f, 0, 50.0f * P10_CELL), 1, 5, 3, 1.6f, 0.8f, 120);
    add_facade(s, cadv_v3(51.0f * P10_CELL + 0.6f, 0, 15.0f * P10_CELL), 1, 4, 3, 1.5f, 0.8f, 140);
    for (k = 0; k < 4; ++k) {
        static const int pillar[4][2] = {{71, 45}, {71, 49}, {78, 45}, {78, 49}};
        add_facade(s, cadv_v3((float)pillar[k][0] * P10_CELL, 0, (float)pillar[k][1] * P10_CELL + 0.5f), 0, 2, 2, 1.0f, 1.0f, 90);
    }
    add_facade(s, cadv_v3(46.0f * P10_CELL, 0, 47.0f * P10_CELL + 0.5f), 0, 3, 2, 1.0f, 1.0f, 100); /* plaza kiosk */
    cadv_nav_mark_cover(&s->nav);
    s->relay[0] = cadv_v3(CC(18), 0, CC(47));
    s->relay[1] = cadv_v3(CC(82), 0, CC(8));
    s->relay[2] = cadv_v3(CC(89), 0, CC(47));
    s->star_pos = cadv_v3(CC(89) - 1.6f, 1.4f, CC(47));
    s->pad = cadv_v3(CC(47), 0, CC(56));
    /* neon: lamps for the renderer and the light count */
    {
        static const int lamps[][2] = {{11, 56}, {14, 41}, {25, 41}, {14, 53}, {25, 53}, {30, 44}, {20, 33}, {32, 33}, {40, 33}, {52, 35}, {64, 33},
                                       {76, 35}, {88, 33}, {45, 24}, {47, 14}, {60, 6}, {78, 18}, {63, 40}, {68, 47}, {84, 47}, {38, 42}, {56, 42}, {38, 58}, {56, 58}};
        static const float cols[4][3] = {{1.0f, 0.2f, 0.8f}, {0.2f, 0.9f, 1.0f}, {0.6f, 0.3f, 1.0f}, {1.0f, 0.6f, 0.2f}};
        for (k = 0; k < P10_LAMPS; ++k) {
            s->lamp[k] = cadv_v3(CC(lamps[k][0]), 4.5f, CC(lamps[k][1]));
            s->lamp_col[k] = cadv_v3(cols[k % 4][0], cols[k % 4][1], cols[k % 4][2]);
        }
        s->lamps = P10_LAMPS;
    }
}

static void npc_place(cadv_follower *f, int cx, int cz, float speed, float keep) { cadv_follower_init(f, cadv_v3(CC(cx), 0, CC(cz)), speed, keep); }

static int spawn_enemy(p10_state *s, int kind, int cx, int cz)
{
    int i;
    for (i = 0; i < P10_ENEMIES; ++i) {
        p10_enemy *e = &s->e[i];
        if (!e->active) {
            memset(e, 0, sizeof *e);
            e->active = true;
            e->kind = kind;
            e->hp = kind == P10_E_GUARD ? 60 : kind == P10_E_ENFORCER ? 140 : 150;
            npc_place(&e->f, cx, cz, kind == P10_E_ENFORCER ? 3.0f : 3.8f, 0.0f);
            e->f.mode = CADV_FOLLOW_WAIT;
            e->post = e->f.pos;
            e->target = -2;
            e->think = rnd(s) * 2.0f;
            return i;
        }
    }
    return -1;
}

static void civ_place(p10_state *s, int i, int zone)
{
    /* market plaza or the big plaza */
    p10_civ *c = &s->civ[i];
    int cx;
    int cz;
    do {
        cx = zone == 0 ? 10 + (int)(rnd(s) * 17.0f) : 36 + (int)(rnd(s) * 23.0f);
        cz = zone == 0 ? 40 + (int)(rnd(s) * 15.0f) : 40 + (int)(rnd(s) * 21.0f);
    } while (!cadv_nav_walkable(&s->nav, cx, cz));
    memset(c, 0, sizeof *c);
    c->active = true;
    c->hp = 20;
    npc_place(&c->f, cx, cz, 1.6f + rnd(s) * 0.6f, 2.5f + rnd(s) * 4.0f);
    c->f.mode = CADV_FOLLOW_WAIT;
    c->think = rnd(s) * 3.0f;
}

static void stage_enemies(p10_state *s)
{
    int i;
    for (i = 0; i < P10_ENEMIES; ++i) {
        s->e[i].active = false;
    }
    if (s->act == P10_ACT_MARKET) {
        spawn_enemy(s, P10_E_GUARD, 12, 42);
        spawn_enemy(s, P10_E_GUARD, 25, 42);
        spawn_enemy(s, P10_E_GUARD, 18, 52);
        spawn_enemy(s, P10_E_GUARD, 20, 36);
        spawn_enemy(s, P10_E_GUARD, 29, 50);
    }
    if (s->act <= P10_ACT_INDUSTRIAL) {
        spawn_enemy(s, P10_E_GUARD, 46, 26);
        spawn_enemy(s, P10_E_GUARD, 47, 20);
        spawn_enemy(s, P10_E_GUARD, 60, 12);
        spawn_enemy(s, P10_E_GUARD, 70, 6);
        spawn_enemy(s, P10_E_GUARD, 76, 18);
        spawn_enemy(s, P10_E_ENFORCER, 80, 12);
    }
    if (s->act <= P10_ACT_TOWER) {
        spawn_enemy(s, P10_E_TURRET, 74, 44);
        spawn_enemy(s, P10_E_TURRET, 75, 50);
        spawn_enemy(s, P10_E_TURRET, 84, 44);
        spawn_enemy(s, P10_E_TURRET, 84, 50);
        spawn_enemy(s, P10_E_ENFORCER, 87, 45);
        spawn_enemy(s, P10_E_ENFORCER, 87, 49);
    }
}

/* Puts everything where a checkpoint (or the start) needs it. */
static void stage(p10_state *s, int cp)
{
    int i;
    cadv_vec3 at = cadv_p10_cell(4, 58);
    float yaw = -CADV_PI * 0.5f;
    s->act = cp == 0 ? P10_ACT_MARKET : cp == 1 ? P10_ACT_INDUSTRIAL : cp == 2 ? P10_ACT_TOWER : P10_ACT_PLAZA;
    for (i = 0; i < 3; ++i) {
        s->relay_done[i] = cp > i;
    }
    s->car.pos = cadv_v3(CC(31), 0, CC(34));
    s->car.yaw = -CADV_PI * 0.5f;
    if (cp >= 1) {
        at = cadv_v3(CC(18) + 2.0f, 0, CC(47));
        yaw = 0.0f;
    }
    if (cp >= 2) {
        at = cadv_v3(CC(80), 0, CC(10));
        yaw = CADV_PI;
        s->car.pos = cadv_v3(CC(46), 0, CC(22));
        cadv_facade_blast(&s->fac[P10_FAC_BARRIER], &s->world, cadv_v3(CC(46), 1.0f, 59.0f), 9.0f, 255);
        facade_nav(s, P10_FAC_BARRIER);
    }
    if (cp >= 3) {
        at = cadv_v3(CC(86), 0, CC(47));
        yaw = -CADV_PI * 0.5f;
        s->casing_open = true;
        s->star_taken = true;
        s->hold_t = P10_HOLD;
    }
    cadv_player_spawn(&s->body, at, yaw);
    s->hp = 100;
    for (i = 0; i < P10_AGENTS; ++i) {
        p10_agent *a = &s->ag[i];
        cadv_vec3 slot = cadv_formation_slot(at, yaw, i, P10_AGENTS, 1.8f);
        memset(a, 0, sizeof *a);
        a->alive = true;
        a->hp = 100;
        a->order = CADV_ORDER_FOLLOW;
        a->target = -1;
        cadv_follower_init(&a->f, slot, 5.5f, 0.0f);
        cadv_follower_goto(&a->f, slot);
    }
    for (i = 0; i < P10_CIVS; ++i) {
        civ_place(s, i, i < 28 ? 0 : 1);
    }
    stage_enemies(s);
    s->checkpoint = cp;
}

/* ---- init ---------------------------------------------------------------------------- */

bool cadv_p10_init(void *state, const cadv_portal_cfg *cfg)
{
    p10_state *s = (p10_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    cadv_rng_seed(&s->rng, cfg->seed, 10u);
    cadv_friends_default(&fr);
    if (cfg->friends) {
        for (i = 0; i < cfg->friend_count && i < CADV_MAX_FRIENDS; ++i) {
            snprintf(fr.name[i], CADV_FRIEND_NAME, "%s", cfg->friends[i]);
        }
        fr.count = cfg->friend_count < CADV_MAX_FRIENDS ? cfg->friend_count : CADV_MAX_FRIENDS;
    }
    for (i = 0; d && i < d->mark_count && i < 3; ++i) {
        cadv_friends_mark_text(&fr, &d->marks[i], s->mark_text[i], sizeof s->mark_text[i]);
    }
    {
        /* the first agent goes by a friend's codename */
        const char *n = fr.name[3 < fr.count ? 3 : 0];
        int k;
        for (k = 0; n[k] && k + 1 < (int)sizeof s->names[0]; ++k) {
            s->names[0][k] = (char)(n[k] >= 'a' && n[k] <= 'z' ? n[k] - 32 : n[k]);
        }
        s->names[0][k] = '\0';
        copy_text(s->names[1], sizeof s->names[1], k_agent_names[1]);
        copy_text(s->names[2], sizeof s->names[2], k_agent_names[2]);
    }
    build_city(s);
    s->charm.range = 8.0f;
    s->charm.base_limit = 6;
    s->charm.growth = 0.85f;
    s->charm.time = 1.0f;
    cadv_overclock_init(&s->oc, 100.0f, 9.0f, 6.0f, 5.0f);
    s->hacking = -1;
    s->gauss_ammo = 10;
    stage(s, cfg->checkpoint < 0 ? 0 : (cfg->checkpoint > 3 ? 3 : cfg->checkpoint));
    radio(s, s->act == P10_ACT_MARKET ? "CHOIR: THREE RELAYS FEED THE CHEER NETWORK. THE FIRST IS A KIOSK IN THE MARKET. CUT IT."
                                      : "CHOIR: BACK ON THE STREET. KEEP CUTTING.");
    return true;
}

/* ---- damage ---------------------------------------------------------------------------- */

static void hurt_player(p10_state *s, float amount)
{
    if (s->lose_timer > 0 || s->win_timer > 0) {
        return;
    }
    s->hp -= (int)(amount + 0.5f);
    s->hurt_flash = 12;
    s->body.health = s->hp; /* the shared body keeps a copy for the frame */
    if (s->hp <= 0) {
        s->hp = 0;
        s->lose_timer = 180;
        say(s, "THE LEAD AGENT IS DOWN");
    }
}

static void hurt_agent(p10_state *s, int i, float amount)
{
    p10_agent *a = &s->ag[i];
    if (!a->alive) {
        return;
    }
    a->hp -= (int)(amount + 0.5f);
    a->hurt = 10;
    if (a->hp <= 0) {
        char buf[80];
        a->alive = false;
        s->agent_lost = true;
        snprintf(buf, sizeof buf, "AGENT %s IS DOWN", s->names[i]);
        say(s, buf);
    }
}

static void hurt_enemy(p10_state *s, int i, int amount, bool by_us)
{
    p10_enemy *e = &s->e[i];
    if (!e->active) {
        return;
    }
    e->hp -= amount;
    e->hurt = 300;
    if (by_us && !e->ally) {
        raise_alert(s, 4.0f);
    }
    if (e->hp <= 0) {
        e->active = false;
        if (by_us && e->kind == P10_E_GUARD) {
            raise_alert(s, 10.0f);
        }
        sfx(s, e->kind == P10_E_TURRET ? CADV_SFX_EXPLOSION : CADV_SFX_HIT, 0.7f, e->f.pos);
    }
}

static void hurt_civ(p10_state *s, int i, int amount)
{
    p10_civ *c = &s->civ[i];
    c->hp -= amount;
    if (c->hp <= 0 && c->state != P10_CIV_DOWN) {
        c->state = P10_CIV_DOWN;
        c->active = false;
    }
}

/* a target id's position: -1 player, 0..2 agents, 100+ civilians, 200+ enemies */
static bool target_pos(const p10_state *s, int t, cadv_vec3 *out)
{
    if (t == -1) {
        *out = s->body.pos;
        return s->lose_timer == 0;
    }
    if (t >= 0 && t < P10_AGENTS) {
        *out = s->ag[t].f.pos;
        return s->ag[t].alive && !s->car.driven;
    }
    if (t >= 100 && t < 100 + P10_CIVS) {
        *out = s->civ[t - 100].f.pos;
        return s->civ[t - 100].active && s->civ[t - 100].state == P10_CIV_FOLLOW;
    }
    if (t >= 200 && t < 200 + P10_ENEMIES) {
        *out = s->e[t - 200].f.pos;
        return s->e[t - 200].active;
    }
    return false;
}

static void hurt_target(p10_state *s, int t, float amount)
{
    if (t == -1) {
        hurt_player(s, s->car.driven ? amount * 0.4f : amount);
    } else if (t >= 0 && t < P10_AGENTS) {
        hurt_agent(s, t, amount);
    } else if (t >= 100 && t < 100 + P10_CIVS) {
        hurt_civ(s, t - 100, (int)amount);
    } else if (t >= 200) {
        hurt_enemy(s, t - 200, (int)amount, false);
    }
}

static void tracer(p10_state *s, cadv_vec3 a, cadv_vec3 b, int kind)
{
    int k;
    for (k = 0; k < P10_TRACERS; ++k) {
        if (s->tracers[k].ttl <= 0) {
            s->tracers[k].a = a;
            s->tracers[k].b = b;
            s->tracers[k].ttl = kind == 1 ? 8 : 4;
            s->tracers[k].kind = kind;
            return;
        }
    }
}

/* ---- the player's weapons ------------------------------------------------------------------- */

static float enemy_radius(int kind) { return kind == P10_E_TURRET ? 0.9f : 0.5f; }
static cadv_vec3 enemy_centre(const p10_enemy *e) { return cadv_v3(e->f.pos.x, e->f.pos.y + (e->kind == P10_E_TURRET ? 1.4f : 1.1f), e->f.pos.z); }

/* The first enemy hit along the ray (index or -1) and the distance. */
static int ray_enemy(const p10_state *s, cadv_vec3 o, cadv_vec3 d, float range, float *dist, int *fac, int *chunk)
{
    float best = range;
    int hit = -1;
    int i;
    float t;
    *fac = -1;
    if (cadv_world_raycast(&s->world, o, d, best, &t, NULL, chunk)) {
        int k;
        best = t;
        for (k = 0; k < s->facades; ++k) {
            int c;
            for (c = 0; c < s->fac[k].w * s->fac[k].h; ++c) {
                if (s->fac[k].solid[c] == *chunk) {
                    *fac = k;
                }
            }
        }
    }
    for (i = 0; i < P10_ENEMIES; ++i) {
        const p10_enemy *e = &s->e[i];
        if (e->active && !e->ally && cadv_segment_sphere(o, cadv_v3_add(o, cadv_v3_scale(d, best)), enemy_centre(e), enemy_radius(e->kind))) {
            float dd = cadv_v3_dist(o, enemy_centre(e));
            if (dd < best) {
                best = dd;
                hit = i;
                *fac = -1;
            }
        }
    }
    *dist = best;
    return hit;
}

static void blast(p10_state *s, cadv_vec3 at, float radius, int damage, int facade_damage)
{
    int k;
    int i;
    for (k = 0; k < s->facades; ++k) {
        if (cadv_facade_blast(&s->fac[k], &s->world, at, radius, facade_damage) > 0) {
            facade_nav(s, k);
        }
    }
    for (i = 0; i < P10_ENEMIES; ++i) {
        p10_enemy *e = &s->e[i];
        float d = cadv_v3_dist(enemy_centre(e), at);
        if (e->active && !e->ally && d < radius) {
            hurt_enemy(s, i, (int)((float)damage * (1.0f - d / radius)), true);
        }
    }
    sfx(s, CADV_SFX_EXPLOSION, 0.9f, at);
}

static void fire(p10_state *s, bool held)
{
    cadv_vec3 e = cadv_p10_eye(s);
    cadv_vec3 f = cadv_player_forward(&s->body);
    float aim = cadv_overclock_level(&s->oc, CADV_OC_AIM);
    float dist;
    int fac;
    int chunk;
    int hit;
    if (s->weapon == P10_W_MINIGUN) {
        s->spin = held ? cadv_minf(1.0f, s->spin + P10_DT * 3.0f) : cadv_maxf(0.0f, s->spin - P10_DT * 2.0f);
        if (!held || s->spin < 1.0f || s->cd > 0.0f) {
            return;
        }
        {
            float sp = 0.035f * (1.0f - 0.6f * aim);
            f = cadv_v3_norm(cadv_v3(f.x + (rnd(s) * 2.0f - 1.0f) * sp, f.y + (rnd(s) * 2.0f - 1.0f) * sp, f.z + (rnd(s) * 2.0f - 1.0f) * sp));
        }
        s->cd = 0.07f;
        hit = ray_enemy(s, e, f, 60.0f, &dist, &fac, &chunk);
        if (hit >= 0) {
            hurt_enemy(s, hit, 7, true);
        }
        tracer(s, cadv_v3_add(e, cadv_v3(0, -0.2f, 0)), cadv_v3_add(e, cadv_v3_scale(f, dist)), 0);
        if (s->ticks % 3 == 0) {
            sfx(s, CADV_SFX_SHOT, 0.3f, s->body.pos);
        }
        raise_alert(s, 0.15f);
        return;
    }
    s->spin = cadv_maxf(0.0f, s->spin - P10_DT * 2.0f);
    if (!held || s->cd > 0.0f) {
        return;
    }
    if (s->weapon == P10_W_LASER) {
        s->cd = 0.9f * (1.0f - 0.35f * aim);
        hit = ray_enemy(s, e, f, 80.0f, &dist, &fac, &chunk);
        if (hit >= 0) {
            hurt_enemy(s, hit, 45, true);
        } else if (fac >= 0 && cadv_facade_blast(&s->fac[fac], &s->world, cadv_v3_add(e, cadv_v3_scale(f, dist)), 0.8f, 70) > 0) {
            facade_nav(s, fac);
        }
        tracer(s, cadv_v3_add(e, cadv_v3(0.1f, -0.2f, 0)), cadv_v3_add(e, cadv_v3_scale(f, dist)), 1);
        sfx(s, CADV_SFX_SHOT, 0.6f, s->body.pos);
        raise_alert(s, 3.0f);
        return;
    }
    if (s->gauss_ammo <= 0) {
        s->cd = 0.3f;
        say(s, "GAUSS: NO SLUGS LEFT");
        return;
    }
    {
        int k;
        for (k = 0; k < P10_SHOTS; ++k) {
            if (!s->shots[k].active) {
                s->shots[k].active = true;
                s->shots[k].pos = cadv_v3_add(e, cadv_v3_scale(f, 0.6f));
                s->shots[k].vel = cadv_v3_scale(f, 45.0f);
                s->shots[k].fuse = 3.0f;
                break;
            }
        }
    }
    --s->gauss_ammo;
    s->cd = 1.4f;
    sfx(s, CADV_SFX_SHOT, 0.9f, s->body.pos);
    raise_alert(s, 8.0f);
}

static void tick_shots(p10_state *s)
{
    int k;
    for (k = 0; k < P10_SHOTS; ++k) {
        p10_shot *p = &s->shots[k];
        cadv_vec3 d;
        float len;
        float t;
        int i;
        bool boom = false;
        cadv_vec3 at;
        if (!p->active) {
            continue;
        }
        d = cadv_v3_scale(p->vel, P10_DT);
        len = cadv_v3_len(d);
        d = cadv_v3_scale(d, 1.0f / len);
        at = cadv_v3_add(p->pos, cadv_v3_scale(d, len));
        if (cadv_world_raycast(&s->world, p->pos, d, len, &t, NULL, NULL)) {
            at = cadv_v3_add(p->pos, cadv_v3_scale(d, t));
            boom = true;
        }
        for (i = 0; i < P10_ENEMIES && !boom; ++i) {
            const p10_enemy *e = &s->e[i];
            if (e->active && !e->ally && cadv_segment_sphere(p->pos, at, enemy_centre(e), enemy_radius(e->kind) + 0.2f)) {
                boom = true;
            }
        }
        p->pos = at;
        p->fuse -= P10_DT;
        if (boom || p->fuse <= 0.0f || p->pos.y < 0.0f) {
            blast(s, p->pos, 4.0f, 85, 220);
            p->active = false;
        }
    }
}

/* ---- squad, crowd and enemies ------------------------------------------------------------- */

/* nearest hostile an ally at 'from' can see, within range: enemy index or -1 */
static int nearest_hostile(const p10_state *s, cadv_vec3 from, float range)
{
    int best = -1;
    float bd = range;
    int i;
    for (i = 0; i < P10_ENEMIES; ++i) {
        float d;
        if (!cadv_p10_hostile(s, i)) {
            continue;
        }
        d = flat(s->e[i].f.pos, from);
        if (d < bd && sees(s, from, s->e[i].f.pos)) {
            bd = d;
            best = i;
        }
    }
    return best;
}

/* A shot by a friend at enemy i: hits with a chance that falls with range. */
static void ally_shot(p10_state *s, cadv_vec3 from, int i, float acc, int dmg)
{
    const p10_enemy *e = &s->e[i];
    float d = flat(from, e->f.pos);
    if (rnd(s) < cadv_clampf(acc - d * 0.012f, 0.15f, 0.95f)) {
        hurt_enemy(s, i, dmg, true);
    }
    tracer(s, cadv_v3(from.x, from.y + 1.3f, from.z), enemy_centre(e), 0);
}

static void tick_agents(p10_state *s)
{
    float aim = cadv_overclock_level(&s->oc, CADV_OC_AIM);
    int i;
    for (i = 0; i < P10_AGENTS; ++i) {
        p10_agent *a = &s->ag[i];
        int t;
        if (!a->alive) {
            continue;
        }
        if (a->hurt > 0) {
            --a->hurt;
        }
        if (s->car.driven) {
            a->f.pos = s->car.pos; /* riding along */
            continue;
        }
        a->cd = cadv_maxf(0.0f, a->cd - P10_DT);
        /* where to stand */
        if (a->order == CADV_ORDER_FOLLOW) {
            cadv_vec3 slot = cadv_formation_slot(s->body.pos, s->body.yaw, i, P10_AGENTS, 1.8f);
            if (a->f.mode != CADV_FOLLOW_GOTO || flat(a->f.goal, slot) > 1.5f) {
                cadv_follower_goto(&a->f, slot);
            }
        } else if (a->order == CADV_ORDER_ATTACK && a->target >= 0 && s->e[a->target].active && !s->e[a->target].ally) {
            cadv_vec3 tp = s->e[a->target].f.pos;
            if ((flat(a->f.pos, tp) > 14.0f || !sees(s, a->f.pos, tp)) && (s->ticks + i * 11) % 30 == 0) {
                cadv_follower_goto(&a->f, tp);
            } else if (flat(a->f.pos, tp) <= 14.0f && sees(s, a->f.pos, tp)) {
                a->f.mode = CADV_FOLLOW_WAIT;
            }
        } else if (a->order == CADV_ORDER_ATTACK) {
            a->order = CADV_ORDER_FOLLOW; /* the target is gone */
        } else if (a->order == CADV_ORDER_COVER || a->order == CADV_ORDER_HOLD) {
            if (a->f.mode == CADV_FOLLOW_ON) {
                a->f.mode = CADV_FOLLOW_WAIT;
            }
            if (flat(a->f.pos, a->hold) > 0.5f && a->f.mode != CADV_FOLLOW_GOTO) {
                cadv_follower_goto(&a->f, a->hold);
            }
        }
        cadv_follower_tick(&a->f, &s->nav, &s->world, s->body.pos, P10_DT);
        /* shoot: the ordered target first, else whoever is nearest */
        t = a->order == CADV_ORDER_ATTACK && a->target >= 0 && s->e[a->target].active && sees(s, a->f.pos, s->e[a->target].f.pos) ? a->target
                                                                                                                           : nearest_hostile(s, a->f.pos, 26.0f);
        if (t >= 0) {
            a->yaw = yaw_to(a->f.pos, s->e[t].f.pos);
            if (a->cd <= 0.0f) {
                a->cd = 0.25f;
                ally_shot(s, a->f.pos, t, 0.5f + 0.25f * aim + (s->oc.crash_t > 0.0f ? -0.15f : 0.0f), 9);
            }
        } else if (a->f.mode == CADV_FOLLOW_GOTO) {
            a->yaw = yaw_to(a->f.pos, a->f.goal);
        }
    }
}

static void count_followers(p10_state *s)
{
    int n = 0;
    int i;
    for (i = 0; i < P10_CIVS; ++i) {
        n += s->civ[i].active && s->civ[i].state == P10_CIV_FOLLOW ? 1 : 0;
    }
    for (i = 0; i < P10_ENEMIES; ++i) {
        n += s->e[i].active && s->e[i].ally ? 1 : 0;
    }
    s->followers = n;
    if (n > s->max_followers) {
        s->max_followers = n;
    }
}

static void tick_civs(p10_state *s)
{
    int i;
    for (i = 0; i < P10_CIVS; ++i) {
        p10_civ *c = &s->civ[i];
        float d;
        if (!c->active) {
            continue;
        }
        d = flat(c->f.pos, s->body.pos);
        if (c->state != P10_CIV_FOLLOW && s->beacon && !s->car.driven &&
            cadv_charm_tick(&s->charm, &c->charm, d, s->followers, 0, P10_DT)) {
            c->state = P10_CIV_FOLLOW;
            c->f.mode = CADV_FOLLOW_ON;
            ++s->followers;
            sfx(s, CADV_SFX_BELL, 0.3f, c->f.pos);
        } else if (!s->beacon) {
            c->charm = cadv_maxf(0.0f, c->charm - P10_DT);
        }
        if (s->bench) {
            continue;
        }
        c->think -= P10_DT;
        if (c->state == P10_CIV_FOLLOW) {
            if (s->car.driven) {
                c->f.mode = CADV_FOLLOW_WAIT;
            } else if (c->f.mode != CADV_FOLLOW_ON) {
                c->f.mode = CADV_FOLLOW_ON;
            }
        } else if (s->alert >= 80.0f && d < 30.0f) {
            /* panic: away from the shooting */
            if (c->think <= 0.0f) {
                cadv_vec3 away = cadv_v3_add(c->f.pos, cadv_v3_scale(cadv_v3_norm(cadv_v3_sub(c->f.pos, s->body.pos)), 10.0f));
                cadv_nav_cell cell = cadv_nav_cell_at(&s->nav, away.x, away.z);
                c->state = P10_CIV_FLEE;
                c->think = 2.0f;
                if (cadv_nav_walkable(&s->nav, cell.x, cell.z)) {
                    cadv_follower_goto(&c->f, cadv_nav_centre(&s->nav, cell));
                }
            }
        } else if (c->think <= 0.0f) {
            /* stroll to a nearby open cell */
            cadv_nav_cell here = cadv_nav_cell_at(&s->nav, c->f.pos.x, c->f.pos.z);
            int nx = here.x + (int)(rnd(s) * 9.0f) - 4;
            int nz = here.z + (int)(rnd(s) * 9.0f) - 4;
            c->state = P10_CIV_WANDER;
            c->think = 3.0f + rnd(s) * 4.0f;
            if (cadv_nav_walkable(&s->nav, nx, nz)) {
                cadv_nav_cell to = {(int8_t)nx, (int8_t)nz};
                cadv_follower_goto(&c->f, cadv_nav_centre(&s->nav, to));
            }
        }
        if (c->f.mode == CADV_FOLLOW_GOTO || c->f.mode == CADV_FOLLOW_ON) {
            cadv_vec3 before = c->f.pos;
            cadv_follower_tick(&c->f, &s->nav, &s->world, s->body.pos, P10_DT);
            if (flat(before, c->f.pos) > 0.001f) {
                c->yaw = yaw_to(before, c->f.pos);
            }
        }
    }
}

/* the nearest friend an enemy at 'from' can see: target id, or -2 */
static int enemy_pick(const p10_state *s, cadv_vec3 from, float range)
{
    int best = -2;
    float bd = range;
    int i;
    cadv_vec3 p;
    for (i = -1; i < P10_AGENTS; ++i) {
        if (target_pos(s, i, &p) && flat(p, from) < bd && sees(s, from, p)) {
            bd = flat(p, from);
            best = i;
        }
    }
    for (i = 0; i < P10_CIVS; ++i) {
        if (target_pos(s, 100 + i, &p) && flat(p, from) < bd * 0.8f && sees(s, from, p)) {
            bd = flat(p, from) / 0.8f;
            best = 100 + i; /* the crowd soaks up fire */
        }
    }
    for (i = 0; i < P10_ENEMIES; ++i) {
        if (s->e[i].active && s->e[i].ally && flat(s->e[i].f.pos, from) < bd && sees(s, from, s->e[i].f.pos)) {
            bd = flat(s->e[i].f.pos, from);
            best = 200 + i;
        }
    }
    return best;
}

static void tick_enemy(p10_state *s, int i)
{
    p10_enemy *e = &s->e[i];
    float range = e->kind == P10_E_TURRET ? 26.0f : 30.0f;
    cadv_vec3 tp;
    if (e->hurt > 0) {
        --e->hurt;
    }
    e->cd = cadv_maxf(0.0f, e->cd - P10_DT);
    e->think -= P10_DT;
    /* the beacon sways guards too, once the crowd is big enough */
    if (!e->ally && e->kind != P10_E_TURRET && s->beacon && !s->car.driven &&
        cadv_charm_tick(&s->charm, &e->charm, flat(e->f.pos, s->body.pos), s->followers, e->kind == P10_E_GUARD ? 1 : 2, P10_DT)) {
        e->ally = true;
        e->f.mode = CADV_FOLLOW_ON;
        e->f.keep = 4.0f;
        ++s->followers;
        say(s, "A GUARD JOINS YOUR CROWD");
        return;
    }
    if (s->bench) {
        return;
    }
    if (e->ally) {
        int t = nearest_hostile(s, e->f.pos, 24.0f);
        e->f.mode = s->car.driven ? CADV_FOLLOW_WAIT : CADV_FOLLOW_ON;
        cadv_follower_tick(&e->f, &s->nav, &s->world, s->body.pos, P10_DT);
        if (t >= 0 && e->cd <= 0.0f) {
            e->cd = 0.5f;
            e->yaw = yaw_to(e->f.pos, s->e[t].f.pos);
            ally_shot(s, e->f.pos, t, 0.55f, 8);
        }
        return;
    }
    if (!cadv_p10_hostile(s, i)) {
        /* on patrol around the post */
        if (e->think <= 0.0f) {
            cadv_nav_cell c = cadv_nav_cell_at(&s->nav, e->post.x + (rnd(s) * 2.0f - 1.0f) * 6.0f, e->post.z + (rnd(s) * 2.0f - 1.0f) * 6.0f);
            e->think = 4.0f + rnd(s) * 3.0f;
            if (cadv_nav_walkable(&s->nav, c.x, c.z)) {
                cadv_follower_goto(&e->f, cadv_nav_centre(&s->nav, c));
            }
        }
        if (e->f.mode == CADV_FOLLOW_GOTO) {
            cadv_vec3 before = e->f.pos;
            cadv_follower_tick(&e->f, &s->nav, &s->world, s->body.pos, P10_DT);
            e->yaw = yaw_to(before, e->f.goal);
        }
        /* a guard who sees the beacon or a gun at work in the open grows wary */
        return;
    }
    if (e->think <= 0.0f || e->target < -1 || !target_pos(s, e->target, &tp)) {
        e->target = enemy_pick(s, e->f.pos, range);
        e->think = 0.6f;
    }
    if (e->target < -1 || !target_pos(s, e->target, &tp)) {
        /* hunt the player through the streets */
        if (e->kind == P10_E_GUARD && (s->ticks + i * 7) % 60 == 0 && flat(e->f.pos, s->body.pos) < 60.0f) {
            cadv_follower_goto(&e->f, s->body.pos);
        } else if (e->kind == P10_E_ENFORCER && flat(e->f.pos, e->post) > 3.0f && (s->ticks + i * 7) % 60 == 0) {
            cadv_follower_goto(&e->f, e->post); /* enforcers hold their ground */
        }
        if (e->kind != P10_E_TURRET && e->f.mode == CADV_FOLLOW_GOTO) {
            cadv_follower_tick(&e->f, &s->nav, &s->world, s->body.pos, P10_DT);
        }
        return;
    }
    e->yaw = yaw_to(e->f.pos, tp);
    if (e->kind == P10_E_GUARD || (e->kind == P10_E_ENFORCER && flat(e->f.pos, e->post) < 15.0f)) {
        /* keep a fighting distance */
        float d = flat(e->f.pos, tp);
        if (d > 16.0f && (s->ticks + i * 5) % 40 == 0) {
            cadv_follower_goto(&e->f, tp);
        } else if (d < 10.0f) {
            e->f.mode = CADV_FOLLOW_WAIT;
        }
        if (e->f.mode == CADV_FOLLOW_GOTO) {
            cadv_follower_tick(&e->f, &s->nav, &s->world, s->body.pos, P10_DT);
        }
    }
    if (e->cd <= 0.0f && sees(s, e->f.pos, tp)) {
        float d = flat(e->f.pos, tp);
        float speed = e->target == -1 ? cadv_sqrtf(s->body.vel.x * s->body.vel.x + s->body.vel.z * s->body.vel.z) : 0.0f;
        float acc = 0.45f + 0.05f * (float)s->cfg.difficulty - d * 0.012f - speed * 0.03f;
        float dmg = k_guard_dmg[s->cfg.difficulty] * (e->kind == P10_E_ENFORCER ? 0.5f : e->kind == P10_E_TURRET ? 0.8f : 1.0f);
        if (e->target == -1 && cadv_overclock_level(&s->oc, CADV_OC_SPEED) > 0.0f) {
            acc -= 0.1f; /* hard to track a sprinting agent */
        }
        e->cd = e->kind == P10_E_ENFORCER ? 0.25f : e->kind == P10_E_TURRET ? 0.8f : 0.55f;
        if (rnd(s) < cadv_clampf(acc, 0.08f, 0.9f)) {
            hurt_target(s, e->target, dmg);
        }
        tracer(s, enemy_centre(e), cadv_v3(tp.x, tp.y + 1.1f, tp.z), 0);
        if (s->ticks % 2 == 0) {
            sfx(s, CADV_SFX_SHOT, 0.25f, e->f.pos);
        }
    }
}

static void reinforce(p10_state *s)
{
    int hostile = 0;
    int i;
    for (i = 0; i < P10_ENEMIES; ++i) {
        hostile += cadv_p10_hostile(s, i) && flat(s->e[i].f.pos, s->body.pos) < 60.0f ? 1 : 0; /* the fight nearby */
    }
    if (s->act == P10_ACT_PLAZA) {
        /* waves while the VTOL comes in */
        int every = s->cfg.difficulty == CADV_DIFF_STORY ? 11 * 60 : s->cfg.difficulty == CADV_DIFF_HARD ? 9 * 60 : 10 * 60;
        if (--s->reinforce_t <= 0 && hostile < 4 + (int)s->cfg.difficulty && s->hold_t > 0) {
            static const int edge[4][2] = {{37, 38}, {57, 38}, {37, 60}, {57, 60}};
            int k = (int)(rnd(s) * 4.0f) % 4;
            spawn_enemy(s, P10_E_GUARD, edge[k][0], edge[k][1]);
            spawn_enemy(s, P10_E_GUARD, edge[(k + 1) % 4][0], edge[(k + 1) % 4][1]);
            s->reinforce_t = every;
        }
        return;
    }
    if (s->alert >= 80.0f && s->act <= P10_ACT_INDUSTRIAL) {
        if (--s->reinforce_t <= 0 && hostile < 8) {
            /* a van pulls up at the end of the avenue nearer the player */
            int cx = s->body.pos.x < 120.0f ? 36 : 90;
            spawn_enemy(s, P10_E_GUARD, cx, 33);
            spawn_enemy(s, P10_E_GUARD, cx, 35);
            s->reinforce_t = s->cfg.difficulty == CADV_DIFF_HARD ? 18 * 60 : 25 * 60;
            say(s, "A HALCYON VAN UNLOADS ON THE AVENUE");
        }
    }
}

/* ---- the car ---------------------------------------------------------------------------- */

static void drive(p10_state *s, const cadv_input_frame *in)
{
    p10_car *c = &s->car;
    float target = in->move_y * 22.0f;
    cadv_move_result r;
    cadv_vec3 dir;
    int k;
    c->yaw = cadv_wrap_angle(c->yaw - in->move_x * 1.6f * P10_DT * cadv_clampf(cadv_absf(c->speed) / 6.0f, 0.0f, 1.0f) * (c->speed < 0.0f ? -1.0f : 1.0f) + in->look_x * 0.5f);
    c->speed += cadv_clampf(target - c->speed, -16.0f * P10_DT, 9.0f * P10_DT);
    dir = cadv_v3(-cadv_sinf(c->yaw), 0, -cadv_cosf(c->yaw));
    /* ramming: a fast car through a facade breaks it */
    if (cadv_absf(c->speed) > 7.0f) {
        cadv_vec3 nose = cadv_v3_add(c->pos, cadv_v3_scale(dir, 2.6f * (c->speed > 0.0f ? 1.0f : -1.0f)));
        for (k = 0; k < s->facades; ++k) {
            cadv_aabb probe = cadv_aabb_make(cadv_v3(nose.x - 1.2f, 0.2f, nose.z - 1.2f), cadv_v3(nose.x + 1.2f, 1.5f, nose.z + 1.2f));
            int ch;
            bool touch = false;
            for (ch = 0; ch < s->fac[k].w * s->fac[k].h; ++ch) {
                if (s->fac[k].hp[ch] > 0 && cadv_aabb_overlap(s->world.s[s->fac[k].solid[ch]].box, probe)) {
                    touch = true;
                }
            }
            if (touch) {
                cadv_facade_blast(&s->fac[k], &s->world, cadv_v3(nose.x, 1.0f, nose.z), 3.2f, 255);
                facade_nav(s, k);
                c->speed *= 0.55f;
                raise_alert(s, 15.0f);
                sfx(s, CADV_SFX_EXPLOSION, 1.0f, nose);
                say(s, "CRUNCH! THROUGH THE BARRIER");
            }
        }
    }
    r = cadv_world_move(&s->world, &c->pos, 1.3f, 1.4f, cadv_v3_scale(dir, c->speed * P10_DT), 0.0f);
    if (r.hit_x || r.hit_z) {
        c->speed *= -0.25f;
    }
    c->pos.y = 0.0f;
    s->body.pos = c->pos;
    s->body.vel = cadv_v3_scale(dir, c->speed);
    s->body.yaw = cadv_wrap_angle(s->body.yaw + in->look_x * 0.5f);
    s->body.pitch = cadv_clampf(s->body.pitch + in->look_y, -1.2f, 1.2f);
}

static void leave_car(p10_state *s)
{
    int i;
    p10_car *c = &s->car;
    cadv_vec3 side = cadv_v3(c->pos.x - cadv_cosf(c->yaw) * 2.2f, 0, c->pos.z + cadv_sinf(c->yaw) * 2.2f);
    cadv_nav_cell cell = cadv_nav_cell_at(&s->nav, side.x, side.z);
    if (!cadv_nav_walkable(&s->nav, cell.x, cell.z)) {
        side = cadv_v3(c->pos.x + cadv_cosf(c->yaw) * 2.2f, 0, c->pos.z - cadv_sinf(c->yaw) * 2.2f);
    }
    c->driven = false;
    c->speed = 0.0f;
    s->body.pos = side;
    s->body.vel = cadv_v3(0, 0, 0);
    for (i = 0; i < P10_AGENTS; ++i) {
        s->ag[i].f.pos = cadv_v3(side.x + (float)(i - 1) * 0.9f, 0, side.z + 0.9f);
        s->ag[i].f.path_len = 0;
    }
}

/* ---- using things -------------------------------------------------------------------------- */

static void finish_relay(p10_state *s, int r)
{
    int i;
    /* the district behind you loses track of you */
    for (i = 0; i < P10_ENEMIES; ++i) {
        p10_enemy *e = &s->e[i];
        if (e->active && !e->ally && e->kind == P10_E_GUARD && flat(e->f.pos, s->body.pos) > 45.0f) {
            e->active = false;
        }
    }
    s->relay_done[r] = true;
    s->hacking = -1;
    s->gauss_ammo += 5;
    raise_alert(s, 25.0f);
    s->hp = cadv_mini(100, s->hp + 40);
    for (i = 0; i < P10_AGENTS; ++i) {
        if (s->ag[i].alive) {
            s->ag[i].hp = cadv_mini(100, s->ag[i].hp + 40);
        }
    }
    sfx(s, CADV_SFX_SOCKET, 1.0f, s->relay[r]);
    if (r == 0) {
        s->act = P10_ACT_INDUSTRIAL;
        s->checkpoint = 1;
        say(s, "RELAY 1 CUT");
        radio(s, "CHOIR: THE SECOND RELAY IS IN THE FACTORY UP NORTH. THERE'S A HOVER-CAR AT THE MARKET'S EAST END - BORROW IT.");
    } else if (r == 1) {
        s->act = P10_ACT_TOWER;
        s->checkpoint = 2;
        say(s, "RELAY 2 CUT");
        radio(s, "CHOIR: THE LAST ONE IS IN THE HALCYON TOWER. THEIR TURRETS COVER THE HALL. USE THE PILLARS.");
    } else {
        s->act = P10_ACT_STAR;
        s->casing_open = true;
        say(s, "RELAY 3 CUT - THE ANTENNA CASING SWINGS OPEN");
        radio(s, "CHOIR: THE NETWORK IS DARK. WHAT IS THAT GLOW IN THE ANTENNA?");
    }
}

static void use(p10_state *s)
{
    int r;
    if (s->car.driven) {
        if (cadv_absf(s->car.speed) < 3.0f) {
            leave_car(s);
        }
        return;
    }
    if (s->hacking >= 0) {
        int got = cadv_hack_press(&s->hack);
        if (got < 0) {
            raise_alert(s, 18.0f);
            say(s, "HACK SLIPPED - SECURITY NOTICED");
            sfx(s, CADV_SFX_DENIED, 0.8f, s->body.pos);
        } else {
            sfx(s, CADV_SFX_PICKUP, 0.6f, s->body.pos);
            if (s->hack.done) {
                finish_relay(s, s->hacking);
            }
        }
        return;
    }
    for (r = 0; r < 3; ++r) {
        bool turn = (r == 0 && s->act == P10_ACT_MARKET) || (r == 1 && s->act == P10_ACT_INDUSTRIAL) || (r == 2 && s->act == P10_ACT_TOWER);
        if (!s->relay_done[r] && flat(s->body.pos, s->relay[r]) < 2.8f) {
            if (!turn) {
                say(s, "THIS RELAY IS PROTECTED UNTIL THE OTHERS ARE CUT");
                return;
            }
            if (r == 2 && cadv_p10_hostiles(s) > 0 && flat(s->body.pos, s->relay[2]) < 2.8f) {
                int i;
                bool turrets = false;
                for (i = 0; i < P10_ENEMIES; ++i) {
                    turrets = turrets || (s->e[i].active && s->e[i].kind == P10_E_TURRET);
                }
                if (turrets) {
                    say(s, "THE TURRETS LOCK THE RELAY - DESTROY THEM");
                    return;
                }
            }
            s->hacking = r;
            cadv_hack_start(&s->hack, 3, 2.4f + 0.5f * (float)s->cfg.difficulty, 0.5f - 0.08f * (float)s->cfg.difficulty, (unsigned)(s->ticks + 7));
            say(s, "HACKING: PRESS USE WHEN THE MARKER CROSSES THE GAP");
            return;
        }
    }
    if (flat(s->body.pos, s->car.pos) < 3.2f && s->act <= P10_ACT_TOWER) {
        int i;
        s->car.driven = true;
        for (i = 0; i < P10_AGENTS; ++i) {
            if (s->ag[i].alive) {
                s->ag[i].order = CADV_ORDER_FOLLOW;
            }
        }
        say(s, "YOU TAKE THE HOVER-CAR - THE CELL PILES IN");
        sfx(s, CADV_SFX_DOOR, 0.8f, s->car.pos);
        return;
    }
    say(s, "NOTHING TO USE HERE");
}

/* Orders for the cell: 0 follow, 1 attack, 2 hold, 3 cover. */
static void order(p10_state *s, int o)
{
    static const int map[4] = {CADV_ORDER_FOLLOW, CADV_ORDER_ATTACK, CADV_ORDER_HOLD, CADV_ORDER_COVER};
    int i;
    int target = -1;
    char buf[64];
    if (o == 1) {
        /* whatever is under the crosshair, else the nearest hostile */
        cadv_vec3 e = cadv_p10_eye(s);
        cadv_vec3 f = cadv_player_forward(&s->body);
        float best = 0.995f;
        for (i = 0; i < P10_ENEMIES; ++i) {
            if (cadv_p10_hostile(s, i)) {
                float d = cadv_v3_dot(cadv_v3_norm(cadv_v3_sub(enemy_centre(&s->e[i]), e)), f);
                if (d > best) {
                    best = d;
                    target = i;
                }
            }
        }
        if (target < 0) {
            target = nearest_hostile(s, s->body.pos, 40.0f);
        }
        if (target < 0) {
            say(s, "NO TARGET FOR THE CELL");
            return;
        }
    }
    for (i = 0; i < P10_AGENTS; ++i) {
        p10_agent *a = &s->ag[i];
        if (!a->alive) {
            continue;
        }
        a->order = map[o];
        a->target = target;
        a->hold = a->f.pos;
        if (map[o] == CADV_ORDER_COVER) {
            cadv_nav_cell c;
            if (cadv_nav_nearest_cover(&s->nav, cadv_nav_cell_at(&s->nav, a->f.pos.x, a->f.pos.z), 5, &c)) {
                a->hold = cadv_nav_centre(&s->nav, c);
            }
        }
        if (map[o] == CADV_ORDER_HOLD || map[o] == CADV_ORDER_COVER) {
            cadv_follower_goto(&a->f, a->hold);
        }
    }
    snprintf(buf, sizeof buf, "CELL: %s", cadv_order_name((cadv_order)map[o]));
    say(s, buf);
}

/* ---- marks ---------------------------------------------------------------------------- */

static void check_marks(p10_state *s)
{
    char buf[160];
    if (!(s->marks_found & 1u) && flat(s->body.pos, cadv_v3(CC(18), 0, CC(39))) < 12.0f) {
        s->marks_found |= 1u;
        snprintf(buf, sizeof buf, "A NEON AD: \"%s - UPGRADE NOW!\"", s->mark_text[0]);
        say(s, buf);
    }
    if (!(s->marks_found & 2u) && s->oc.on[CADV_OC_AWARE] && s->ag[0].alive) {
        s->marks_found |= 2u;
        snprintf(buf, sizeof buf, "%s: \"I SEE THEM THROUGH THE WALLS, BOSS.\"", s->mark_text[1]);
        radio(s, buf);
    }
    if (!(s->marks_found & 4u) && flat(s->body.pos, cadv_v3(CC(29), 0, CC(53))) < 5.0f) {
        s->marks_found |= 4u;
        snprintf(buf, sizeof buf, "GRAFFITI: \"%s\"", s->mark_text[2]);
        say(s, buf);
    }
}

/* ---- tick ------------------------------------------------------------------------------ */

static void tick_acts(p10_state *s)
{
    if (s->act == P10_ACT_STAR && !s->star_taken && flat(s->body.pos, s->star_pos) < 1.8f) {
        int k;
        s->star_taken = true;
        s->act = P10_ACT_PLAZA;
        s->hp = 100; /* the star's glow patches up the whole cell */
        for (k = 0; k < P10_AGENTS; ++k) {
            if (s->ag[k].alive) {
                s->ag[k].hp = 100;
            }
        }
        s->checkpoint = 3;
        s->hold_t = P10_HOLD;
        s->reinforce_t = 120;
        say(s, "THE SHINING STAR OF NIVALIS!");
        radio(s, "CHOIR: VTOL INBOUND TO THE PLAZA. ONE MINUTE. HOLD THE PAD.");
        sfx(s, CADV_SFX_STAR, 1.0f, s->body.pos);
    }
    if (s->act == P10_ACT_PLAZA) {
        if (s->hold_t > 0 && flat(s->body.pos, s->pad) < 20.0f) {
            if (--s->hold_t == 0) {
                s->vtol_down = true;
                radio(s, "CHOIR: WHEELS DOWN. EVERYONE ON THE PAD.");
                sfx(s, CADV_SFX_BELL, 1.0f, s->pad);
            }
        }
        if (s->vtol_down && flat(s->body.pos, s->pad) < 3.5f && s->win_timer == 0) {
            s->win_timer = 240;
            s->act = P10_ACT_DONE;
            say(s, "EXTRACTED - THE CHOIR SINGS TONIGHT");
        }
    }
}

void cadv_p10_tick(void *state, const cadv_input_frame *in)
{
    p10_state *s = (p10_state *)state;
    cadv_input_frame body_in;
    int i;
    if (s->outcome != CADV_OUTCOME_RUNNING) {
        return;
    }
    ++s->ticks;
    if (s->message_timer > 0) {
        --s->message_timer;
    }
    if (s->radio_timer > 0) {
        --s->radio_timer;
    }
    if (s->hurt_flash > 0) {
        --s->hurt_flash;
    }
    for (i = 0; i < P10_TRACERS; ++i) {
        if (s->tracers[i].ttl > 0) {
            --s->tracers[i].ttl;
        }
    }
    if (s->win_timer > 0) {
        if (--s->win_timer == 0) {
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
    /* overclock toggles: shift speed, C aim, TAB awareness */
    if (cadv_input_pressed(in, CADV_ACT_RUN) && !cadv_overclock_set(&s->oc, CADV_OC_SPEED, !s->oc.on[CADV_OC_SPEED])) {
        say(s, "CRASHED - THE SLIDERS WON'T MOVE");
    }
    if (cadv_input_pressed(in, CADV_ACT_CROUCH)) {
        cadv_overclock_set(&s->oc, CADV_OC_AIM, !s->oc.on[CADV_OC_AIM]);
    }
    if (cadv_input_pressed(in, CADV_ACT_MAP)) {
        cadv_overclock_set(&s->oc, CADV_OC_AWARE, !s->oc.on[CADV_OC_AWARE]);
    }
    if (cadv_overclock_tick(&s->oc, P10_DT)) {
        say(s, "OVERCLOCK CRASH - YOUR LEGS ARE LEAD");
    }
    /* the radial ring for orders, and the quick keys */
    body_in = *in;
    if (cadv_input_pressed(in, CADV_ACT_RADIAL)) {
        cadv_ring_open(&s->ring, 4);
    }
    if (s->ring.open) {
        cadv_ring_look(&s->ring, in->look_x * 4.0f, in->look_y * 4.0f);
        body_in.look_x = 0.0f;
        body_in.look_y = 0.0f;
        if (!cadv_input_down(in, CADV_ACT_RADIAL)) {
            int o = cadv_ring_close(&s->ring);
            if (o >= 0) {
                order(s, o);
            }
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_WEAPON_4)) {
        order(s, 0);
    }
    if (cadv_input_pressed(in, CADV_ACT_WEAPON_5)) {
        order(s, 2);
    }
    if (cadv_input_pressed(in, CADV_ACT_WEAPON_6)) {
        order(s, 1);
    }
    for (i = 0; i < P10_WEAPONS; ++i) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + i))) {
            s->weapon = i;
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON)) {
        s->weapon = (s->weapon + 1) % P10_WEAPONS;
    }
    if (cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        s->weapon = (s->weapon + P10_WEAPONS - 1) % P10_WEAPONS;
    }
    if (cadv_input_pressed(in, CADV_ACT_USE)) {
        use(s);
    }
    s->cd = cadv_maxf(0.0f, s->cd - P10_DT);
    s->beacon = cadv_input_down(in, CADV_ACT_ALT_FIRE) && !s->car.driven && s->hacking < 0;
    if (s->car.driven) {
        drive(s, in);
    } else if (s->hacking >= 0) {
        cadv_hack_tick(&s->hack, P10_DT);
        s->body.yaw = cadv_wrap_angle(s->body.yaw + body_in.look_x);
        if (cadv_input_pressed(in, CADV_ACT_JUMP)) {
            s->hacking = -1;
            say(s, "HACK ABANDONED");
        }
    } else {
        /* the body walks; shift and C are the sliders here, not run and crouch */
        float level = cadv_overclock_level(&s->oc, CADV_OC_SPEED);
        body_in.held &= ~(CADV_ACT_BIT(CADV_ACT_RUN) | CADV_ACT_BIT(CADV_ACT_CROUCH));
        s->body.tune.walk_speed = 5.0f * (1.0f + 0.6f * level);
        cadv_player_tick(&s->body, &s->world, &body_in, P10_DT);
        if (!s->beacon) {
            fire(s, cadv_input_down(in, CADV_ACT_FIRE));
        }
    }
    tick_shots(s);
    tick_agents(s);
    tick_civs(s);
    for (i = 0; i < P10_ENEMIES; ++i) {
        if (s->e[i].active) {
            tick_enemy(s, i);
        }
    }
    count_followers(s);
    if (!s->bench) {
        reinforce(s);
    }
    /* the alert cools down when nothing is happening */
    s->alert = cadv_maxf(0.0f, s->alert - P10_DT * 1.5f);
    /* patch up between fights */
    if (s->hurt_flash == 0 && s->ticks % 30 == 0 && s->hp < 100 && s->hp > 0) {
        s->hp += 1;
    }
    check_marks(s);
    tick_acts(s);
}

/* ---- results & scripting -------------------------------------------------------------- */

cadv_portal_result cadv_p10_result(const void *state)
{
    const p10_state *s = (const p10_state *)state;
    cadv_portal_result res;
    memset(&res, 0, sizeof res);
    res.outcome = s->outcome;
    res.star_collected = s->star_taken && s->outcome == CADV_OUTCOME_WON;
    res.checkpoint = s->checkpoint;
    res.marks_found = s->marks_found;
    res.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        res.snowflakes = 1 + (s->agent_lost ? 0 : 1) + (s->max_followers >= 30 ? 1 : 0);
    }
    return res;
}

bool cadv_p10_query(const void *state, const char *key, double *out)
{
    const p10_state *s = (const p10_state *)state;
    cadv_vec3 e = cadv_p10_eye(s);
    struct kv {
        const char *k;
        double v;
    };
    const struct kv table[] = {
        {"player.x", e.x},
        {"player.y", e.y},
        {"player.z", e.z},
        {"player.yaw", s->body.yaw},
        {"player.health", s->hp},
        {"act", s->act},
        {"relays", (s->relay_done[0] ? 1 : 0) + (s->relay_done[1] ? 1 : 0) + (s->relay_done[2] ? 1 : 0)},
        {"hacking", s->hacking},
        {"alert", s->alert},
        {"followers", s->followers},
        {"followers.max", s->max_followers},
        {"agents", cadv_p10_agents_alive(s)},
        {"order", s->ag[0].order},
        {"driving", s->car.driven},
        {"car.speed", s->car.speed},
        {"barrier", s->fac[P10_FAC_BARRIER].alive},
        {"hostiles", cadv_p10_hostiles(s)},
        {"weapon", s->weapon},
        {"gauss", s->gauss_ammo},
        {"overclock", cadv_overclock_active(&s->oc)},
        {"stamina", s->oc.stamina},
        {"crashed", cadv_overclock_crashed(&s->oc)},
        {"hold", s->hold_t / 60},
        {"vtol", s->vtol_down},
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

void cadv_p10_teleport(void *state, float x, float y, float z, float yaw)
{
    p10_state *s = (p10_state *)state;
    (void)y;
    s->body.pos = cadv_v3(x, 0.0f, z);
    s->body.vel = cadv_v3(0, 0, 0);
    s->body.yaw = yaw;
    s->body.pitch = 0.0f;
    if (s->car.driven) {
        s->car.pos = s->body.pos;
    }
}

/* ---- completion bot ---------------------------------------------------------------------- */

static void bot_press(cadv_input_frame *out, cadv_action a)
{
    out->pressed |= CADV_ACT_BIT(a);
    out->held |= CADV_ACT_BIT(a);
}

static float bot_walk(p10_state *s, cadv_vec3 goal, cadv_input_frame *out, bool look)
{
    cadv_nav_cell here = cadv_nav_cell_at(&s->nav, s->body.pos.x, s->body.pos.z);
    cadv_nav_cell g = cadv_nav_cell_at(&s->nav, goal.x, goal.z);
    cadv_vec3 step = goal;
    float d = flat(s->body.pos, goal);
    if (!cadv_nav_walkable(&s->nav, here.x, here.z)) {
        int r;
        int dx;
        int dz;
        bool found = false;
        for (r = 1; r <= 2 && !found; ++r) {
            for (dz = -r; dz <= r && !found; ++dz) {
                for (dx = -r; dx <= r && !found; ++dx) {
                    if (cadv_nav_walkable(&s->nav, here.x + dx, here.z + dz)) {
                        here.x = (int8_t)(here.x + dx);
                        here.z = (int8_t)(here.z + dz);
                        found = true;
                    }
                }
            }
        }
        goal = cadv_nav_centre(&s->nav, here);
        g = here;
        s->bot_path_len = 0;
    }
    if (g.x != s->bot_goal_x || g.z != s->bot_goal_z || s->bot_path_len == 0 || s->ticks % 50 == 0) {
        int n = cadv_nav_path(&s->nav, here, g, s->bot_path, 128);
        s->bot_path_len = n > 0 ? n : 0;
        s->bot_path_pos = 0;
        s->bot_goal_x = g.x;
        s->bot_goal_z = g.z;
    }
    while (s->bot_path_pos < s->bot_path_len) {
        cadv_vec3 c = cadv_nav_centre(&s->nav, s->bot_path[s->bot_path_pos]);
        if (flat(c, s->body.pos) < 0.8f && s->bot_path_pos + 1 < s->bot_path_len) {
            ++s->bot_path_pos;
        } else {
            step = s->bot_path_pos + 1 < s->bot_path_len ? c : goal;
            break;
        }
    }
    if (d > 0.35f) {
        float y = s->body.yaw;
        float sx = step.x - s->body.pos.x;
        float sz = step.z - s->body.pos.z;
        float sl = cadv_sqrtf(sx * sx + sz * sz) + 1e-5f;
        float sp = cadv_minf(1.0f, d * 0.8f + 0.2f);
        out->move_y = (sx * -cadv_sinf(y) + sz * -cadv_cosf(y)) / sl * sp;
        out->move_x = (sx * cadv_cosf(y) + sz * -cadv_sinf(y)) / sl * sp;
        if (look) {
            out->look_x = cadv_clampf(cadv_wrap_angle(yaw_to(s->body.pos, step) - y), -0.15f, 0.15f);
            out->look_y = cadv_clampf(-s->body.pitch, -0.05f, 0.05f);
        }
        if (flat(s->body.pos, s->bot_last) < 0.01f) {
            if (++s->bot_stuck > 40) {
                bot_press(out, CADV_ACT_JUMP);
                s->bot_path_len = 0;
                s->bot_stuck = 0;
            }
        } else {
            s->bot_stuck = 0;
        }
    }
    s->bot_last = s->body.pos;
    return d;
}

/* Turns on the nearest visible hostile and shoots; true while fighting. */
static bool bot_fight(p10_state *s, cadv_input_frame *out, float range)
{
    int t = nearest_hostile(s, s->body.pos, range);
    cadv_vec3 e = cadv_p10_eye(s);
    int want;
    /* stay on the current target while it is in sight */
    if (s->bot_target >= 0 && s->bot_push == 0 && cadv_p10_hostile(s, s->bot_target) && flat(s->e[s->bot_target].f.pos, s->body.pos) < range &&
        sees(s, s->body.pos, s->e[s->bot_target].f.pos)) {
        t = s->bot_target;
    }
    cadv_vec3 c;
    float dy;
    float dp;
    if (t < 0) {
        s->bot_target = -1;
        return false;
    }
    /* a target that will not take damage (a pillar in the way): move on */
    if (t != s->bot_target || s->e[t].hp != s->bot_target_hp) {
        s->bot_target = t;
        s->bot_target_hp = s->e[t].hp;
        s->bot_since_hit = 0;
    } else if (++s->bot_since_hit > 120) {
        s->bot_push = 90;
        s->bot_since_hit = 0;
    }
    c = enemy_centre(&s->e[t]);
    dy = cadv_wrap_angle(yaw_to(e, c) - s->body.yaw);
    dp = cadv_atan2f(c.y - e.y, flat(e, c)) - s->body.pitch;
    out->look_x = cadv_clampf(dy, -0.18f, 0.18f);
    out->look_y = cadv_clampf(dp, -0.1f, 0.1f);
    /* the laser for turrets, gauss slugs for enforcers, the minigun for guards */
    want = s->e[t].kind == P10_E_TURRET ? P10_W_LASER : (s->e[t].kind == P10_E_ENFORCER && s->gauss_ammo > 0 && flat(s->e[t].f.pos, s->body.pos) > 6.0f) ? P10_W_GAUSS : P10_W_MINIGUN;
    if (s->weapon != want) {
        bot_press(out, (cadv_action)(CADV_ACT_WEAPON_1 + want));
    }
    if (s->weapon == P10_W_MINIGUN || (cadv_absf(dy) < 0.03f && cadv_absf(dp) < 0.03f)) {
        out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
    }
    if (s->e[t].kind == P10_E_TURRET && s->ag[0].order != CADV_ORDER_ATTACK && cadv_absf(dy) < 0.05f && s->ticks % 30 == 0) {
        bot_press(out, CADV_ACT_WEAPON_6); /* the cell: on that turret */
    }
    return true;
}

static void bot_drive(p10_state *s, cadv_vec3 goal, cadv_input_frame *out)
{
    p10_car *c = &s->car;
    float want = yaw_to(c->pos, goal);
    float dy = cadv_wrap_angle(want - c->yaw);
    float d = flat(c->pos, goal);
    out->move_x = cadv_clampf(-dy * 2.0f, -1.0f, 1.0f);
    out->move_y = d > 8.0f ? (cadv_absf(dy) > 0.8f ? 0.35f : 1.0f) : cadv_clampf(d / 8.0f - c->speed / 20.0f, -1.0f, 1.0f);
    if (cadv_absf(c->speed) < 0.5f && d > 5.0f) {
        if (++s->bot_stuck > 90) {
            out->move_y = -1.0f; /* back off and try again */
            if (s->bot_stuck > 150) {
                s->bot_stuck = 0;
            }
        }
    } else if (s->bot_stuck < 90) {
        s->bot_stuck = 0;
    }
}

void cadv_p10_bot(const void *state, cadv_input_frame *out)
{
    p10_state *s = (p10_state *)(uintptr_t)state; /* the bot keeps its path in the state */
    cadv_vec3 goal;
    bool fighting;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0 || s->lose_timer > 0) {
        return;
    }
    if (s->hacking >= 0) {
        /* no sliders while hacking; tap when the marker is in the gap */
        if (s->oc.on[CADV_OC_AIM] && s->ticks % 2 == 0) {
            bot_press(out, CADV_ACT_CROUCH);
            return;
        }
        if (cadv_absf(cadv_wrap_angle(s->hack.angle - s->hack.window)) < s->hack.width * 0.5f) {
            bot_press(out, CADV_ACT_USE);
        }
        return;
    }
    if (s->car.driven) {
        /* down the avenue, up the industrial road through the barrier, park at the factory */
        static const float wps[3][2] = {{93.0f, 68.0f}, {93.0f, 40.0f}, {93.0f, 25.0f}};
        cadv_vec3 wp = cadv_v3(wps[s->bot_wp][0], 0, wps[s->bot_wp][1]);
        if (s->act != P10_ACT_INDUSTRIAL) {
            if (cadv_absf(s->car.speed) < 2.0f) {
                bot_press(out, CADV_ACT_USE);
            } else {
                out->move_y = -1.0f;
            }
            return;
        }
        if (flat(s->car.pos, wp) < 4.0f && s->bot_wp < 2) {
            ++s->bot_wp;
        }
        if (s->bot_wp == 2 && flat(s->car.pos, wp) < 4.0f) {
            out->move_y = -cadv_clampf(s->car.speed / 10.0f, 0.0f, 1.0f);
            if (cadv_absf(s->car.speed) < 2.0f) {
                bot_press(out, CADV_ACT_USE);
            }
            return;
        }
        bot_drive(s, wp, out);
        return;
    }
    switch (s->act) {
    case P10_ACT_MARKET:
        goal = cadv_v3(s->relay[0].x + 1.6f, 0, s->relay[0].z);
        break;
    case P10_ACT_INDUSTRIAL:
        if (s->body.pos.z < 58.0f) { /* north of the barrier: on foot to the factory */
            goal = cadv_v3(s->relay[1].x, 0, s->relay[1].z + 1.6f);
        } else {
            goal = s->car.pos;
            s->bot_wp = 0;
            if (flat(s->body.pos, s->car.pos) < 2.8f) {
                bot_press(out, CADV_ACT_USE);
                return;
            }
        }
        break;
    case P10_ACT_TOWER: {
        /* clear the turrets from the pillars, then the relay */
        int i;
        int turret = -1;
        float best = 1e9f;
        for (i = 0; i < P10_ENEMIES; ++i) {
            if (s->e[i].active && s->e[i].kind == P10_E_TURRET && s->e[i].f.pos.x < best) {
                best = s->e[i].f.pos.x;
                turret = i;
            }
        }
        if (turret >= 0) {
            /* close in until it is in sight and in range, round the pillars */
            bool seen = sees(s, s->body.pos, s->e[turret].f.pos) && flat(s->body.pos, s->e[turret].f.pos) < 20.0f;
            goal = seen ? s->body.pos : s->e[turret].f.pos;
            if (s->body.pos.x < CC(60)) {
                goal = cadv_v3(CC(62), 0, CC(46));
            }
        } else {
            goal = cadv_v3(s->relay[2].x - 1.6f, 0, s->relay[2].z);
        }
        break;
    }
    case P10_ACT_STAR:
        goal = s->star_pos;
        break;
    default:
        /* wait behind the kiosk until the VTOL is down, then onto the pad */
        goal = s->vtol_down ? cadv_v3(s->pad.x, 0, s->pad.z) : cadv_v3(CC(47), 0, CC(49));
        break;
    }
    if (!s->oc.on[CADV_OC_AIM] && !cadv_overclock_crashed(&s->oc) && s->oc.stamina > 60.0f && nearest_hostile(s, s->body.pos, 35.0f) >= 0 && s->ticks % 20 == 0) {
        bot_press(out, CADV_ACT_CROUCH);
    } else if (s->oc.on[CADV_OC_AIM] && s->oc.stamina < 20.0f) {
        bot_press(out, CADV_ACT_CROUCH);
    }
    fighting = bot_fight(s, out, s->act == P10_ACT_TOWER ? 40.0f : 36.0f);
    if (!fighting && s->act == P10_ACT_MARKET) {
        out->held |= CADV_ACT_BIT(CADV_ACT_ALT_FIRE); /* gather a crowd on the way */
    }
    if (s->bot_push > 0) {
        --s->bot_push;
    }
    {
        cadv_input_frame walk;
        float d;
        memset(&walk, 0, sizeof walk);
        d = bot_walk(s, goal, &walk, !fighting);
        out->move_x = walk.move_x;
        out->move_y = walk.move_y;
        out->pressed |= walk.pressed;
        out->held |= walk.held & ~CADV_ACT_BIT(CADV_ACT_FIRE);
        if (!fighting) {
            out->look_x = walk.look_x;
            out->look_y = walk.look_y;
        } else if (s->hp < 45 && s->bot_push == 0) {
            /* hurt: stand and trade from where we are; the cell and the regen help */
            out->move_x = 0.0f;
            out->move_y = 0.0f;
        }
        if (d < 1.2f) {
            if ((s->act == P10_ACT_MARKET && !s->relay_done[0]) || (s->act == P10_ACT_INDUSTRIAL && flat(s->body.pos, s->relay[1]) < 2.5f) ||
                (s->act == P10_ACT_TOWER && flat(s->body.pos, s->relay[2]) < 2.5f)) {
                if (s->ticks % 20 == 0 && !fighting) {
                    bot_press(out, CADV_ACT_USE);
                }
            }
        }
    }
}
