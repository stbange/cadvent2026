/* SPDX-License-Identifier: MIT */
#include "portals/p05_hearthguard.h"

#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "core/mathx.h"
#include "hub/friends.h"
#include "portals/p05_internal.h"

/* ---- tables (original units and structures) ---------------------------------------- */

#define F_HALL (CADV_BF_HQ | CADV_BF_REFINERY)
static const cadv_rts_bdef k_buildings[P05_BUILDINGS] = {
    /* name           fw fh cost pow hp    ticks flags                 range dmg cool sight sup lum tal */
    {"Town Hall", 3, 3, 0, 0, 1500, 1, F_HALL, 0, 0, 0, 7, 5, 0, 0},
    {"Farm", 2, 2, 80, 0, 300, 300, 0, 0, 0, 0, 3, 5, 40, 0},
    {"Barracks", 3, 2, 160, 0, 600, 480, CADV_BF_BARRACKS, 0, 0, 0, 4, 0, 80, 0},
    {"Lumber Mill", 2, 2, 120, 0, 450, 360, CADV_BF_REFINERY, 0, 0, 0, 4, 0, 60, 0},
    {"Watchtower", 1, 1, 100, 0, 400, 360, CADV_BF_DEFENCE, 18.0f, 9, 30, 8, 0, 80, 0},
    {"Shipyard", 3, 2, 150, 0, 600, 480, P05_BF_SHIPYARD, 0, 0, 0, 4, 0, 100, 0},
    {"Workshop", 2, 2, 160, 0, 500, 480, P05_BF_WORKSHOP, 0, 0, 0, 4, 0, 80, 40},
    {"Keep", 3, 3, 0, 0, 2500, 1, F_HALL, 0, 0, 0, 8, 8, 0, 0},
    {"Longhall", 3, 3, 0, 0, 3000, 1, CADV_BF_HQ, 0, 0, 0, 8, 0, 0, 0},
    {"Troll Hut", 3, 2, 200, 0, 700, 600, CADV_BF_BARRACKS, 0, 0, 0, 5, 0, 0, 0},
    {"Bone Totem", 1, 1, 150, 0, 450, 400, CADV_BF_DEFENCE, 15.0f, 10, 40, 7, 0, 0, 0},
    {"Tavern", 2, 2, 0, 0, 500, 1, 0, 0, 0, 0, 3, 0, 0, 0},
};

static const cadv_rts_udef k_units[P05_UNITS] = {
    /* name        cost hp   speed  range dmg cool ticks harv carry from                sight inf  sup lum tal gath naval cap */
    {"Worker", 50, 40, 0.060f, 0.0f, 0, 0, 300, true, 20, CADV_BF_HQ, 4, true, 1, 0, 0, 7, false, 0},
    {"Footman", 80, 70, 0.055f, 2.5f, 9, 40, 360, false, 0, CADV_BF_BARRACKS, 5, true, 1, 0, 0, 0, false, 0},
    {"Archer", 70, 45, 0.055f, 15.0f, 7, 40, 360, false, 0, CADV_BF_BARRACKS, 6, true, 1, 30, 0, 0, false, 0},
    {"Knight", 140, 140, 0.090f, 2.5f, 16, 45, 480, false, 0, CADV_BF_BARRACKS, 6, false, 2, 40, 0, 0, false, 0},
    {"Ballista", 120, 110, 0.045f, 20.0f, 55, 150, 540, false, 0, P05_BF_WORKSHOP, 7, false, 2, 120, 40, 0, false, 0},
    {"Ferry", 100, 220, 0.100f, 0.0f, 0, 0, 480, false, 0, P05_BF_SHIPYARD, 6, false, 0, 80, 60, 0, true, 6},
    {"Captain", 0, 350, 0.0f, 0.0f, 0, 0, 1, false, 0, CADV_BF_HQ, 7, true, 0, 0, 0, 0, false, 0},
    {"Troll Axeman", 90, 75, 0.055f, 2.5f, 10, 45, 420, false, 0, CADV_BF_BARRACKS, 5, true, 0, 0, 0, 0, false, 0},
    {"Troll Thrower", 80, 50, 0.055f, 13.0f, 8, 50, 420, false, 0, CADV_BF_BARRACKS, 6, true, 0, 0, 0, 0, false, 0},
    {"Warlord", 0, 400, 0.060f, 2.5f, 22, 50, 1, false, 0, CADV_BF_HQ, 7, true, 0, 0, 0, 0, false, 0},
};

static const int k_stave_struct[P05_STAVE_STRUCTURES] = {P05_B_FARM, P05_B_BARRACKS, P05_B_MILL, P05_B_TOWER, P05_B_SHIPYARD, P05_B_WORKSHOP};
static const int k_stave_unit[P05_STAVE_UNITS] = {P05_U_WORKER, P05_U_FOOTMAN, P05_U_ARCHER, P05_U_KNIGHT, P05_U_BALLISTA, P05_U_FERRY};
#define KEEP_GOLD 300
#define KEEP_LUMBER 200
#define KEEP_TICKS 900

static const cadv_rts_ai_build k_troll_plan[] = {{P05_B_HUT, 24, 8}, {P05_B_HUT, 36, 8}, {P05_B_TOTEM, 27, 13}, {P05_B_TOTEM, 35, 13}};
static const int k_troll_train[] = {P05_U_AXE, P05_U_AXE, P05_U_THROWER};

const cadv_rts_bdef *cadv_p05_buildings(void) { return k_buildings; }
const cadv_rts_udef *cadv_p05_units(void) { return k_units; }

const char *cadv_p05_stave_name(int item)
{
    if (item < P05_STAVE_STRUCTURES) {
        return k_buildings[k_stave_struct[item]].name;
    }
    if (item < P05_STAVE_STRUCTURES + P05_STAVE_UNITS) {
        return k_units[k_stave_unit[item - P05_STAVE_STRUCTURES]].name;
    }
    return "Keep Upgrade";
}

void cadv_p05_stave_cost(int item, int *gold, int *lumber, int *tallow)
{
    if (item < P05_STAVE_STRUCTURES) {
        const cadv_rts_bdef *b = &k_buildings[k_stave_struct[item]];
        *gold = b->cost;
        *lumber = b->lumber;
        *tallow = b->tallow;
    } else if (item < P05_STAVE_STRUCTURES + P05_STAVE_UNITS) {
        const cadv_rts_udef *u = &k_units[k_stave_unit[item - P05_STAVE_STRUCTURES]];
        *gold = u->cost;
        *lumber = u->lumber;
        *tallow = u->tallow;
    } else {
        *gold = KEEP_GOLD;
        *lumber = KEEP_LUMBER;
        *tallow = 0;
    }
}

/* Bounded copy: long lines are cut on purpose. */
static void copy_text(char *out, size_t cap, const char *text)
{
    size_t i;
    for (i = 0; text[i] && i + 1 < cap; ++i) {
        out[i] = text[i];
    }
    out[i] = '\0';
}

static void say(p05_state *s, const char *text)
{
    copy_text(s->message, sizeof s->message, text);
    s->message_timer = 180;
}

static void quip(p05_state *s, const char *text)
{
    copy_text(s->quip, sizeof s->quip, text);
    s->quip_timer = 240;
}

static void sfx(p05_state *s, cadv_sfx id, float vol)
{
    if (s->cfg.audio) {
        cadv_audio_sfx(s->cfg.audio, id, vol, 0.0f);
    }
}

static float cc(int c) { return ((float)c + 0.5f) * P05_CELL; }

static cadv_vec3 centre_of(const p05_state *s, int b)
{
    const cadv_rts_building *bb = &s->rts.b[b];
    const cadv_rts_bdef *d = &k_buildings[bb->def];
    return cadv_v3(((float)bb->x + 0.5f * (float)d->fw) * P05_CELL, 0, ((float)bb->z + 0.5f * (float)d->fh) * P05_CELL);
}

int cadv_p05_raid_ticks(const p05_state *s, int raid)
{
    static const int story[3] = {300, 570, 840};
    static const int normal[3] = {210, 420, 630};
    static const int hard[3] = {165, 345, 525};
    const int *t = s->cfg.difficulty == CADV_DIFF_STORY ? story : (s->cfg.difficulty == CADV_DIFF_HARD ? hard : normal);
    return t[raid] * 60;
}

int cadv_p05_raiders_alive(const p05_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        n += s->raider[i] && s->rts.u[i].alive && s->rts.u[i].owner == P05_TROLLS;
    }
    return n;
}

/* ---- the valley of Kindlewick --------------------------------------------------------- */

static void terrain_rect(p05_state *s, int x0, int z0, int x1, int z1, int kind)
{
    int x;
    int z;
    for (z = z0; z <= z1; ++z) {
        for (x = x0; x <= x1; ++x) {
            if (x >= 0 && z >= 0 && x < P05_W && z < P05_H) {
                s->terrain[z * P05_W + x] = (uint8_t)kind;
                if (kind == P05_T_ROCK) {
                    cadv_rts_set_blocked(&s->rts.grid, x, z, x, z);
                }
            }
        }
    }
}

static int unit_at(p05_state *s, int side, int def, int cx, int cz) { return cadv_rts_add_unit(&s->rts, side, def, cc(cx), cc(cz)); }

static void build_valley(p05_state *s)
{
    cadv_rts *r = &s->rts;
    int z;
    terrain_rect(s, 0, 0, P05_W - 1, 0, P05_T_ROCK);
    terrain_rect(s, 0, P05_H - 1, P05_W - 1, P05_H - 1, P05_T_ROCK);
    terrain_rect(s, 0, 0, 0, P05_H - 1, P05_T_ROCK);
    terrain_rect(s, P05_W - 1, 0, P05_W - 1, P05_H - 1, P05_T_ROCK);
    terrain_rect(s, 1, P05_LAKE_Z0, P05_W - 2, P05_LAKE_Z1, P05_T_LAKE); /* frozen until spring */
    terrain_rect(s, 14, 18, 18, 21, P05_T_ROCK);
    terrain_rect(s, 46, 17, 50, 20, P05_T_ROCK);
    /* forests: lumber that blocks the way until felled */
    for (z = 38; z <= 61; ++z) {
        int x0 = 3 + (z % 3 == 0 ? 1 : 0);
        cadv_rts_add_resource(&r->grid, x0, z, 19 - (z % 4 == 1 ? 1 : 0) - (z % 5 == 2 ? 1 : 0), z, 1, 40, true);
    }
    cadv_rts_add_resource(&r->grid, 50, 53, 58, 60, 1, 40, true);
    cadv_rts_add_resource(&r->grid, 6, 4, 14, 12, 1, 40, true);
    /* the gold mine and the whale-tallow platform on the shore */
    cadv_rts_add_resource(&r->grid, 38, 46, 39, 47, 0, 3000, true);
    cadv_rts_add_resource(&r->grid, 44, 34, 45, 34, 2, 1500, true);
    /* the Rimeback Clans' camp */
    s->longhall = cadv_rts_add_building(r, P05_TROLLS, P05_B_LONGHALL, 30, 5);
    cadv_rts_add_building(r, P05_TROLLS, P05_B_HUT, 24, 8);
    cadv_rts_add_building(r, P05_TROLLS, P05_B_HUT, 36, 8);
    cadv_rts_add_building(r, P05_TROLLS, P05_B_TOTEM, 27, 13);
    cadv_rts_add_building(r, P05_TROLLS, P05_B_TOTEM, 35, 13);
    unit_at(s, P05_TROLLS, P05_U_AXE, 30, 12);
    unit_at(s, P05_TROLLS, P05_U_AXE, 33, 12);
    unit_at(s, P05_TROLLS, P05_U_THROWER, 31, 10);
    /* Kindlewick */
    s->hall = cadv_rts_add_building(r, P05_GNOMES, P05_B_HALL, 30, 50);
    s->tavern = cadv_rts_add_building(r, CADV_RTS_NEUTRAL, P05_B_TAVERN, 23, 55);
    unit_at(s, P05_GNOMES, P05_U_WORKER, 29, 54);
    unit_at(s, P05_GNOMES, P05_U_WORKER, 31, 54);
    unit_at(s, P05_GNOMES, P05_U_WORKER, 33, 54);
    cadv_rts_update_fog(r, P05_GNOMES);
}

static void setup_ai(p05_state *s)
{
    cadv_rts_ai *ai = &s->ai;
    cadv_rts_ai_init(ai, P05_TROLLS);
    ai->plan = k_troll_plan;
    ai->nplan = (int)(sizeof k_troll_plan / sizeof k_troll_plan[0]);
    ai->train = k_troll_train;
    ai->ntrain = (int)(sizeof k_troll_train / sizeof k_troll_train[0]);
    ai->home.x = 31;
    ai->home.z = 8;
    ai->home_radius = 13;
    ai->wave_every = 1 << 30; /* the raids are scripted */
    ai->income = s->cfg.difficulty == CADV_DIFF_HARD ? 3 : 2;
    ai->army_cap = s->cfg.difficulty == CADV_DIFF_STORY ? 4 : (s->cfg.difficulty == CADV_DIFF_HARD ? 9 : 6);
    s->rts.side[P05_TROLLS].credits = 600;
}

static void spawn_captain(p05_state *s, float x, float z)
{
    s->captain = cadv_rts_add_unit(&s->rts, P05_GNOMES, P05_U_CAPTAIN, x, z);
}

static void thaw(p05_state *s);

static void place_checkpoint_base(p05_state *s)
{
    cadv_rts *r = &s->rts;
    int i;
    cadv_rts_add_building(r, P05_GNOMES, P05_B_FARM, 26, 51);
    cadv_rts_add_building(r, P05_GNOMES, P05_B_MILL, 22, 47);
    cadv_rts_add_building(r, P05_GNOMES, P05_B_BARRACKS, 33, 46);
    cadv_rts_add_building(r, P05_GNOMES, P05_B_FARM, 26, 54);
    cadv_rts_add_building(r, P05_GNOMES, P05_B_TOWER, 30, 46);
    for (i = 0; i < 4; ++i) {
        unit_at(s, P05_GNOMES, P05_U_WORKER, 28 + i, 55);
    }
    for (i = 0; i < 5; ++i) {
        unit_at(s, P05_GNOMES, i % 2 ? P05_U_ARCHER : P05_U_FOOTMAN, 30 + i, 44);
    }
    r->side[P05_GNOMES].credits = 800;
    r->side[P05_GNOMES].lumber = 600;
}

bool cadv_p05_init(void *state, const cadv_portal_cfg *cfg)
{
    p05_state *s = (p05_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    s->checkpoint = cfg->checkpoint;
    s->last_pick = -1;
    if (!cadv_rts_init(&s->rts, P05_W, P05_H, P05_CELL, k_buildings, P05_BUILDINGS, k_units, P05_UNITS)) {
        return false; /* GCOVR_EXCL_LINE: fixed sizes */
    }
    s->rts.use_supply = true;
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
    build_valley(s);
    setup_ai(s);
    s->rts.side[P05_GNOMES].credits = cfg->difficulty == CADV_DIFF_STORY ? 700 : 450;
    s->rts.side[P05_GNOMES].lumber = cfg->difficulty == CADV_DIFF_STORY ? 400 : 250;
    spawn_captain(s, cc(31), cc(55));
    s->yaw = -0.6f; /* the hall to the left, the lake ahead */
    s->raid_timer = cadv_p05_raid_ticks(s, 0);
    if (s->checkpoint >= 1) {
        /* after the first raid: the village stands, two raids to go */
        place_checkpoint_base(s);
        s->raid = 1;
        s->raid_timer = cadv_p05_raid_ticks(s, 1) - cadv_p05_raid_ticks(s, 0);
        s->ticks = cadv_p05_raid_ticks(s, 0) + 60 * 30;
    }
    if (s->checkpoint >= 2) {
        /* spring: the raids are over and the lake is open */
        cadv_rts *r = &s->rts;
        cadv_rts_add_building(r, P05_GNOMES, P05_B_FARM, 35, 54);
        cadv_rts_add_building(r, P05_GNOMES, P05_B_FARM, 35, 57);
        cadv_rts_add_building(r, P05_GNOMES, P05_B_TOWER, 34, 43);
        for (i = 0; i < 4; ++i) {
            unit_at(s, P05_GNOMES, P05_U_WORKER, 28 + i, 57);
            unit_at(s, P05_GNOMES, i % 2 ? P05_U_ARCHER : P05_U_FOOTMAN, 30 + i, 42);
        }
        r->side[P05_GNOMES].credits = 1200;
        r->side[P05_GNOMES].lumber = 900;
        r->side[P05_GNOMES].tallow = 150;
        s->raid = CADV_P05_RAIDS;
        s->ticks = cadv_p05_raid_ticks(s, CADV_P05_RAIDS - 1) + 60 * 60;
        thaw(s);
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        s->was_alive[i] = s->rts.u[i].alive;
    }
    cadv_rts_update_supply(&s->rts, P05_GNOMES);
    if (s->checkpoint < 2) {
        quip(s, s->checkpoint >= 1 ? "They'll be back. Let them come." : "Kindlewick, to work! The Rimeback raiders are coming.");
        say(s, "Q PICKS FROM THE STAVE, E BUILDS - FARMS FIRST");
    }
    return true;
}

/* ---- aiming (shared shape with Day 3) ----------------------------------------------------- */

cadv_vec3 cadv_p05_eye(const p05_state *s)
{
    if (s->captain >= 0) {
        return cadv_v3(s->rts.u[s->captain].x, P05_EYE + (s->rts.u[s->captain].carrier >= 0 ? 1.0f : 0.0f), s->rts.u[s->captain].z);
    }
    return cadv_v3(cc(31), P05_EYE, cc(55));
}

static cadv_vec3 forward(const p05_state *s) { return cadv_v3_from_yaw_pitch(s->yaw, s->pitch); }

bool cadv_p05_aim_ground(const p05_state *s, float *x, float *z)
{
    cadv_vec3 e = cadv_p05_eye(s);
    cadv_vec3 f = forward(s);
    float t;
    if (f.y > -0.004f) {
        return false;
    }
    t = -e.y / f.y;
    if (t > 160.0f) {
        return false;
    }
    *x = e.x + f.x * t;
    *z = e.z + f.z * t;
    return *x >= 0.0f && *z >= 0.0f && *x < (float)P05_W * P05_CELL && *z < (float)P05_H * P05_CELL;
}

static float ray_point(cadv_vec3 e, cadv_vec3 f, cadv_vec3 p, float radius)
{
    cadv_vec3 to = cadv_v3_sub(p, e);
    float t = cadv_v3_dot(to, f);
    if (t < 0.5f || t > 120.0f) {
        return -1.0f;
    }
    return cadv_v3_dist(cadv_v3_add(e, cadv_v3_scale(f, t)), p) <= radius ? t : -1.0f;
}

static float ray_box(cadv_vec3 e, cadv_vec3 f, cadv_vec3 lo, cadv_vec3 hi)
{
    float t0 = 0.0f;
    float t1 = 120.0f;
    int k;
    const float *o = &e.x;
    const float *dd = &f.x;
    const float *a = &lo.x;
    const float *b = &hi.x;
    for (k = 0; k < 3; ++k) {
        if (cadv_absf(dd[k]) < 1e-6f) {
            if (o[k] < a[k] || o[k] > b[k]) {
                return -1.0f;
            }
        } else {
            float ta = (a[k] - o[k]) / dd[k];
            float tb = (b[k] - o[k]) / dd[k];
            if (ta > tb) {
                float tmp = ta;
                ta = tb;
                tb = tmp;
            }
            t0 = cadv_maxf(t0, ta);
            t1 = cadv_minf(t1, tb);
            if (t0 > t1) {
                return -1.0f;
            }
        }
    }
    return t0;
}

int cadv_p05_aim_target(const p05_state *s, bool own)
{
    const cadv_rts *r = &s->rts;
    cadv_vec3 e = cadv_p05_eye(s);
    cadv_vec3 f = forward(s);
    float best = 1e9f;
    int hit = -1;
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        bool mine = u->owner == P05_GNOMES;
        float t;
        if (!u->alive || i == s->captain || mine != own || u->carrier >= 0 ||
            (!mine && !cadv_rts_cell_visible(r, P05_GNOMES, u->x, u->z))) {
            continue;
        }
        t = ray_point(e, f, cadv_v3(u->x, k_units[u->def].infantry ? 0.8f : 1.0f, u->z), k_units[u->def].infantry ? 1.2f : 2.2f);
        if (t >= 0.0f && t < best) {
            best = t;
            hit = i;
        }
    }
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        const cadv_rts_bdef *d = &k_buildings[b->def];
        bool mine = b->owner == P05_GNOMES;
        float t;
        if (!b->alive || mine != own || b->owner == CADV_RTS_NEUTRAL) {
            continue;
        }
        t = ray_box(e, f, cadv_v3((float)b->x * P05_CELL, 0, (float)b->z * P05_CELL),
                    cadv_v3((float)(b->x + d->fw) * P05_CELL, b->def == P05_B_LONGHALL ? 7.0f : 4.0f, (float)(b->z + d->fh) * P05_CELL));
        if (t >= 0.0f && t < best) {
            best = t;
            hit = CADV_RTS_TARGET_BUILDING + i;
        }
    }
    return hit;
}

/* The own ferry under the crosshair, even with soldiers crowding in front of it. */
static int aim_ferry(const p05_state *s)
{
    const cadv_rts *r = &s->rts;
    cadv_vec3 e = cadv_p05_eye(s);
    cadv_vec3 f = forward(s);
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (u->alive && u->owner == P05_GNOMES && u->def == P05_U_FERRY && ray_point(e, f, cadv_v3(u->x, 1.0f, u->z), 2.2f) >= 0.0f) {
            return i;
        }
    }
    return -1;
}

/* ---- the Captain ------------------------------------------------------------------------ */

static bool walkable(const p05_state *s, float x, float z)
{
    cadv_cell c = cadv_rts_cell_at(&s->rts.grid, x, z);
    return cadv_rts_passable(&s->rts.grid, c.x, c.z);
}

static void walk(p05_state *s, const cadv_input_frame *in)
{
    cadv_rts_unit *u = &s->rts.u[s->captain];
    float speed = (cadv_input_down(in, CADV_ACT_RUN) ? 6.5f : 4.5f) * P05_DT;
    float fx = -cadv_sinf(s->yaw);
    float fz = -cadv_cosf(s->yaw);
    float mx = cadv_clampf(in->move_x, -1, 1);
    float my = cadv_clampf(in->move_y, -1, 1);
    float dx = (fx * my - fz * mx) * speed;
    float dz = (fz * my + fx * mx) * speed;
    s->yaw = cadv_wrap_angle(s->yaw + in->look_x);
    s->pitch = cadv_clampf(s->pitch + in->look_y, -1.3f, 1.2f);
    if (u->carrier >= 0) {
        return; /* aboard the ferry */
    }
    if (walkable(s, u->x + dx + (dx > 0 ? 0.4f : -0.4f), u->z)) {
        u->x += dx;
    }
    if (walkable(s, u->x, u->z + dz + (dz > 0 ? 0.4f : -0.4f))) {
        u->z += dz;
    }
    if (dx != 0.0f || dz != 0.0f) {
        u->yaw = s->yaw;
    }
}

static int count_selected(const p05_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        n += s->selected[i] && s->rts.u[i].alive;
    }
    return n;
}

static bool is_army(const p05_state *s, int i)
{
    const cadv_rts_unit *u = &s->rts.u[i];
    return u->alive && u->owner == P05_GNOMES && i != s->captain && u->def != P05_U_WORKER && u->def != P05_U_FERRY;
}

static void select_where(p05_state *s, int def, bool army)
{
    int i;
    memset(s->selected, 0, sizeof s->selected);
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &s->rts.u[i];
        if (army ? is_army(s, i) : (u->alive && u->owner == P05_GNOMES && u->def == def)) {
            s->selected[i] = 1;
        }
    }
}

static bool resource_near(const p05_state *s, float gx, float gz, cadv_cell *out)
{
    cadv_cell c = cadv_rts_cell_at(&s->rts.grid, gx, gz);
    int dx;
    int dz;
    for (dz = 0; dz <= 1; ++dz) {
        for (dx = -dz; dx <= dz; ++dx) {
            int k;
            for (k = 0; k < (dz ? 4 : 1); ++k) {
                int x = c.x + (k == 0 ? dx : k == 1 ? -dx : k == 2 ? dz : -dz);
                int z = c.z + (k == 0 ? dz : k == 1 ? -dz : k == 2 ? dx : -dx);
                if (cadv_rts_in_bounds(&s->rts.grid, x, z) && s->rts.grid.ore[cadv_rts_index(&s->rts.grid, x, z)] > 0) {
                    out->x = x;
                    out->z = z;
                    return true;
                }
            }
        }
    }
    return false;
}

static void order_selected(p05_state *s, int target, float gx, float gz, bool ground, bool force_move)
{
    cadv_rts *r = &s->rts;
    cadv_cell goal = cadv_rts_cell_at(&r->grid, gx, gz);
    cadv_cell res;
    bool on_res = ground && resource_near(s, gx, gz, &res);
    bool to_ferry = target >= 0 && target < CADV_RTS_TARGET_BUILDING && r->u[target].owner == P05_GNOMES && r->u[target].def == P05_U_FERRY;
    float mid_lake = (float)(P05_LAKE_Z0 + P05_LAKE_Z1 + 1) * 0.5f * P05_CELL;
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (!s->selected[i] || !u->alive || i == target) {
            continue;
        }
        if (to_ferry && (u->z > mid_lake) != (r->u[target].z > mid_lake)) {
            continue; /* only those on the ferry's side of the lake can board */
        }
        ++n;
        if (to_ferry && !k_units[u->def].naval) {
            cadv_rts_order_board(r, i, target);
        } else if (u->def == P05_U_WORKER && on_res) {
            cadv_rts_order_harvest_at(r, i, res);
        } else if (u->def == P05_U_WORKER && !ground) {
            cadv_rts_order_harvest(r, i);
        } else if (!ground && !to_ferry && k_units[u->def].damage > 0) {
            cadv_rts_order_attack(r, i, target);
        } else if (ground) {
            cadv_rts_order_move(r, i, goal, !force_move && k_units[u->def].damage > 0);
        }
    }
    if (n > 0) {
        say(s, to_ferry ? "ALL ABOARD!" : on_res ? "TO WORK!" : ground ? (force_move ? "MARCHING" : "FORWARD, KINDLEWICK!") : "AT THEM!");
        sfx(s, CADV_SFX_BELL, 0.25f); /* the signal horn */
    }
}

static bool shore_ok(const p05_state *s, int x, int z, int fw, int fh)
{
    int i;
    /* a shipyard needs lake along one side of its footprint */
    for (i = x; i < x + fw; ++i) {
        if (z - 1 >= 0 && s->terrain[(z - 1) * P05_W + i] == P05_T_LAKE) {
            return true;
        }
        if (z + fh < P05_H && s->terrain[(z + fh) * P05_W + i] == P05_T_LAKE) {
            return true;
        }
    }
    return false;
}

static bool on_lake(const p05_state *s, int x0, int z0, int fw, int fh)
{
    int x;
    int z;
    for (z = z0; z < z0 + fh; ++z) {
        for (x = x0; x < x0 + fw; ++x) {
            if (x >= 0 && z >= 0 && x < P05_W && z < P05_H && s->terrain[z * P05_W + x] == P05_T_LAKE) {
                return true;
            }
        }
    }
    return false;
}

static void place(p05_state *s)
{
    cadv_rts *r = &s->rts;
    int def = r->side[P05_GNOMES].ready_building;
    const cadv_rts_bdef *d = &k_buildings[def];
    float gx;
    float gz;
    cadv_cell c;
    cadv_place_result res;
    int x0;
    int z0;
    if (!cadv_p05_aim_ground(s, &gx, &gz)) {
        say(s, "POINT THE STAVE AT THE GROUND");
        return;
    }
    c = cadv_rts_cell_at(&r->grid, gx, gz);
    x0 = c.x - d->fw / 2;
    z0 = c.z - d->fh / 2;
    if (def == P05_B_SHIPYARD) {
        /* shipyards go on the shore, wherever it is */
        if (on_lake(s, x0, z0, d->fw, d->fh) || !shore_ok(s, x0, z0, d->fw, d->fh)) {
            say(s, "A SHIPYARD MUST STAND ON THE LAKE SHORE");
            sfx(s, CADV_SFX_DENIED, 0.6f);
            return;
        }
        res = cadv_rts_can_place(&r->grid, x0, z0, d->fw, d->fh, P05_GNOMES, 0, true);
        if (res == CADV_PLACE_OK) {
            cadv_rts_add_building(r, P05_GNOMES, def, x0, z0);
            r->side[P05_GNOMES].ready_building = -1;
        }
    } else if (on_lake(s, x0, z0, d->fw, d->fh)) {
        say(s, "THE ICE WON'T HOLD A BUILDING");
        sfx(s, CADV_SFX_DENIED, 0.6f);
        return;
    } else {
        res = cadv_rts_place_ready(r, P05_GNOMES, x0, z0);
    }
    if (res == CADV_PLACE_OK) {
        s->placing = false;
        sfx(s, CADV_SFX_DOOR, 0.8f);
        say(s, "BUILT!");
    } else {
        say(s, res == CADV_PLACE_TOO_FAR ? "TOO FAR FROM THE VILLAGE" : "CANNOT BUILD THERE");
        sfx(s, CADV_SFX_DENIED, 0.6f);
    }
}

static void stave(p05_state *s, const cadv_input_frame *in)
{
    cadv_rts *r = &s->rts;
    cadv_rts_side *me = &r->side[P05_GNOMES];
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_RADIAL)) {
        s->stave_item = (s->stave_item + 1) % P05_STAVE_ITEMS;
    }
    if (cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        s->stave_item = (s->stave_item + P05_STAVE_ITEMS - 1) % P05_STAVE_ITEMS;
    }
    if (cadv_input_pressed(in, CADV_ACT_MAP)) {
        s->map_open = !s->map_open;
    }
    if (!cadv_input_pressed(in, CADV_ACT_USE)) {
        return;
    }
    if (s->captain >= 0 && s->rts.u[s->captain].carrier >= 0) {
        return; /* aboard: JUMP steps ashore */
    }
    if (s->stave_item == P05_STAVE_KEEP) {
        if (s->keep || s->keep_timer > 0) {
            say(s, s->keep ? "THE HALL IS ALREADY A KEEP" : "THE KEEP IS GOING UP");
        } else if (me->credits >= KEEP_GOLD && me->lumber >= KEEP_LUMBER) {
            me->credits -= KEEP_GOLD;
            me->lumber -= KEEP_LUMBER;
            s->keep_timer = KEEP_TICKS;
            say(s, "RAISING THE KEEP");
        } else {
            say(s, "NOT ENOUGH GOLD OR LUMBER");
            sfx(s, CADV_SFX_DENIED, 0.6f);
        }
    } else if (s->stave_item < P05_STAVE_STRUCTURES) {
        if (me->ready_building >= 0) {
            s->placing = true;
            say(s, "POINT AND FIRE TO PLACE IT");
        } else if (cadv_rts_queue(r, P05_GNOMES, true, k_stave_struct[s->stave_item])) {
            say(s, "BUILDING");
        } else {
            say(s, me->queued[0] >= CADV_RTS_QUEUE ? "QUEUE FULL" : "NOT ENOUGH GOLD OR LUMBER");
            sfx(s, CADV_SFX_DENIED, 0.6f);
        }
    } else {
        int def = k_stave_unit[s->stave_item - P05_STAVE_STRUCTURES];
        uint32_t before = r->events[P05_GNOMES];
        if (def == P05_U_KNIGHT && !s->keep) {
            say(s, "KNIGHTS NEED A KEEP");
            sfx(s, CADV_SFX_DENIED, 0.6f);
        } else if (cadv_rts_queue(r, P05_GNOMES, false, def)) {
            say(s, "TRAINING");
        } else {
            const char *why = !cadv_rts_can_train(r, P05_GNOMES, def) ? (def == P05_U_BALLISTA ? "BALLISTAS NEED A WORKSHOP"
                                                                         : def == P05_U_FERRY ? "FERRIES NEED A SHIPYARD"
                                                                                              : "NEEDS A BARRACKS")
                              : ((r->events[P05_GNOMES] & ~before) & CADV_RTS_EV_SUPPLY) ? "NOT ENOUGH FOOD - BUILD A FARM"
                              : me->queued[1] >= CADV_RTS_QUEUE ? "QUEUE FULL" : "NOT ENOUGH RESOURCES";
            say(s, why);
            sfx(s, CADV_SFX_DENIED, 0.6f);
        }
    }
}

static int ferry_near_captain(const p05_state *s)
{
    int i;
    const cadv_rts_unit *c = &s->rts.u[s->captain];
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &s->rts.u[i];
        if (u->alive && u->owner == P05_GNOMES && u->def == P05_U_FERRY &&
            (u->x - c->x) * (u->x - c->x) + (u->z - c->z) * (u->z - c->z) < (2.6f * P05_CELL) * (2.6f * P05_CELL)) {
            return i;
        }
    }
    return -1;
}

static void board_or_land(p05_state *s)
{
    cadv_rts *r = &s->rts;
    cadv_rts_unit *c = &r->u[s->captain];
    if (c->carrier >= 0) {
        /* step ashore onto free land beside the ferry */
        cadv_rts_unit *f = &r->u[c->carrier];
        cadv_cell fc = cadv_rts_cell_at(&r->grid, f->x, f->z);
        int dx;
        int dz;
        int ring;
        for (ring = 1; ring <= 2; ++ring) { /* the nearest free land first */
            for (dz = -ring; dz <= ring; ++dz) {
            for (dx = -ring; dx <= ring; ++dx) {
                if (cadv_rts_passable(&r->grid, fc.x + dx, fc.z + dz)) {
                    cadv_cell to = {fc.x + dx, fc.z + dz};
                    cadv_rts_cell_centre(&r->grid, to, &c->x, &c->z);
                    --f->passengers;
                    c->carrier = -1;
                    say(s, "ASHORE");
                    return;
                }
            }
            }
        }
        say(s, "NO SHORE TO STEP ONTO HERE");
        return;
    }
    {
        int f = ferry_near_captain(s);
        if (f >= 0) { /* the Captain takes the helm, however full the boat */
            c->carrier = f;
            ++r->u[f].passengers;
            say(s, "THE CAPTAIN IS ABOARD - ORDER THE FERRY TO A SHORE");
        }
    }
}

static void bash(p05_state *s)
{
    cadv_rts *r = &s->rts;
    const cadv_rts_unit *c = &r->u[s->captain];
    float fx = -cadv_sinf(s->yaw);
    float fz = -cadv_cosf(s->yaw);
    int i;
    int hits = 0;
    s->bash_cd = 0.9f;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        cadv_rts_unit *e = &r->u[i];
        float dx = e->x - c->x;
        float dz = e->z - c->z;
        float d = cadv_sqrtf(dx * dx + dz * dz);
        if (e->alive && e->owner == P05_TROLLS && e->carrier < 0 && d < 3.5f && (d < 0.5f || (dx * fx + dz * fz) / d > 0.5f)) {
            /* shield bash: hurts and shoves the troll back */
            if (d > 0.01f && walkable(s, e->x + dx / d * 2.0f, e->z + dz / d * 2.0f)) {
                e->x += dx / d * 2.0f;
                e->z += dz / d * 2.0f;
                e->path_len = 0;
            }
            cadv_rts_damage(r, i, 30);
            ++hits;
        }
    }
    sfx(s, hits ? CADV_SFX_HIT : CADV_SFX_SHOT, 0.6f);
}

static void command(p05_state *s, const cadv_input_frame *in)
{
    int i;
    float gx = 0.0f;
    float gz = 0.0f;
    bool ground = cadv_p05_aim_ground(s, &gx, &gz);
    for (i = 0; i < 4; ++i) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + i))) {
            if (cadv_input_down(in, CADV_ACT_RUN)) {
                int members[CADV_RTS_MAX_UNITS];
                int n = 0;
                int k;
                for (k = 0; k < CADV_RTS_MAX_UNITS; ++k) {
                    if (s->selected[k]) {
                        members[n++] = k;
                    }
                }
                cadv_rts_group_set(&s->rts, P05_GNOMES, i, members, n);
                say(s, "GROUP SET");
            } else {
                int k;
                memset(s->selected, 0, sizeof s->selected);
                for (k = 0; k < CADV_RTS_MAX_UNITS; ++k) {
                    s->selected[k] = s->rts.side[P05_GNOMES].group[i][k] && s->rts.u[k].alive;
                }
            }
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_WEAPON_5)) {
        select_where(s, 0, true);
    }
    if (cadv_input_pressed(in, CADV_ACT_WEAPON_6)) {
        select_where(s, P05_U_WORKER, false);
    }
    if (cadv_input_pressed(in, CADV_ACT_CROUCH)) {
        memset(s->selected, 0, sizeof s->selected);
    }
    if (cadv_input_pressed(in, CADV_ACT_JUMP)) {
        board_or_land(s);
    }
    if (cadv_input_pressed(in, CADV_ACT_ALT_FIRE)) {
        if (s->placing) {
            s->placing = false;
            say(s, "PLACING PAUSED - E TO RESUME");
        } else if (ground) {
            int n = 0;
            memset(s->selected, 0, sizeof s->selected);
            for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
                const cadv_rts_unit *u = &s->rts.u[i];
                if (u->alive && u->owner == P05_GNOMES && i != s->captain && u->carrier < 0 &&
                    (u->x - gx) * (u->x - gx) + (u->z - gz) * (u->z - gz) < 12.0f * 12.0f) {
                    s->selected[i] = 1;
                    ++n;
                }
            }
            if (n > 0) {
                say(s, "WITH ME!");
            }
        }
    }
    s->bash_cd -= P05_DT;
    if (!cadv_input_pressed(in, CADV_ACT_FIRE)) {
        return;
    }
    if (s->captain >= 0 && s->rts.u[s->captain].carrier >= 0) {
        /* aboard, the Captain commands the boat itself */
        if (ground) {
            cadv_rts_order_move(&s->rts, s->rts.u[s->captain].carrier, cadv_rts_cell_at(&s->rts.grid, gx, gz), false);
            say(s, "HELMSMAN, MAKE FOR THAT SHORE!");
        }
        return;
    }
    if (s->placing && s->rts.side[P05_GNOMES].ready_building >= 0) {
        place(s);
        return;
    }
    {
        int own = cadv_p05_aim_target(s, true);
        bool again = own >= 0 && own < CADV_RTS_TARGET_BUILDING && s->selected[own] && count_selected(s) == 1;
        /* pick with no squad, add with SHIFT, or click the one picked unit again */
        if (own >= 0 && own < CADV_RTS_TARGET_BUILDING && (count_selected(s) == 0 || cadv_input_down(in, CADV_ACT_RUN) || again)) {
            if (!cadv_input_down(in, CADV_ACT_RUN)) {
                memset(s->selected, 0, sizeof s->selected);
            }
            s->selected[own] = 1;
            if (s->rts.u[own].def == P05_U_WORKER) {
                static const char *const barks[] = {"READY TO WORK!", "YES, CAPTAIN?", "", "STOP POKING ME!"};
                s->worker_clicks = own == s->last_pick ? s->worker_clicks + 1 : 1;
                if (s->worker_clicks == 3) {
                    char buf[160];
                    snprintf(buf, sizeof buf, "\"%s!\"", s->mark_text[2]);
                    say(s, buf);
                    if (!(s->marks_found & 4u)) {
                        s->marks_found |= 4u;
                        quip(s, "A worker's bark - a friend's mark!");
                    }
                } else {
                    say(s, barks[(s->worker_clicks - 1) % 4]);
                }
            } else {
                say(s, k_units[s->rts.u[own].def].name);
            }
            s->last_pick = own;
            return;
        }
    }
    if (count_selected(s) > 0) {
        int target = cadv_p05_aim_target(s, false);
        int ferry = target < 0 ? aim_ferry(s) : -1;
        if (ferry >= 0 && !(s->selected[ferry] && count_selected(s) == 1)) {
            order_selected(s, ferry, gx, gz, false, false);
        } else if (target >= 0) {
            order_selected(s, target, gx, gz, false, false);
        } else if (ground) {
            order_selected(s, -1, gx, gz, true, cadv_input_down(in, CADV_ACT_RUN));
        }
    } else if (s->bash_cd <= 0.0f && s->captain >= 0 && s->rts.u[s->captain].carrier < 0) {
        bash(s);
    }
}

/* ---- rules ------------------------------------------------------------------------------ */

static int find_hall(const p05_state *s)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &s->rts.b[i];
        if (b->alive && b->owner == P05_GNOMES && (b->def == P05_B_HALL || b->def == P05_B_KEEP)) {
            return i;
        }
    }
    return -1;
}

static void send_raid(p05_state *s)
{
    cadv_rts *r = &s->rts;
    int axes = s->raid == 0 ? 4 : (s->raid == 1 ? 4 : 5);
    int throwers = s->raid == 0 ? 0 : 3;
    int i;
    int hall = find_hall(s);
    cadv_cell goal = {31, 50};
    if (s->cfg.difficulty == CADV_DIFF_STORY) {
        axes -= 1;
        throwers = throwers > 0 ? throwers - 1 : 0;
    } else if (s->cfg.difficulty == CADV_DIFF_HARD) {
        axes += 1;
        throwers += 1;
    }
    if (hall >= 0) {
        goal.x = r->b[hall].x + 1;
        goal.z = r->b[hall].z;
    }
    for (i = 0; i < axes + throwers + (s->raid == 2 ? 1 : 0); ++i) {
        int def = i < axes ? P05_U_AXE : (i < axes + throwers ? P05_U_THROWER : P05_U_WARLORD);
        int u = unit_at(s, P05_TROLLS, def, 27 + (i % 8), 19 + i / 8);
        if (u >= 0) {
            s->raider[u] = 1;
            cadv_rts_order_move(r, u, goal, true);
        }
    }
    ++s->raid;
    s->raid_timer = s->raid < CADV_P05_RAIDS ? cadv_p05_raid_ticks(s, s->raid) - cadv_p05_raid_ticks(s, s->raid - 1) : 0;
    if (s->raid == 1) {
        char buf[160];
        snprintf(buf, sizeof buf, "OWL: \"%s - RAIDERS ON THE ICE!\"", s->mark_text[1]);
        quip(s, buf);
        s->marks_found |= 2u;
    } else if (s->raid == 3) {
        char buf[160];
        snprintf(buf, sizeof buf, "%s LEADS THE LAST RAID!", s->mark_text[0]);
        quip(s, buf);
        s->marks_found |= 1u;
    }
    say(s, "TROLL TORCHES ON THE LAKE - A RAID!");
    sfx(s, CADV_SFX_BELL, 1.0f);
}

static void thaw(p05_state *s)
{
    cadv_rts *r = &s->rts;
    int i;
    s->thawed = true;
    cadv_rts_set_water(&r->grid, 1, P05_LAKE_Z0, P05_W - 2, P05_LAKE_Z1, true);
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        cadv_rts_unit *u = &r->u[i];
        cadv_cell c = cadv_rts_cell_at(&r->grid, u->x, u->z);
        if (u->alive && !k_units[u->def].naval && u->carrier < 0 && c.z >= P05_LAKE_Z0 && c.z <= P05_LAKE_Z1) {
            cadv_rts_damage(r, i, 10000); /* the ice gives way */
        }
    }
    say(s, "SPRING THAW! THE LAKE IS OPEN - BUILD A SHIPYARD AND A FERRY");
    quip(s, "The ward breaks with the ice. The Longhall is ours to burn.");
    if (s->checkpoint < 2) {
        s->checkpoint = 2;
    }
}

static void check_marks(p05_state *s)
{
    if (!(s->marks_found & 8u) && s->tavern >= 0 && s->captain >= 0) {
        cadv_vec3 t = centre_of(s, s->tavern);
        const cadv_rts_unit *c = &s->rts.u[s->captain];
        if ((c->x - t.x) * (c->x - t.x) + (c->z - t.z) * (c->z - t.z) < 14.0f * 14.0f) {
            char buf[160];
            s->marks_found |= 8u;
            snprintf(buf, sizeof buf, "THE TAVERN SIGN: \"%s\" - A FRIEND'S MARK!", s->mark_text[3]);
            say(s, buf);
        }
    }
}

static void star_rules(p05_state *s)
{
    cadv_rts *r = &s->rts;
    int i;
    if (s->star_state == 0 && s->longhall >= 0 && !r->b[s->longhall].alive) {
        cadv_vec3 c = centre_of(s, s->longhall);
        cadv_cell corner = {58, 3};
        s->star_state = 1;
        s->star_x = c.x;
        s->star_z = c.z;
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            if (r->u[i].alive && r->u[i].owner == P05_TROLLS) {
                cadv_rts_order_move(r, i, corner, false); /* the clans scatter */
            }
        }
        say(s, "THE LONGHALL BURNS! A STAR RISES FROM ITS FIRE PIT");
        quip(s, "Bring it home, Captain - the hearth is waiting.");
        sfx(s, CADV_SFX_EXPLOSION, 1.0f);
    }
    if (s->captain < 0) {
        return;
    }
    {
        const cadv_rts_unit *c = &r->u[s->captain];
        if (s->star_state == 1 && c->carrier < 0 && (c->x - s->star_x) * (c->x - s->star_x) + (c->z - s->star_z) * (c->z - s->star_z) < 2.5f * 2.5f) {
            s->star_state = 2;
            say(s, "THE CAPTAIN HAS THE STAR - TAKE IT HOME TO THE HEARTH");
            sfx(s, CADV_SFX_PICKUP, 1.0f);
        }
        if (s->star_state == 2) {
            int hall = find_hall(s);
            s->star_x = c->x;
            s->star_z = c->z;
            if (hall >= 0) {
                cadv_vec3 h = centre_of(s, hall);
                if (c->carrier < 0 && (c->x - h.x) * (c->x - h.x) + (c->z - h.z) * (c->z - h.z) < 7.0f * 7.0f) {
                    s->star_state = 3;
                    s->win_timer = 300;
                    say(s, "THE MIDWINTER FIRE IS LIT - SHINING STAR!");
                    sfx(s, CADV_SFX_STAR, 1.0f);
                }
            }
        }
    }
}

static void events(p05_state *s)
{
    cadv_rts *r = &s->rts;
    uint32_t ev = r->events[P05_GNOMES];
    int i;
    if (ev & CADV_RTS_EV_BUILT) {
        s->placing = true;
        say(s, "READY - POINT AND FIRE TO PLACE IT");
        sfx(s, CADV_SFX_BELL, 0.6f);
    }
    if (ev & CADV_RTS_EV_UNIT_READY) {
        sfx(s, CADV_SFX_PICKUP, 0.5f);
        /* new workers go where the village is shortest */
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            cadv_rts_unit *u = &r->u[i];
            if (u->alive && u->owner == P05_GNOMES && u->def == P05_U_WORKER && !s->was_alive[i]) {
                bool tallow_needed = (cadv_rts_count_buildings(r, P05_GNOMES, P05_B_SHIPYARD) > 0 ||
                                      cadv_rts_count_buildings(r, P05_GNOMES, P05_B_WORKSHOP) > 0) &&
                                     r->side[P05_GNOMES].tallow < 160;
                u->res_kind = tallow_needed ? 2 : (r->side[P05_GNOMES].lumber < r->side[P05_GNOMES].credits ? 1 : 0);
                u->res_cell.x = -1;
            }
        }
    }
    if (ev & CADV_RTS_EV_BUILDING_LOST) {
        say(s, "A BUILDING HAS FALLEN");
        if (!s->thawed) {
            s->building_lost_in_raids = true;
        }
    }
    if (ev & CADV_RTS_EV_FELLED) {
        sfx(s, CADV_SFX_HIT, 0.2f);
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        s->was_alive[i] = r->u[i].alive;
    }
    if (s->checkpoint < 1 && s->raid >= 1 && cadv_p05_raiders_alive(s) == 0) {
        s->checkpoint = 1;
    }
}

/* The village foreman: workers whose resource ran out move to another one,
 * and once boats or engines are wanted two of them tend the tallow fire. */
static void foreman(p05_state *s)
{
    cadv_rts *r = &s->rts;
    int hall = find_hall(s);
    bool avail[CADV_RTS_RESOURCES];
    int count[CADV_RTS_RESOURCES] = {0, 0, 0};
    bool want_tallow;
    int k;
    int i;
    cadv_cell from;
    cadv_cell dummy;
    if (hall < 0) {
        return;
    }
    from.x = r->b[hall].x;
    from.z = r->b[hall].z;
    for (k = 0; k < CADV_RTS_RESOURCES; ++k) {
        avail[k] = cadv_rts_nearest_resource(&r->grid, from.x, from.z, k, &dummy);
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (u->alive && u->owner == P05_GNOMES && u->def == P05_U_WORKER && u->order == CADV_ORDER_HARVEST) {
            ++count[u->res_kind];
        }
    }
    want_tallow = avail[2] && r->side[P05_GNOMES].tallow < 240 &&
                  (cadv_rts_count_buildings(r, P05_GNOMES, P05_B_SHIPYARD) > 0 || cadv_rts_count_buildings(r, P05_GNOMES, P05_B_WORKSHOP) > 0);
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        cadv_rts_unit *u = &r->u[i];
        int to = -1;
        if (!u->alive || u->owner != P05_GNOMES || u->def != P05_U_WORKER || u->carrier >= 0) {
            continue;
        }
        if (u->order == CADV_ORDER_HARVEST && !avail[u->res_kind]) {
            to = avail[1] ? 1 : (avail[0] ? 0 : (avail[2] ? 2 : -1));
        } else if (want_tallow && count[2] < 2 && u->order == CADV_ORDER_HARVEST && u->res_kind != 2 &&
                   count[u->res_kind] >= count[u->res_kind == 0 ? 1 : 0]) {
            to = 2;
        } else if (u->order == CADV_ORDER_IDLE && u->path_len == 0 && !s->selected[i]) {
            to = avail[1] ? 1 : (avail[0] ? 0 : (avail[2] ? 2 : -1));
        }
        if (to >= 0 && to != u->res_kind) {
            --count[u->res_kind];
            ++count[to];
            u->res_kind = to;
            u->res_cell.x = -1;
            cadv_rts_order_harvest(r, i);
        } else if (to >= 0 && u->order == CADV_ORDER_IDLE) {
            cadv_rts_order_harvest(r, i);
        }
    }
}

void cadv_p05_tick(void *state, const cadv_input_frame *in)
{
    p05_state *s = (p05_state *)state;
    cadv_rts *r = &s->rts;
    if (s->outcome != CADV_OUTCOME_RUNNING) {
        return;
    }
    ++s->ticks;
    s->owl_t += P05_DT;
    if (s->message_timer > 0) {
        --s->message_timer;
    }
    if (s->quip_timer > 0) {
        --s->quip_timer;
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
    if (s->captain >= 0) {
        walk(s, in);
        stave(s, in);
        command(s, in);
    } else if (s->respawn > 0 && --s->respawn == 0) {
        int hall = find_hall(s);
        cadv_vec3 c = centre_of(s, hall);
        spawn_captain(s, c.x, c.z + 7.0f);
        say(s, "THE CAPTAIN RETURNS TO THE HALL");
    }
    /* raids, the ward and the thaw */
    if (s->raid < CADV_P05_RAIDS && --s->raid_timer <= 0) {
        send_raid(s);
    }
    if (!s->thawed && s->raid >= CADV_P05_RAIDS && cadv_p05_raiders_alive(s) == 0) {
        thaw(s);
    }
    if (!s->thawed && s->longhall >= 0 && r->b[s->longhall].alive) {
        r->b[s->longhall].hp = k_buildings[P05_B_LONGHALL].hp; /* warded until spring */
    }
    if (s->keep_timer > 0 && --s->keep_timer == 0) {
        int hall = find_hall(s);
        if (hall >= 0) {
            r->b[hall].def = P05_B_KEEP;
            r->b[hall].hp = k_buildings[P05_B_KEEP].hp;
            cadv_rts_update_supply(r, P05_GNOMES);
        }
        s->keep = true;
        say(s, "THE KEEP STANDS - KNIGHTS CAN BE TRAINED");
    }
    if (s->ticks % 60 == 0) {
        foreman(s);
    }
    if (s->star_state == 0) {
        cadv_rts_ai_tick(r, &s->ai);
    }
    cadv_rts_tick(r);
    {
        /* a fallen raider's slot may be reused by a new unit */
        int i;
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            if (s->raider[i] && (!r->u[i].alive || r->u[i].owner != P05_TROLLS)) {
                s->raider[i] = 0;
            }
        }
    }
    if (s->captain >= 0) {
        cadv_rts_reveal(r, P05_GNOMES, r->u[s->captain].x, r->u[s->captain].z, 7);
    }
    /* the snow-owl scout circles over the lake */
    cadv_rts_reveal(r, P05_GNOMES, cadv_p05_owl(s).x, cadv_p05_owl(s).z, 4);
    events(s);
    star_rules(s);
    check_marks(s);
    if (s->captain >= 0 && !r->u[s->captain].alive) {
        s->captain = -1;
        s->hero_fell = true;
        if (s->star_state == 2) {
            s->star_state = 1; /* dropped where the Captain fell */
        }
        s->respawn = 360;
        say(s, "THE CAPTAIN IS DOWN - CARRIED BACK TO THE HALL");
    }
    if (find_hall(s) < 0 && s->star_state < 3) {
        s->lose_timer = 180;
        say(s, "THE HALL HAS FALLEN - KINDLEWICK IS LOST");
    }
}

cadv_vec3 cadv_p05_owl(const p05_state *s)
{
    float a = s->owl_t * 0.25f;
    return cadv_v3(cc(31) + 40.0f * cadv_cosf(a), 14.0f, cc(29) + 24.0f * cadv_sinf(a));
}

/* ---- results & scripting ------------------------------------------------------------------ */

cadv_portal_result cadv_p05_result(const void *state)
{
    const p05_state *s = (const p05_state *)state;
    cadv_portal_result res;
    memset(&res, 0, sizeof res);
    res.outcome = s->outcome;
    res.star_collected = s->star_state == 3;
    res.checkpoint = s->checkpoint;
    res.marks_found = s->marks_found;
    res.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        res.snowflakes = 1 + (s->building_lost_in_raids ? 0 : 1) +
                         (!s->hero_fell && s->rts.side[P05_GNOMES].units_built <= 30 ? 1 : 0);
    }
    return res;
}

static int army_count(const p05_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        n += is_army(s, i);
    }
    return n;
}

bool cadv_p05_query(const void *state, const char *key, double *out)
{
    const p05_state *s = (const p05_state *)state;
    const cadv_rts *r = &s->rts;
    cadv_vec3 e = cadv_p05_eye(s);
    struct kv { const char *k; double v; };
    const struct kv table[] = {
        {"player.x", e.x},
        {"player.y", e.y},
        {"player.z", e.z},
        {"player.yaw", s->yaw},
        {"player.health", s->captain >= 0 ? r->u[s->captain].hp : 0},
        {"gold", r->side[P05_GNOMES].credits},
        {"lumber", r->side[P05_GNOMES].lumber},
        {"tallow", r->side[P05_GNOMES].tallow},
        {"supply", r->side[P05_GNOMES].supply_used},
        {"supply.cap", r->side[P05_GNOMES].supply_cap},
        {"buildings", cadv_rts_count_buildings(r, P05_GNOMES, -1)},
        {"workers", cadv_rts_count_units(r, P05_GNOMES, P05_U_WORKER)},
        {"army", army_count(s)},
        {"ferries", cadv_rts_count_units(r, P05_GNOMES, P05_U_FERRY)},
        {"raid", s->raid},
        {"raiders", cadv_p05_raiders_alive(s)},
        {"thawed", s->thawed},
        {"keep", s->keep},
        {"longhall.hp", s->longhall >= 0 && r->b[s->longhall].alive ? r->b[s->longhall].hp : 0},
        {"star.state", s->star_state},
        {"aboard", s->captain >= 0 && r->u[s->captain].carrier >= 0},
        {"selected", count_selected(s)},
        {"placing", s->placing},
        {"stave", s->stave_item},
        {"built", r->side[P05_GNOMES].units_built},
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

void cadv_p05_teleport(void *state, float x, float y, float z, float yaw)
{
    p05_state *s = (p05_state *)state;
    (void)y;
    if (s->captain >= 0) {
        s->rts.u[s->captain].x = x;
        s->rts.u[s->captain].z = z;
    }
    s->yaw = yaw;
    s->pitch = 0.0f;
}

/* ---- completion bot -------------------------------------------------------------------- */

typedef struct bot_spot {
    int def;
    int x, z;
    bool spring; /* only after the thaw */
} bot_spot;

static const bot_spot k_bot_base[] = {
    {P05_B_FARM, 26, 51, false}, {P05_B_MILL, 22, 47, false}, {P05_B_BARRACKS, 33, 46, false}, {P05_B_FARM, 26, 54, false},
    {P05_B_TOWER, 30, 46, false}, {P05_B_FARM, 35, 54, false}, {P05_B_TOWER, 34, 43, false}, {P05_B_FARM, 35, 57, false},
    {P05_B_FARM, 38, 54, false}, {P05_B_SHIPYARD, 36, 34, true}, {P05_B_WORKSHOP, 34, 50, true}, {P05_B_FARM, 38, 57, true},
    {P05_B_FARM, 41, 54, true},
};
#define BOT_SPOTS ((int)(sizeof k_bot_base / sizeof k_bot_base[0]))

static void bot_press(cadv_input_frame *out, cadv_action a)
{
    out->pressed |= CADV_ACT_BIT(a);
    out->held |= CADV_ACT_BIT(a);
}

static int stave_of_struct(int def)
{
    int i;
    for (i = 0; i < P05_STAVE_STRUCTURES; ++i) {
        if (k_stave_struct[i] == def) {
            return i;
        }
    }
    return 0; /* GCOVR_EXCL_LINE */
}

static int stave_of_unit(int def)
{
    int i;
    for (i = 0; i < P05_STAVE_UNITS; ++i) {
        if (k_stave_unit[i] == def) {
            return P05_STAVE_STRUCTURES + i;
        }
    }
    return P05_STAVE_STRUCTURES; /* GCOVR_EXCL_LINE */
}

static void bot_buy(const p05_state *s, int item, cadv_input_frame *out)
{
    if (s->stave_item != item) {
        int fwd = (item - s->stave_item + P05_STAVE_ITEMS) % P05_STAVE_ITEMS;
        bot_press(out, fwd <= P05_STAVE_ITEMS / 2 ? CADV_ACT_NEXT_WEAPON : CADV_ACT_PREV_WEAPON);
        return;
    }
    bot_press(out, CADV_ACT_USE);
}

static float bot_aim(const p05_state *s, cadv_vec3 p, cadv_input_frame *out)
{
    cadv_vec3 e = cadv_p05_eye(s);
    cadv_vec3 to = cadv_v3_sub(p, e);
    float want_yaw = cadv_atan2f(-to.x, -to.z);
    float want_pitch = cadv_atan2f(to.y, cadv_sqrtf(to.x * to.x + to.z * to.z));
    float dy = cadv_wrap_angle(want_yaw - s->yaw);
    float dp = want_pitch - s->pitch;
    out->look_x = cadv_clampf(dy, -0.15f, 0.15f);
    out->look_y = cadv_clampf(dp, -0.1f, 0.1f);
    return cadv_absf(dy) + cadv_absf(dp);
}

/* Aim at a ground cell; true once the crosshair really lands within a cell of
 * it (grazing angles make an angular tolerance far too loose at range). */
static bool bot_aim_cell(const p05_state *s, int cx, int cz, cadv_input_frame *out)
{
    float gx;
    float gz;
    (void)bot_aim(s, cadv_v3(cc(cx), 0, cc(cz)), out);
    return cadv_p05_aim_ground(s, &gx, &gz) && cadv_absf(gx - cc(cx)) < P05_CELL && cadv_absf(gz - cc(cz)) < P05_CELL;
}

static float bot_walk(const p05_state *s, float cx, float cz, cadv_input_frame *out)
{
    cadv_vec3 e = cadv_p05_eye(s);
    float tx = cx * P05_CELL;
    float tz = cz * P05_CELL;
    float d = cadv_sqrtf((tx - e.x) * (tx - e.x) + (tz - e.z) * (tz - e.z));
    float want;
    float dy;
    if (d > 2.0f * P05_CELL) {
        /* walk around buildings and trees along the grid path */
        cadv_cell path[256];
        cadv_cell a;
        cadv_cell b;
        int n;
        a.x = (int)(e.x / P05_CELL);
        a.z = (int)(e.z / P05_CELL);
        b.x = (int)cx;
        b.z = (int)cz;
        n = cadv_rts_path(&s->rts.grid, a, b, path, 256);
        if (n >= 2) {
            int k = n > 2 ? 1 : 0;
            tx = cc(path[k].x);
            tz = cc(path[k].z);
        }
    }
    want = cadv_atan2f(-(tx - e.x), -(tz - e.z));
    dy = cadv_wrap_angle(want - s->yaw);
    out->look_x = cadv_clampf(dy, -0.15f, 0.15f);
    out->look_y = cadv_clampf(-s->pitch, -0.1f, 0.1f);
    out->move_y = d > 0.3f && cadv_absf(dy) < 0.6f ? cadv_minf(1.0f, d) : 0.0f;
    if (d > 6.0f) {
        out->held |= CADV_ACT_BIT(CADV_ACT_RUN);
    }
    return d;
}

static bool bot_standing(const p05_state *s, const bot_spot *b)
{
    int i = cadv_rts_building_at(&s->rts, b->x, b->z);
    return i >= 0 && s->rts.b[i].alive && s->rts.b[i].def == b->def && s->rts.b[i].owner == P05_GNOMES;
}

static bool affordable(const p05_state *s, int gold, int lumber, int tallow, int keep_gold, int keep_lumber)
{
    const cadv_rts_side *me = &s->rts.side[P05_GNOMES];
    /* money saved for a building only holds back the resources a unit needs */
    return (gold == 0 || me->credits - keep_gold >= gold) && (lumber == 0 || me->lumber - keep_lumber >= lumber) && me->tallow >= tallow;
}

static bool bot_economy(const p05_state *s, cadv_input_frame *out)
{
    const cadv_rts *r = &s->rts;
    const cadv_rts_side *me = &r->side[P05_GNOMES];
    int i;
    int reserve_g = 0;
    int reserve_l = 0;
    if (me->ready_building >= 0) {
        for (i = 0; i < BOT_SPOTS; ++i) {
            const bot_spot *b = &k_bot_base[i];
            if (b->def == me->ready_building && !bot_standing(s, b)) {
                const cadv_rts_bdef *d = &k_buildings[b->def];
                cadv_vec3 p = cadv_v3(cc(b->x + d->fw / 2), 0, cc(b->z + d->fh / 2));
                if (!s->placing) {
                    bot_buy(s, stave_of_struct(b->def), out);
                } else if (bot_aim(s, p, out) < 0.01f) {
                    bot_press(out, CADV_ACT_FIRE);
                }
                return true;
            }
        }
    }
    if (me->queued[0] == 0 && me->ready_building < 0) {
        for (i = 0; i < BOT_SPOTS; ++i) {
            const bot_spot *b = &k_bot_base[i];
            if (!bot_standing(s, b) && (!b->spring || s->thawed)) {
                const cadv_rts_bdef *d = &k_buildings[b->def];
                if (affordable(s, d->cost, d->lumber, d->tallow, 0, 0)) {
                    bot_buy(s, stave_of_struct(b->def), out);
                    return true;
                }
                reserve_g = d->cost;
                reserve_l = d->lumber;
                break;
            }
        }
    }
    if (me->queued[1] < 2) {
        int want = -1;
        int army = army_count(s);
        int workers = cadv_rts_count_units(r, P05_GNOMES, P05_U_WORKER);
        if (workers < 9) {
            want = P05_U_WORKER;
        } else if (s->thawed && cadv_rts_can_train(r, P05_GNOMES, P05_U_FERRY) && cadv_rts_count_units(r, P05_GNOMES, P05_U_FERRY) == 0) {
            want = P05_U_FERRY;
        } else if (s->thawed && cadv_rts_can_train(r, P05_GNOMES, P05_U_BALLISTA) && cadv_rts_count_units(r, P05_GNOMES, P05_U_BALLISTA) < 2) {
            want = P05_U_BALLISTA;
        } else if (cadv_rts_can_train(r, P05_GNOMES, P05_U_FOOTMAN) && army < (s->thawed ? 16 : 11)) {
            want = (army % 3 == 2) ? P05_U_ARCHER : P05_U_FOOTMAN;
        }
        if (want >= 0) {
            const cadv_rts_udef *u = &k_units[want];
            if (affordable(s, u->cost, u->lumber, u->tallow, want == P05_U_WORKER ? 0 : reserve_g, want == P05_U_WORKER ? 0 : reserve_l) &&
                me->supply_cap - me->supply_used >= u->supply) {
                bot_buy(s, stave_of_unit(want), out);
                return true;
            }
        }
    }
    return false;
}

static int find_ferry(const p05_state *s)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (s->rts.u[i].alive && s->rts.u[i].owner == P05_GNOMES && s->rts.u[i].def == P05_U_FERRY) {
            return i;
        }
    }
    return -1;
}

static int south_army(const p05_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &s->rts.u[i];
        n += is_army(s, i) && u->carrier < 0 && u->z > (float)(P05_LAKE_Z1 + 1) * P05_CELL;
    }
    return n;
}

/* Select one unit by aiming at it (clears the squad first). The ferry is
 * kept on group 4 once picked, since soldiers crowd in front of it. */
static bool bot_pick(const p05_state *s, int unit, int phase, cadv_input_frame *out)
{
    const cadv_rts_unit *u = &s->rts.u[unit];
    bool grouped = s->rts.side[P05_GNOMES].group[3][unit] != 0;
    if (s->selected[unit] && count_selected(s) == 1) {
        if (!grouped && u->def == P05_U_FERRY) {
            out->held |= CADV_ACT_BIT(CADV_ACT_RUN);
            bot_press(out, CADV_ACT_WEAPON_4);
            return false;
        }
        return true;
    }
    if (grouped) {
        if (phase % 6 == 0) {
            bot_press(out, CADV_ACT_WEAPON_4);
        }
        return false;
    }
    if (count_selected(s) > 0) {
        bot_press(out, CADV_ACT_CROUCH);
        return false;
    }
    if (bot_aim(s, cadv_v3(u->x, 1.0f, u->z), out) < 0.03f && phase % 6 == 0) {
        bot_press(out, CADV_ACT_FIRE);
    }
    return false;
}

void cadv_p05_bot(const void *state, cadv_input_frame *out)
{
    const p05_state *s = (const p05_state *)state;
    const cadv_rts *r = &s->rts;
    const cadv_rts_unit *c;
    cadv_vec3 e;
    int phase = s->ticks % 120;
    int ferry;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0) {
        if (s->win_timer < 170) {
            bot_press(out, CADV_ACT_USE);
        }
        return;
    }
    if (s->captain < 0) {
        return;
    }
    c = &r->u[s->captain];
    e = cadv_p05_eye(s);
    ferry = find_ferry(s);
    /* the star: carry it to the hearth */
    if (s->star_state >= 1) {
        float cz = c->z / P05_CELL;
        if (s->star_state == 1 && c->carrier < 0 && cz < (float)P05_LAKE_Z0) {
            bot_walk(s, s->star_x / P05_CELL, s->star_z / P05_CELL, out);
        } else if (c->carrier >= 0) {
            /* aboard: point the boat at the home shore */
            if (r->u[c->carrier].order == CADV_ORDER_IDLE && r->u[c->carrier].z < (float)P05_LAKE_Z1 * P05_CELL) {
                if (bot_aim_cell(s, 31, 36, out) && phase % 20 == 0) {
                    bot_press(out, CADV_ACT_FIRE);
                }
            }
        } else if (cz < (float)P05_LAKE_Z0) {
            /* north shore with the star: get the ferry over and board */
            if (ferry >= 0 && (r->u[ferry].x - c->x) * (r->u[ferry].x - c->x) + (r->u[ferry].z - c->z) * (r->u[ferry].z - c->z) < (2.4f * P05_CELL) * (2.4f * P05_CELL)) {
                if (phase % 30 == 0) {
                    bot_press(out, CADV_ACT_JUMP);
                }
            } else if (ferry >= 0 && r->u[ferry].z < cc(28)) {
                bot_walk(s, r->u[ferry].x / P05_CELL, r->u[ferry].z / P05_CELL - 1.5f, out);
            } else if (bot_walk(s, 31.5f, 22.5f, out) < 1.5f && ferry >= 0) {
                out->move_y = 0.0f;
                if (r->u[ferry].z > cc(26) && r->u[ferry].order == CADV_ORDER_IDLE && bot_pick(s, ferry, phase, out)) {
                    if (bot_aim_cell(s, 31, 22, out) && phase % 20 == 0) {
                        bot_press(out, CADV_ACT_FIRE);
                    }
                }
            }
        } else {
            bot_walk(s, 31.5f, 54.5f, out); /* home to the hearth */
        }
        return;
    }
    /* before the thaw (and while building up after it): the village */
    if (!s->thawed || ferry < 0 || army_count(s) < 12 ||
        (cadv_rts_count_units(r, P05_GNOMES, P05_U_BALLISTA) < 2 && r->side[P05_GNOMES].supply_cap - r->side[P05_GNOMES].supply_used >= 3)) {
        if (cadv_p05_raiders_alive(s) > 0 && phase < 30) {
            /* call the army to the hall when raiders come */
            if (phase == 0) {
                bot_press(out, CADV_ACT_WEAPON_5);
            } else if (bot_aim(s, cadv_v3(cc(31), 0, cc(47)), out) < 0.02f && phase % 10 == 1) {
                bot_press(out, CADV_ACT_FIRE);
            }
            return;
        }
        if (count_selected(s) > 0 && phase >= 30) {
            bot_press(out, CADV_ACT_CROUCH);
        }
        if (bot_economy(s, out)) {
            return;
        }
        bot_walk(s, 31.5f, 55.5f, out);
        return;
    }
    /* the crossing: shuttle the army north, the Captain last */
    if (r->side[P05_GNOMES].group[3][ferry] == 0) {
        if (bot_walk(s, r->u[ferry].x / P05_CELL, (float)P05_LAKE_Z1 + 3.5f, out) < 1.5f) {
            out->move_y = 0.0f;
            (void)bot_pick(s, ferry, phase, out);
        }
        return;
    }
    {
        int south = south_army(s);
        const cadv_rts_unit *f = &r->u[ferry];
        bool ferry_south = f->z > cc(28);
        bool captain_south = c->z > cc(P05_LAKE_Z1);
        if (phase >= 90 && bot_economy(s, out)) {
            return;
        }
        if (captain_south && c->carrier < 0) {
            if (south > 0 || f->passengers >= k_units[P05_U_FERRY].capacity || !ferry_south) {
                /* load soldiers / send the boat */
                if (bot_walk(s, 31.5f, 37.5f, out) > 1.5f) {
                    return;
                }
                out->move_y = 0.0f;
                if (ferry_south && f->order == CADV_ORDER_IDLE && south > 0 && f->passengers < k_units[P05_U_FERRY].capacity) {
                    if (phase == 0) {
                        bot_press(out, CADV_ACT_WEAPON_5);
                    } else if (phase < 40 && bot_aim(s, cadv_v3(f->x, 1.0f, f->z), out) < 0.03f && phase % 10 == 1) {
                        bot_press(out, CADV_ACT_FIRE);
                    }
                } else if (f->order == CADV_ORDER_IDLE && bot_pick(s, ferry, phase, out)) {
                    if (bot_aim_cell(s, 31, ferry_south ? 22 : 35, out) && phase % 20 == 0) {
                        bot_press(out, CADV_ACT_FIRE);
                    }
                }
                return;
            }
            /* everyone is across: the Captain boards */
            if ((f->x - c->x) * (f->x - c->x) + (f->z - c->z) * (f->z - c->z) < (2.4f * P05_CELL) * (2.4f * P05_CELL)) {
                if (phase % 30 == 0) {
                    bot_press(out, CADV_ACT_JUMP);
                }
            } else {
                bot_walk(s, f->x / P05_CELL, f->z / P05_CELL + 1.5f, out);
            }
            return;
        }
        if (c->carrier >= 0) {
            if (f->order == CADV_ORDER_IDLE && bot_aim_cell(s, 31, 22, out) && phase % 20 == 0) {
                bot_press(out, CADV_ACT_FIRE);
            }
            return;
        }
        /* north: storm the Longhall, the Captain behind the line */
        if (phase == 0) {
            bot_press(out, CADV_ACT_WEAPON_5);
        } else if (phase < 40 && s->longhall >= 0) {
            cadv_vec3 lh = centre_of(s, s->longhall);
            float d = cadv_sqrtf((lh.x - e.x) * (lh.x - e.x) + (lh.z - e.z) * (lh.z - e.z));
            float k = d > 35.0f ? 35.0f / d : 1.0f;
            int tgt = cadv_p05_aim_target(s, false);
            if (bot_aim(s, d < 35.0f ? cadv_v3(lh.x, 2.0f, lh.z) : cadv_v3(e.x + (lh.x - e.x) * k, 0, e.z + (lh.z - e.z) * k), out) < 0.03f &&
                phase % 10 == 1) {
                (void)tgt;
                bot_press(out, CADV_ACT_FIRE);
            }
        } else {
            bot_walk(s, 31.5f, 20.5f, out);
        }
    }
}
