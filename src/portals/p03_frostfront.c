/* SPDX-License-Identifier: MIT */
#include "portals/p03_frostfront.h"

#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "core/mathx.h"
#include "hub/friends.h"
#include "portals/p03_internal.h"

/* ---- tables (original units and structures) ---------------------------------------- */

static const cadv_rts_bdef k_buildings[P03_BUILDINGS] = {
    /* name             fw fh cost  pow  hp    ticks flags                range dmg cool sight */
    {"Mobile Yard", 3, 3, 0, 10, 1500, 1, CADV_BF_HQ, 0, 0, 0, 7, 0, 0, 0},
    {"Power Plant", 2, 2, 300, 100, 400, 300, 0, 0, 0, 0, 4, 0, 0, 0},
    {"Refinery", 3, 2, 1200, -30, 900, 600, CADV_BF_REFINERY, 0, 0, 0, 5, 0, 0, 0},
    {"Barracks", 2, 2, 300, -20, 500, 360, CADV_BF_BARRACKS, 0, 0, 0, 5, 0, 0, 0},
    {"War Factory", 3, 3, 1200, -40, 1000, 720, CADV_BF_FACTORY, 0, 0, 0, 5, 0, 0, 0},
    {"Guard Tower", 1, 1, 500, -20, 400, 360, CADV_BF_DEFENCE, 15.0f, 12, 30, 6, 0, 0, 0},
    {"Ember Spire", 3, 3, 0, 0, 2400, 1, CADV_BF_HQ, 0, 0, 0, 8, 0, 0, 0},
    {"Lancer Pylon", 1, 1, 0, -30, 500, 1, CADV_BF_DEFENCE, 16.0f, 18, 60, 7, 0, 0, 0},
    {"Research Lab", 3, 2, 0, 0, 800, 1, 0, 0, 0, 0, 4, 0, 0, 0},
};

static const cadv_rts_udef k_units[P03_UNITS] = {
    /* name           cost  hp   speed  range dmg cool ticks harv  carry from               sight infantry */
    {"Rifleman", 100, 60, 0.060f, 12.0f, 7, 30, 300, false, 0, CADV_BF_BARRACKS, 5, true, 0, 0, 0, 0, false, 0},
    {"Rocket Trooper", 250, 50, 0.055f, 18.0f, 30, 90, 420, false, 0, CADV_BF_BARRACKS, 5, true, 0, 0, 0, 0, false, 0},
    {"Engineer", 400, 40, 0.060f, 0.0f, 0, 0, 420, false, 0, CADV_BF_BARRACKS, 4, true, 0, 0, 0, 0, false, 0},
    {"Harvester", 1200, 600, 0.090f, 0.0f, 0, 0, 900, true, 500, CADV_BF_FACTORY, 4, false, 0, 0, 0, 0, false, 0},
    {"Light Tank", 600, 350, 0.100f, 16.0f, 25, 60, 600, false, 0, CADV_BF_FACTORY, 6, false, 0, 0, 0, 0, false, 0},
    {"Recon Buggy", 450, 160, 0.160f, 12.0f, 8, 20, 360, false, 0, CADV_BF_FACTORY, 10, false, 0, 0, 0, 0, false, 0},
    {"Commander", 0, 300, 0.0f, 0.0f, 0, 0, 1, false, 0, CADV_BF_HQ, 7, true, 0, 0, 0, 0, false, 0},
};

static const int k_tab_struct[P03_TAB_STRUCTURES] = {P03_B_POWER, P03_B_REFINERY, P03_B_BARRACKS, P03_B_FACTORY, P03_B_TOWER};
static const int k_tab_unit[P03_TAB_ITEMS - P03_TAB_STRUCTURES] = {P03_U_RIFLE, P03_U_ROCKET, P03_U_ENGINEER,
                                                                   P03_U_HARVESTER, P03_U_TANK, P03_U_BUGGY};

/* The Order of the Pale Sun: its base, rebuilt by the AI, and its unit mix. */
static const cadv_rts_ai_build k_order_plan[] = {
    {P03_B_POWER, 24, 6}, {P03_B_POWER, 37, 6}, {P03_B_REFINERY, 40, 9}, {P03_B_BARRACKS, 37, 12},
    {P03_B_FACTORY, 21, 10}, {P03_B_POWER, 24, 3},
};
static const int k_order_train[] = {P03_U_RIFLE, P03_U_RIFLE, P03_U_ROCKET, P03_U_TANK, P03_U_RIFLE, P03_U_TANK};

const cadv_rts_bdef *cadv_p03_buildings(void) { return k_buildings; }
const cadv_rts_udef *cadv_p03_units(void) { return k_units; }

const char *cadv_p03_tab_name(int item)
{
    return item < P03_TAB_STRUCTURES ? k_buildings[k_tab_struct[item]].name : k_units[k_tab_unit[item - P03_TAB_STRUCTURES]].name;
}

int cadv_p03_tab_cost(int item)
{
    return item < P03_TAB_STRUCTURES ? k_buildings[k_tab_struct[item]].cost : k_units[k_tab_unit[item - P03_TAB_STRUCTURES]].cost;
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

static void say(p03_state *s, const char *text)
{
    copy_text(s->message, sizeof s->message, text);
    s->message_timer = 180;
}

static void quip(p03_state *s, const char *text)
{
    copy_text(s->quip, sizeof s->quip, text);
    s->quip_timer = 240;
}

static void sfx(p03_state *s, cadv_sfx id, float vol)
{
    if (s->cfg.audio) {
        cadv_audio_sfx(s->cfg.audio, id, vol, 0.0f);
    }
}

static cadv_vec3 centre_of(const p03_state *s, int b)
{
    const cadv_rts_building *bb = &s->rts.b[b];
    const cadv_rts_bdef *d = &k_buildings[bb->def];
    return cadv_v3(((float)bb->x + 0.5f * (float)d->fw) * P03_CELL, 0, ((float)bb->z + 0.5f * (float)d->fh) * P03_CELL);
}

static float cell_centre(int c) { return ((float)c + 0.5f) * P03_CELL; }

/* ---- the battlefield ------------------------------------------------------------------ */

static void terrain_rect(p03_state *s, int x0, int z0, int x1, int z1, int kind)
{
    int x;
    int z;
    for (z = z0; z <= z1; ++z) {
        for (x = x0; x <= x1; ++x) {
            if (x >= 0 && z >= 0 && x < P03_W && z < P03_H) {
                s->terrain[z * P03_W + x] = (uint8_t)kind;
                if (kind == P03_T_WATER || kind == P03_T_ROCK) {
                    cadv_rts_set_blocked(&s->rts.grid, x, z, x, z);
                }
            }
        }
    }
}

static int unit_at(p03_state *s, int side, int def, int cx, int cz)
{
    return cadv_rts_add_unit(&s->rts, side, def, cell_centre(cx), cell_centre(cz));
}

static void spawn_commander(p03_state *s, float x, float z)
{
    s->commander = cadv_rts_add_unit(&s->rts, P03_ACCORD, P03_U_COMMANDER, x, z);
}

static void build_battlefield(p03_state *s)
{
    cadv_rts *r = &s->rts;
    int i;
    /* the frozen river and its bridge */
    terrain_rect(s, 0, 30, P03_W - 1, 32, P03_T_WATER);
    for (i = 30; i <= 32; ++i) {
        terrain_rect(s, 30, i, 33, i, P03_T_BRIDGE);
        r->grid.kind[cadv_rts_index(&r->grid, 30, i)] = CADV_CELL_OPEN;
        r->grid.kind[cadv_rts_index(&r->grid, 31, i)] = CADV_CELL_OPEN;
        r->grid.kind[cadv_rts_index(&r->grid, 32, i)] = CADV_CELL_OPEN;
        r->grid.kind[cadv_rts_index(&r->grid, 33, i)] = CADV_CELL_OPEN;
    }
    /* rocks */
    terrain_rect(s, 9, 44, 12, 47, P03_T_ROCK);
    terrain_rect(s, 52, 42, 55, 45, P03_T_ROCK);
    terrain_rect(s, 17, 19, 20, 22, P03_T_ROCK);
    terrain_rect(s, 50, 17, 52, 20, P03_T_ROCK);
    terrain_rect(s, 0, 0, P03_W - 1, 0, P03_T_ROCK);
    terrain_rect(s, 0, P03_H - 1, P03_W - 1, P03_H - 1, P03_T_ROCK);
    terrain_rect(s, 0, 0, 0, P03_H - 1, P03_T_ROCK);
    terrain_rect(s, P03_W - 1, 0, P03_W - 1, P03_H - 1, P03_T_ROCK);
    /* Frostite fields */
    cadv_rts_add_ore(&r->grid, 45, 52, 3, 300);
    cadv_rts_add_ore(&r->grid, 14, 39, 3, 300);
    cadv_rts_add_ore(&r->grid, 46, 12, 3, 300);
    cadv_rts_add_ore(&r->grid, 10, 12, 2, 300);
    /* the Order's base */
    s->spire = cadv_rts_add_building(r, P03_ORDER, P03_B_SPIRE, 30, 5);
    cadv_rts_add_building(r, P03_ORDER, P03_B_PYLON, 27, 9);
    cadv_rts_add_building(r, P03_ORDER, P03_B_PYLON, 35, 9);
    cadv_rts_add_building(r, P03_ORDER, P03_B_PYLON, 27, 3);
    cadv_rts_add_building(r, P03_ORDER, P03_B_PYLON, 35, 3);
    for (i = 0; i < (int)(sizeof k_order_plan / sizeof k_order_plan[0]); ++i) {
        cadv_rts_add_building(r, P03_ORDER, k_order_plan[i].def, k_order_plan[i].x, k_order_plan[i].z);
    }
    cadv_rts_add_building(r, P03_ORDER, P03_B_TOWER, 28, 27);
    cadv_rts_add_building(r, P03_ORDER, P03_B_TOWER, 35, 27);
    unit_at(s, P03_ORDER, P03_U_HARVESTER, 43, 12);
    /* "Bloodmaker squad" holds the bridge */
    unit_at(s, P03_ORDER, P03_U_RIFLE, 30, 27);
    unit_at(s, P03_ORDER, P03_U_RIFLE, 32, 27);
    unit_at(s, P03_ORDER, P03_U_ROCKET, 33, 26);
    unit_at(s, P03_ORDER, P03_U_TANK, 29, 13);
    unit_at(s, P03_ORDER, P03_U_TANK, 33, 13);
    unit_at(s, P03_ORDER, P03_U_ROCKET, 31, 12);
    /* the Dr. Doom Research Lab (neutral tech building) */
    s->lab = cadv_rts_add_building(r, CADV_RTS_NEUTRAL, P03_B_LAB, 44, 24);
    /* Polar Accord landfall */
    cadv_rts_add_building(r, P03_ACCORD, P03_B_YARD, 30, 54);
    cadv_rts_update_fog(r, P03_ACCORD);
}

static void setup_ai(p03_state *s)
{
    cadv_rts_ai *ai = &s->ai;
    cadv_rts_ai_init(ai, P03_ORDER);
    ai->plan = k_order_plan;
    ai->nplan = (int)(sizeof k_order_plan / sizeof k_order_plan[0]);
    ai->train = k_order_train;
    ai->ntrain = (int)(sizeof k_order_train / sizeof k_order_train[0]);
    ai->harvester_def = P03_U_HARVESTER;
    ai->home.x = 31;
    ai->home.z = 7;
    ai->home_radius = 14;
    ai->target.x = 31;
    ai->target.z = 52;
    switch (s->cfg.difficulty) {
    case CADV_DIFF_STORY:
        ai->income = 0;
        ai->wave_every = 240 * 60;
        ai->wave_size = 2;
        ai->wave_max = 4;
        ai->army_cap = 5;
        break;
    case CADV_DIFF_HARD:
        ai->income = 2;
        ai->wave_every = 110 * 60;
        ai->wave_size = 4;
        ai->wave_max = 9;
        ai->army_cap = 14;
        break;
    default:
        ai->income = 0;
        ai->wave_every = 150 * 60;
        ai->wave_size = 3;
        ai->wave_max = 6;
        ai->army_cap = 9;
        break;
    }
    ai->wave_timer = -(240 * 60 - ai->wave_every); /* first wave after four minutes */
    s->rts.side[P03_ORDER].credits = 1500;
}

static void place_player_base(p03_state *s)
{
    cadv_rts *r = &s->rts;
    int ref;
    cadv_rts_add_building(r, P03_ACCORD, P03_B_POWER, 26, 54);
    ref = cadv_rts_add_building(r, P03_ACCORD, P03_B_REFINERY, 34, 54);
    cadv_rts_add_building(r, P03_ACCORD, P03_B_BARRACKS, 26, 51);
    cadv_rts_add_building(r, P03_ACCORD, P03_B_POWER, 26, 57);
    cadv_rts_add_building(r, P03_ACCORD, P03_B_FACTORY, 34, 50);
    if (ref >= 0) {
        unit_at(s, P03_ACCORD, P03_U_HARVESTER, 37, 56);
    }
}

bool cadv_p03_init(void *state, const cadv_portal_cfg *cfg)
{
    p03_state *s = (p03_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    s->checkpoint = cfg->checkpoint;
    s->star_carrier = -1;
    if (!cadv_rts_init(&s->rts, P03_W, P03_H, P03_CELL, k_buildings, P03_BUILDINGS, k_units, P03_UNITS)) {
        return false; /* GCOVR_EXCL_LINE: fixed sizes */
    }
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
    build_battlefield(s);
    setup_ai(s);
    s->rts.side[P03_ACCORD].credits = cfg->difficulty == CADV_DIFF_STORY ? 5000 : 3500;
    spawn_commander(s, cell_centre(31), cell_centre(58));
    s->yaw = 0.0f;
    if (s->checkpoint >= 1) {
        place_player_base(s);
        s->rts.side[P03_ACCORD].credits = 3000;
        s->ticks = 6 * 3600;
        s->rts.ticks = s->ticks;
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        s->was_alive[i] = s->rts.u[i].alive;
    }
    quip(s, s->checkpoint >= 1 ? "Base is up. Time to push north." : "Commander on the ground. Yard deployed.");
    say(s, s->checkpoint >= 1 ? "TRAIN AN ARMY - TAKE THE BRIDGE" : "Q PICKS, E BUILDS - POWER FIRST");
    return true;
}

/* ---- aiming ----------------------------------------------------------------------------- */

cadv_vec3 cadv_p03_eye(const p03_state *s)
{
    float x = 0.0f;
    float z = 0.0f;
    float lift = 0.0f;
    if (s->commander >= 0) {
        x = s->rts.u[s->commander].x;
        z = s->rts.u[s->commander].z;
    }
    if (s->drone > 0) {
        lift = 30.0f * cadv_minf(1.0f, (float)(180 - s->drone) / 30.0f) * cadv_minf(1.0f, (float)s->drone / 30.0f);
    }
    return cadv_v3(x, P03_EYE + lift, z);
}

cadv_vec3 cadv_p03_forward(const p03_state *s)
{
    float pitch = s->drone > 0 ? cadv_minf(s->pitch, -0.9f) : s->pitch;
    return cadv_v3_from_yaw_pitch(s->yaw, pitch);
}

bool cadv_p03_aim_ground(const p03_state *s, float *x, float *z)
{
    cadv_vec3 e = cadv_p03_eye(s);
    cadv_vec3 f = cadv_p03_forward(s);
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
    return *x >= 0.0f && *z >= 0.0f && *x < (float)P03_W * P03_CELL && *z < (float)P03_H * P03_CELL;
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
    const float *d = &f.x;
    const float *a = &lo.x;
    const float *b = &hi.x;
    for (k = 0; k < 3; ++k) {
        if (cadv_absf(d[k]) < 1e-6f) {
            if (o[k] < a[k] || o[k] > b[k]) {
                return -1.0f;
            }
        } else {
            float ta = (a[k] - o[k]) / d[k];
            float tb = (b[k] - o[k]) / d[k];
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

int cadv_p03_aim_target(const p03_state *s, bool own)
{
    const cadv_rts *r = &s->rts;
    cadv_vec3 e = cadv_p03_eye(s);
    cadv_vec3 f = cadv_p03_forward(s);
    float best = 1e9f;
    int hit = -1;
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        bool mine = u->owner == P03_ACCORD;
        float t;
        if (!u->alive || i == s->commander || mine != own || u->owner == CADV_RTS_NEUTRAL ||
            (!mine && !cadv_rts_cell_visible(r, P03_ACCORD, u->x, u->z))) {
            continue;
        }
        t = ray_point(e, f, cadv_v3(u->x, k_units[u->def].infantry ? 0.9f : 1.0f, u->z), k_units[u->def].infantry ? 1.2f : 2.0f);
        if (t >= 0.0f && t < best) {
            best = t;
            hit = i;
        }
    }
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        const cadv_rts_bdef *d = &k_buildings[b->def];
        bool mine = b->owner == P03_ACCORD;
        float t;
        float h = b->def == P03_B_SPIRE ? 9.0f : (b->def == P03_B_PYLON ? 6.0f : 4.0f);
        if (!b->alive || mine != own) {
            continue;
        }
        t = ray_box(e, f, cadv_v3((float)b->x * P03_CELL, 0, (float)b->z * P03_CELL),
                    cadv_v3((float)(b->x + d->fw) * P03_CELL, h, (float)(b->z + d->fh) * P03_CELL));
        if (t >= 0.0f && t < best) {
            best = t;
            hit = CADV_RTS_TARGET_BUILDING + i;
        }
    }
    return hit;
}

/* ---- commander -------------------------------------------------------------------------- */

static bool walkable(const p03_state *s, float x, float z)
{
    cadv_cell c = cadv_rts_cell_at(&s->rts.grid, x, z);
    return cadv_rts_passable(&s->rts.grid, c.x, c.z);
}

static void walk(p03_state *s, const cadv_input_frame *in)
{
    cadv_rts_unit *u = &s->rts.u[s->commander];
    float speed = (cadv_input_down(in, CADV_ACT_RUN) ? 6.5f : 4.5f) * P03_DT;
    float fx = -cadv_sinf(s->yaw);
    float fz = -cadv_cosf(s->yaw);
    float mx = cadv_clampf(in->move_x, -1, 1);
    float my = cadv_clampf(in->move_y, -1, 1);
    float dx = (fx * my - fz * mx) * speed;
    float dz = (fz * my + fx * mx) * speed;
    s->yaw = cadv_wrap_angle(s->yaw + in->look_x);
    s->pitch = cadv_clampf(s->pitch + in->look_y, -1.3f, 1.2f);
    if (s->drone > 0) {
        return; /* the body stands still while the drone flies */
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

static int count_selected(const p03_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        n += s->selected[i] && s->rts.u[i].alive;
    }
    return n;
}

static bool is_army(const p03_state *s, int i)
{
    const cadv_rts_unit *u = &s->rts.u[i];
    return u->alive && u->owner == P03_ACCORD && i != s->commander && u->def != P03_U_HARVESTER && u->def != P03_U_ENGINEER;
}

static void select_where(p03_state *s, int def, bool army)
{
    int i;
    memset(s->selected, 0, sizeof s->selected);
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &s->rts.u[i];
        if (army ? is_army(s, i) : (u->alive && u->owner == P03_ACCORD && u->def == def)) {
            s->selected[i] = 1;
        }
    }
}

static void order_selected(p03_state *s, int target, float gx, float gz, bool ground, bool force_move)
{
    cadv_rts *r = &s->rts;
    cadv_cell goal = cadv_rts_cell_at(&r->grid, gx, gz);
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (!s->selected[i] || !u->alive) {
            continue;
        }
        ++n;
        if (u->def == P03_U_HARVESTER) {
            cadv_rts_order_harvest(r, i);
        } else if (!ground && target >= CADV_RTS_TARGET_BUILDING && u->def == P03_U_ENGINEER) {
            const cadv_rts_building *b = &r->b[target - CADV_RTS_TARGET_BUILDING];
            cadv_cell c = {b->x, b->z};
            cadv_rts_order_move(r, i, c, false); /* engineers walk up to it */
        } else if (!ground && k_units[u->def].damage > 0) {
            cadv_rts_order_attack(r, i, target);
        } else if (ground) {
            cadv_rts_order_move(r, i, goal, !force_move && k_units[u->def].damage > 0);
        }
    }
    if (n > 0) {
        say(s, ground ? (force_move ? "MOVING OUT" : "ACKNOWLEDGED - ADVANCING") : "TARGET ACQUIRED");
    }
}

static void tablet(p03_state *s, const cadv_input_frame *in)
{
    cadv_rts *r = &s->rts;
    cadv_rts_side *me = &r->side[P03_ACCORD];
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_RADIAL)) {
        s->tab_item = (s->tab_item + 1) % P03_TAB_ITEMS;
    }
    if (cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        s->tab_item = (s->tab_item + P03_TAB_ITEMS - 1) % P03_TAB_ITEMS;
    }
    if (cadv_input_pressed(in, CADV_ACT_MAP)) {
        s->tablet_open = !s->tablet_open;
    }
    if (!cadv_input_pressed(in, CADV_ACT_USE)) {
        return;
    }
    if (s->tab_item < P03_TAB_STRUCTURES) {
        if (me->ready_building >= 0) {
            s->placing = true;
            say(s, "PLACE IT WITH THE BEAM (FIRE)");
        } else if (cadv_rts_queue(r, P03_ACCORD, true, k_tab_struct[s->tab_item])) {
            say(s, "BUILDING");
        } else {
            say(s, me->queued[0] >= CADV_RTS_QUEUE ? "QUEUE FULL" : "INSUFFICIENT FUNDS");
            sfx(s, CADV_SFX_DENIED, 0.6f);
        }
    } else {
        int def = k_tab_unit[s->tab_item - P03_TAB_STRUCTURES];
        if (cadv_rts_queue(r, P03_ACCORD, false, def)) {
            say(s, "TRAINING");
        } else {
            say(s, !cadv_rts_can_train(r, P03_ACCORD, def) ? (k_units[def].from == CADV_BF_BARRACKS ? "NEEDS A BARRACKS" : "NEEDS A WAR FACTORY")
                   : me->queued[1] >= CADV_RTS_QUEUE ? "QUEUE FULL" : "INSUFFICIENT FUNDS");
            sfx(s, CADV_SFX_DENIED, 0.6f);
        }
    }
}

static void place(p03_state *s)
{
    cadv_rts *r = &s->rts;
    int def = r->side[P03_ACCORD].ready_building;
    const cadv_rts_bdef *d = &k_buildings[def];
    float gx;
    float gz;
    cadv_cell c;
    cadv_place_result res;
    if (!cadv_p03_aim_ground(s, &gx, &gz)) {
        say(s, "AIM THE BEAM AT THE GROUND");
        return;
    }
    c = cadv_rts_cell_at(&r->grid, gx, gz);
    res = cadv_rts_place_ready(r, P03_ACCORD, c.x - d->fw / 2, c.z - d->fh / 2);
    if (res == CADV_PLACE_OK) {
        s->placing = false;
        sfx(s, CADV_SFX_DOOR, 0.8f);
        say(s, "STRUCTURE PLACED");
        if (def == P03_B_REFINERY) {
            /* a refinery arrives with its harvester */
            unit_at(s, P03_ACCORD, P03_U_HARVESTER, c.x - d->fw / 2 + d->fw, c.z - d->fh / 2 + d->fh);
        }
    } else {
        say(s, res == CADV_PLACE_TOO_FAR ? "TOO FAR FROM THE BASE" : "CANNOT BUILD THERE");
        sfx(s, CADV_SFX_DENIED, 0.6f);
    }
}

static void command(p03_state *s, const cadv_input_frame *in)
{
    int i;
    float gx = 0.0f;
    float gz = 0.0f;
    bool ground = cadv_p03_aim_ground(s, &gx, &gz);
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
                cadv_rts_group_set(&s->rts, P03_ACCORD, i, members, n);
                say(s, "GROUP SET");
            } else {
                int k;
                memset(s->selected, 0, sizeof s->selected);
                for (k = 0; k < CADV_RTS_MAX_UNITS; ++k) {
                    s->selected[k] = s->rts.side[P03_ACCORD].group[i][k] && s->rts.u[k].alive;
                }
            }
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_WEAPON_5)) {
        select_where(s, 0, true);
    }
    if (cadv_input_pressed(in, CADV_ACT_WEAPON_6)) {
        select_where(s, P03_U_ENGINEER, false);
    }
    if (cadv_input_pressed(in, CADV_ACT_CROUCH)) {
        memset(s->selected, 0, sizeof s->selected);
    }
    if (cadv_input_pressed(in, CADV_ACT_JUMP) && s->drone == 0 && s->drone_cd == 0) {
        s->drone = 180;
        s->drone_cd = 20 * 60;
        say(s, "RECON DRONE UP");
    }
    if (cadv_input_pressed(in, CADV_ACT_ALT_FIRE)) {
        if (s->placing) {
            s->placing = false;
            say(s, "PLACEMENT PAUSED - E TO RESUME");
        } else if (ground) {
            int n = 0;
            memset(s->selected, 0, sizeof s->selected);
            for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
                const cadv_rts_unit *u = &s->rts.u[i];
                if (u->alive && u->owner == P03_ACCORD && i != s->commander && u->def != P03_U_HARVESTER &&
                    (u->x - gx) * (u->x - gx) + (u->z - gz) * (u->z - gz) < 12.0f * 12.0f) {
                    s->selected[i] = 1;
                    ++n;
                }
            }
            if (n > 0) {
                say(s, "UNITS SELECTED");
            }
        }
    }
    s->sidearm_cd -= P03_DT;
    if (!cadv_input_pressed(in, CADV_ACT_FIRE) && !(cadv_input_down(in, CADV_ACT_FIRE) && count_selected(s) == 0)) {
        return;
    }
    if (s->placing && s->rts.side[P03_ACCORD].ready_building >= 0) {
        if (cadv_input_pressed(in, CADV_ACT_FIRE)) {
            place(s);
        }
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_FIRE) && (count_selected(s) == 0 || cadv_input_down(in, CADV_ACT_RUN))) {
        /* pick a unit (RUN adds to the squad); with a squad selected FIRE orders */
        int own = cadv_p03_aim_target(s, true);
        if (own >= 0 && own < CADV_RTS_TARGET_BUILDING) {
            if (!cadv_input_down(in, CADV_ACT_RUN)) {
                memset(s->selected, 0, sizeof s->selected);
            }
            s->selected[own] = 1;
            say(s, k_units[s->rts.u[own].def].name);
            return;
        }
    }
    if (count_selected(s) > 0) {
        if (cadv_input_pressed(in, CADV_ACT_FIRE)) {
            int enemy = cadv_p03_aim_target(s, false);
            if (enemy >= 0) {
                order_selected(s, enemy, gx, gz, false, false);
            } else if (ground) {
                order_selected(s, -1, gx, gz, true, cadv_input_down(in, CADV_ACT_RUN));
            }
        }
    } else if (s->sidearm_cd <= 0.0f) {
        /* no squad selected: the commander's own sidearm */
        int enemy = cadv_p03_aim_target(s, false);
        s->sidearm_cd = 0.4f;
        sfx(s, CADV_SFX_SHOT, 0.4f);
        if (enemy >= 0) {
            cadv_rts_damage(&s->rts, enemy, 12);
            sfx(s, CADV_SFX_HIT, 0.5f);
        }
    }
}

/* ---- rules -------------------------------------------------------------------------------- */

static void crystal_burn(p03_state *s)
{
    int i;
    if (s->ticks % 60 != 0) {
        return;
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &s->rts.u[i];
        if (u->alive && k_units[u->def].infantry) {
            cadv_cell c = cadv_rts_cell_at(&s->rts.grid, u->x, u->z);
            if (cadv_rts_in_bounds(&s->rts.grid, c.x, c.z) && s->rts.grid.ore[cadv_rts_index(&s->rts.grid, c.x, c.z)] > 0) {
                cadv_rts_damage(&s->rts, i, 2); /* Frostite bites through boots */
            }
        }
    }
}

static void check_marks(p03_state *s)
{
    cadv_vec3 e = cadv_p03_eye(s);
    int i;
    if (!(s->marks_found & 1u) && s->lab >= 0) {
        cadv_vec3 c = centre_of(s, s->lab);
        if (s->lab_captured || ((e.x - c.x) * (e.x - c.x) + (e.z - c.z) * (e.z - c.z) < 20.0f * 20.0f)) {
            char buf[160];
            s->marks_found |= 1u;
            snprintf(buf, sizeof buf, "THE %s - A FRIEND'S MARK!", s->mark_text[0]);
            say(s, buf);
        }
    }
    if (!(s->marks_found & 2u)) {
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            const cadv_rts_unit *u = &s->rts.u[i];
            if (u->alive && u->owner == P03_ACCORD && u->z < cell_centre(36) && u->z > cell_centre(26) &&
                cadv_absf(u->x - cell_centre(31)) < 18.0f) {
                char buf[160];
                s->marks_found |= 2u;
                snprintf(buf, sizeof buf, "RADIO: \"%s!\"", s->mark_text[1]);
                quip(s, buf);
                break;
            }
        }
    }
    if (!(s->marks_found & 4u) && cadv_rts_count_units(&s->rts, P03_ACCORD, P03_U_BUGGY) > 0) {
        char buf[160];
        s->marks_found |= 4u;
        snprintf(buf, sizeof buf, "BUGGY: \"%s, READY TO ROLL\"", s->mark_text[2]);
        quip(s, buf);
    }
}

static void capture_lab(p03_state *s)
{
    cadv_rts *r = &s->rts;
    const cadv_rts_building *lab;
    int i;
    if (s->lab < 0 || s->lab_captured || !r->b[s->lab].alive) {
        return;
    }
    lab = &r->b[s->lab];
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (u->alive && u->owner == P03_ACCORD && u->def == P03_U_ENGINEER) {
            cadv_cell c = cadv_rts_cell_at(&r->grid, u->x, u->z);
            if (c.x >= lab->x - 1 && c.x <= lab->x + 3 && c.z >= lab->z - 1 && c.z <= lab->z + 2) {
                int t;
                cadv_rts_capture(r, s->lab, P03_ACCORD);
                r->u[i].alive = false; /* the engineer stays inside to run it */
                s->selected[i] = 0;
                s->lab_captured = true;
                /* the lab's gift: a guard tower and a scan of the Order's base */
                t = cadv_rts_add_building(r, P03_ACCORD, P03_B_TOWER, lab->x + 3, lab->z + 2);
                (void)t;
                cadv_rts_reveal(r, P03_ACCORD, cell_centre(31), cell_centre(8), 12);
                say(s, "LAB CAPTURED - ENEMY BASE SCANNED");
                sfx(s, CADV_SFX_BELL, 1.0f);
                return;
            }
        }
    }
}

static void star_rules(p03_state *s)
{
    cadv_rts *r = &s->rts;
    int i;
    if (s->star_state == 0 && s->spire >= 0 && !r->b[s->spire].alive) {
        cadv_vec3 c = centre_of(s, s->spire);
        cadv_cell corner = {2, 2};
        /* the Spire fed the pylons; without it the Order scatters */
        for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
            if (r->b[i].alive && r->b[i].owner == P03_ORDER && r->b[i].def == P03_B_PYLON) {
                cadv_rts_destroy_building(r, i);
            }
        }
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            if (r->u[i].alive && r->u[i].owner == P03_ORDER && r->u[i].def != P03_U_HARVESTER) {
                cadv_rts_order_move(r, i, corner, false);
            }
        }
        s->star_state = 1;
        s->star_x = c.x;
        s->star_z = c.z;
        say(s, "SPIRE DOWN! SEND AN ENGINEER INTO THE VAULT (6 SELECTS)");
        quip(s, "The Pale Sun sets. Now bring me that star.");
        sfx(s, CADV_SFX_EXPLOSION, 1.0f);
    }
    if (s->star_state == 1) {
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            const cadv_rts_unit *u = &r->u[i];
            if (u->alive && u->owner == P03_ACCORD && u->def == P03_U_ENGINEER &&
                (u->x - s->star_x) * (u->x - s->star_x) + (u->z - s->star_z) * (u->z - s->star_z) < 4.5f * 4.5f) {
                s->star_state = 2;
                s->star_carrier = i;
                say(s, "ENGINEER HAS THE STAR - BRINGING IT TO YOU");
                break;
            }
        }
    } else if (s->star_state == 2) {
        const cadv_rts_unit *u = &r->u[s->star_carrier];
        if (!u->alive) {
            s->star_state = 1; /* dropped where the engineer fell */
            s->star_carrier = -1;
            say(s, "THE STAR FELL - SEND ANOTHER ENGINEER");
            return;
        }
        s->star_x = u->x;
        s->star_z = u->z;
        if (s->commander >= 0) {
            const cadv_rts_unit *c = &r->u[s->commander];
            float d2 = (u->x - c->x) * (u->x - c->x) + (u->z - c->z) * (u->z - c->z);
            if (d2 < 3.5f * 3.5f) {
                s->star_state = 3;
                s->win_timer = 300;
                say(s, "SHINING STAR SECURED!");
                sfx(s, CADV_SFX_STAR, 1.0f);
            } else if (s->ticks % 30 == 0) {
                cadv_rts_order_move(r, s->star_carrier, cadv_rts_cell_at(&r->grid, c->x, c->z), false);
            }
        }
    }
}

static int player_buildings(const p03_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        n += s->rts.b[i].alive && s->rts.b[i].owner == P03_ACCORD && s->rts.b[i].def != P03_B_LAB &&
             s->rts.b[i].def != P03_B_TOWER;
    }
    return n;
}

static int find_yard(const p03_state *s)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        if (s->rts.b[i].alive && s->rts.b[i].owner == P03_ACCORD && s->rts.b[i].def == P03_B_YARD) {
            return i;
        }
    }
    return -1;
}

static void events(p03_state *s)
{
    uint32_t ev = s->rts.events[P03_ACCORD];
    if (ev & CADV_RTS_EV_BUILT) {
        s->placing = true;
        say(s, "CONSTRUCTION COMPLETE - AIM AND FIRE TO PLACE");
        sfx(s, CADV_SFX_BELL, 0.7f);
    }
    if (ev & CADV_RTS_EV_UNIT_READY) {
        say(s, "UNIT READY");
        sfx(s, CADV_SFX_PICKUP, 0.6f);
    }
    if (ev & CADV_RTS_EV_LOW_POWER) {
        say(s, "LOW POWER");
    }
    if (ev & CADV_RTS_EV_BUILDING_LOST) {
        say(s, "STRUCTURE LOST");
        sfx(s, CADV_SFX_EXPLOSION, 0.8f);
    }
    {
        int i;
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            const cadv_rts_unit *u = &s->rts.u[i];
            if (s->was_alive[i] && !u->alive && u->owner == P03_ACCORD && u->def == P03_U_HARVESTER) {
                s->harvester_lost = true;
                say(s, "HARVESTER LOST");
            }
            s->was_alive[i] = u->alive;
        }
    }
    if (ev & CADV_RTS_EV_UNLOAD) {
        sfx(s, CADV_SFX_PICKUP, 0.2f);
    }
    if ((s->rts.events[P03_ORDER] & CADV_RTS_EV_BUILDING_LOST)) {
        sfx(s, CADV_SFX_EXPLOSION, 0.9f);
    }
    if (s->checkpoint < 1 && cadv_rts_count_buildings(&s->rts, P03_ACCORD, P03_B_BARRACKS) > 0 &&
        cadv_rts_count_buildings(&s->rts, P03_ACCORD, P03_B_FACTORY) > 0) {
        s->checkpoint = 1;
        quip(s, "Barracks and factory standing. We can win this.");
    }
}

void cadv_p03_tick(void *state, const cadv_input_frame *in)
{
    p03_state *s = (p03_state *)state;
    cadv_rts *r = &s->rts;
    int prev_waves = s->ai.waves_sent;
    if (s->outcome != CADV_OUTCOME_RUNNING) {
        return;
    }
    ++s->ticks;
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
    if (s->drone > 0) {
        --s->drone;
    } else if (s->drone_cd > 0) {
        --s->drone_cd;
    }
    if (s->commander >= 0) {
        walk(s, in);
        tablet(s, in);
        command(s, in);
    } else if (s->respawn > 0 && --s->respawn == 0) {
        int yard = find_yard(s);
        cadv_vec3 c = centre_of(s, yard);
        spawn_commander(s, c.x, c.z + 7.0f);
        say(s, "COMMANDER REDEPLOYED");
    }
    if (s->star_state == 0) {
        cadv_rts_ai_tick(r, &s->ai); /* the Order fights while its Spire stands */
    }
    cadv_rts_tick(r);
    if (s->commander >= 0 && r->u[s->commander].alive && s->ticks % 30 == 0 && r->u[s->commander].hp < k_units[P03_U_COMMANDER].hp) {
        ++r->u[s->commander].hp; /* the command armour patches itself up */
    }
    if (s->commander >= 0) {
        cadv_rts_reveal(r, P03_ACCORD, r->u[s->commander].x, r->u[s->commander].z, s->drone > 0 ? 14 : 7);
    }
    if (s->ticks % 60 == 0 && find_yard(s) >= 0) {
        r->side[P03_ACCORD].credits += 3; /* the Accord's supply line never runs dry */
    }
    crystal_burn(s);
    events(s);
    capture_lab(s);
    star_rules(s);
    check_marks(s);
    if (s->ai.waves_sent > prev_waves) {
        say(s, "ENEMY FORCES APPROACHING FROM THE NORTH");
    }
    if (s->commander >= 0 && !r->u[s->commander].alive) {
        s->commander = -1;
        s->drone = 0;
        if (find_yard(s) >= 0) {
            s->respawn = 300;
            say(s, "COMMANDER DOWN - REDEPLOYING AT THE YARD");
        } else {
            s->lose_timer = 180;
            say(s, "COMMANDER LOST - THE FRONT COLLAPSES");
        }
    } else if (player_buildings(s) == 0 && s->star_state < 3) {
        s->lose_timer = 180;
        say(s, "THE BASE HAS FALLEN");
    }
}

/* ---- results & scripting ------------------------------------------------------------------ */

cadv_portal_result cadv_p03_result(const void *state)
{
    const p03_state *s = (const p03_state *)state;
    cadv_portal_result res;
    memset(&res, 0, sizeof res);
    res.outcome = s->outcome;
    res.star_collected = s->star_state == 3;
    res.checkpoint = s->checkpoint;
    res.marks_found = s->marks_found;
    res.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        res.snowflakes = 1 + (s->harvester_lost ? 0 : 1) + (s->ticks < 15 * 3600 && s->cfg.difficulty != CADV_DIFF_STORY ? 1 : 0);
    }
    return res;
}

static int army_count(const p03_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        n += is_army(s, i);
    }
    return n;
}

bool cadv_p03_query(const void *state, const char *key, double *out)
{
    const p03_state *s = (const p03_state *)state;
    const cadv_rts *r = &s->rts;
    cadv_vec3 e = cadv_p03_eye(s);
    struct kv { const char *k; double v; };
    const struct kv table[] = {
        {"player.x", e.x},
        {"player.y", e.y},
        {"player.z", e.z},
        {"player.yaw", s->yaw},
        {"player.health", s->commander >= 0 ? r->u[s->commander].hp : 0},
        {"credits", r->side[P03_ACCORD].credits},
        {"power", r->side[P03_ACCORD].power_made},
        {"power.used", r->side[P03_ACCORD].power_used},
        {"buildings", cadv_rts_count_buildings(r, P03_ACCORD, -1)},
        {"units", cadv_rts_count_units(r, P03_ACCORD, -1)},
        {"army", army_count(s)},
        {"harvesters", cadv_rts_count_units(r, P03_ACCORD, P03_U_HARVESTER)},
        {"enemy.units", cadv_rts_count_units(r, P03_ORDER, -1)},
        {"enemy.buildings", cadv_rts_count_buildings(r, P03_ORDER, -1)},
        {"spire.hp", s->spire >= 0 && r->b[s->spire].alive ? r->b[s->spire].hp : 0},
        {"star.state", s->star_state},
        {"harvester_lost", s->harvester_lost},
        {"lab", s->lab_captured},
        {"waves", s->ai.waves_sent},
        {"selected", count_selected(s)},
        {"placing", s->placing},
        {"tab", s->tab_item},
        {"drone", s->drone},
        {"harvested", r->side[P03_ACCORD].harvested},
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

void cadv_p03_teleport(void *state, float x, float y, float z, float yaw)
{
    p03_state *s = (p03_state *)state;
    (void)y;
    if (s->commander >= 0) {
        s->rts.u[s->commander].x = x;
        s->rts.u[s->commander].z = z;
    }
    s->yaw = yaw;
    s->pitch = 0.0f;
}

/* ---- completion bot: build order, then leapfrog the army north ------------------------ */

typedef struct bot_spot {
    int def;
    int x, z; /* top-left cell */
} bot_spot;

static const bot_spot k_bot_base[] = {
    {P03_B_POWER, 26, 54}, {P03_B_REFINERY, 34, 54}, {P03_B_BARRACKS, 26, 51},
    {P03_B_POWER, 26, 57}, {P03_B_FACTORY, 34, 50}, {P03_B_TOWER, 24, 50}, {P03_B_TOWER, 37, 47},
};
#define BOT_SPOTS ((int)(sizeof k_bot_base / sizeof k_bot_base[0]))

/* commander route in cells: base post, then north over the bridge */
static const float k_route[][2] = {{29.0f, 58.5f}, {29.0f, 53.5f}, {29.0f, 42.0f}, {31.5f, 36.0f},
                                   {31.5f, 28.0f}, {31.5f, 21.0f}, {31.5f, 16.5f}};
#define ROUTE_LEN ((int)(sizeof k_route / sizeof k_route[0]))

static int tab_of_struct(int def)
{
    int i;
    for (i = 0; i < P03_TAB_STRUCTURES; ++i) {
        if (k_tab_struct[i] == def) {
            return i;
        }
    }
    return 0; /* GCOVR_EXCL_LINE: every planned structure is on the tablet */
}

static int tab_of_unit(int def)
{
    int i;
    for (i = 0; i < P03_TAB_ITEMS - P03_TAB_STRUCTURES; ++i) {
        if (k_tab_unit[i] == def) {
            return P03_TAB_STRUCTURES + i;
        }
    }
    return P03_TAB_STRUCTURES; /* GCOVR_EXCL_LINE */
}

static void bot_press(cadv_input_frame *out, cadv_action a)
{
    out->pressed |= CADV_ACT_BIT(a);
    out->held |= CADV_ACT_BIT(a);
}

/* Steps the tablet to 'item'; true once it is there and E was pressed. */
static bool bot_buy(const p03_state *s, int item, cadv_input_frame *out)
{
    if (s->tab_item != item) {
        int fwd = (item - s->tab_item + P03_TAB_ITEMS) % P03_TAB_ITEMS;
        bot_press(out, fwd <= P03_TAB_ITEMS / 2 ? CADV_ACT_NEXT_WEAPON : CADV_ACT_PREV_WEAPON);
        return false;
    }
    bot_press(out, CADV_ACT_USE);
    return true;
}

static float bot_aim(const p03_state *s, cadv_vec3 p, cadv_input_frame *out)
{
    cadv_vec3 e = cadv_p03_eye(s);
    cadv_vec3 to = cadv_v3_sub(p, e);
    float want_yaw = cadv_atan2f(-to.x, -to.z);
    float want_pitch = cadv_atan2f(to.y, cadv_sqrtf(to.x * to.x + to.z * to.z));
    float dy = cadv_wrap_angle(want_yaw - s->yaw);
    float dp = want_pitch - s->pitch;
    out->look_x = cadv_clampf(dy, -0.15f, 0.15f);
    out->look_y = cadv_clampf(dp, -0.1f, 0.1f);
    return cadv_absf(dy) + cadv_absf(dp);
}

static void bot_walk(const p03_state *s, float cx, float cz, cadv_input_frame *out)
{
    cadv_vec3 e = cadv_p03_eye(s);
    float tx = cx * P03_CELL;
    float tz = cz * P03_CELL;
    float want = cadv_atan2f(-(tx - e.x), -(tz - e.z));
    float dy = cadv_wrap_angle(want - s->yaw);
    float d = cadv_sqrtf((tx - e.x) * (tx - e.x) + (tz - e.z) * (tz - e.z));
    out->look_x = cadv_clampf(dy, -0.15f, 0.15f);
    out->look_y = cadv_clampf(-s->pitch, -0.1f, 0.1f);
    out->move_y = d > 0.3f && cadv_absf(dy) < 0.6f ? cadv_minf(1.0f, d) : 0.0f;
}

/* Index of the next route point still ahead (north) of the commander. */
static int bot_route_index(const p03_state *s)
{
    cadv_vec3 e = cadv_p03_eye(s);
    float cz = e.z / P03_CELL;
    int i;
    for (i = 0; i < ROUTE_LEN; ++i) {
        if (cz > k_route[i][1] + 0.3f) {
            return i;
        }
    }
    return ROUTE_LEN - 1;
}

static int bot_standing(const p03_state *s, const bot_spot *b)
{
    int i = cadv_rts_building_at(&s->rts, b->x, b->z);
    return i >= 0 && s->rts.b[i].def == b->def && s->rts.b[i].owner == P03_ACCORD;
}

static bool bot_economy(const p03_state *s, cadv_input_frame *out)
{
    const cadv_rts *r = &s->rts;
    const cadv_rts_side *me = &r->side[P03_ACCORD];
    int i;
    /* place a finished structure at its planned spot */
    if (me->ready_building >= 0) {
        for (i = 0; i < BOT_SPOTS; ++i) {
            const bot_spot *b = &k_bot_base[i];
            if (b->def == me->ready_building && !bot_standing(s, b)) {
                const cadv_rts_bdef *d = &k_buildings[b->def];
                cadv_vec3 p = cadv_v3(cell_centre(b->x + d->fw / 2), 0, cell_centre(b->z + d->fh / 2));
                if (!s->placing) {
                    bot_buy(s, tab_of_struct(b->def), out);
                } else if (bot_aim(s, p, out) < 0.01f) {
                    bot_press(out, CADV_ACT_FIRE);
                }
                return true;
            }
        }
    }
    /* next structure (money is saved for it before any unit is bought) */
    {
        int reserve = 0;
        for (i = 0; i < BOT_SPOTS; ++i) {
            if (!bot_standing(s, &k_bot_base[i])) {
                reserve = me->queued[0] == 0 && me->ready_building < 0 ? k_buildings[k_bot_base[i].def].cost : 0;
                if (reserve > 0 && me->credits >= reserve) {
                    bot_buy(s, tab_of_struct(k_bot_base[i].def), out);
                    return true;
                }
                break;
            }
        }
        if (reserve > 0 && cadv_rts_count_units(r, P03_ACCORD, P03_U_HARVESTER) > 0) {
            return false;
        }
    }
    /* units: a harvester if lost, one engineer, a buggy, tanks, infantry */
    if (me->queued[1] < 2) {
        int want = -1;
        bool factory = cadv_rts_can_train(r, P03_ACCORD, P03_U_TANK);
        bool barracks = cadv_rts_can_train(r, P03_ACCORD, P03_U_RIFLE);
        int army = army_count(s);
        if (factory && cadv_rts_count_units(r, P03_ACCORD, P03_U_HARVESTER) < (army >= 3 ? 2 : 1)) {
            want = P03_U_HARVESTER;
        } else if (barracks && cadv_rts_count_units(r, P03_ACCORD, P03_U_ENGINEER) == 0 && me->queued[1] == 0) {
            want = P03_U_ENGINEER;
        } else if (factory && cadv_rts_count_units(r, P03_ACCORD, P03_U_BUGGY) == 0 && me->queued[1] == 0) {
            want = P03_U_BUGGY;
        } else if (army < 24) {
            if (factory && cadv_rts_count_units(r, P03_ACCORD, P03_U_TANK) < 8) {
                want = P03_U_TANK;
            } else if (barracks) {
                want = cadv_rts_count_units(r, P03_ACCORD, P03_U_ROCKET) < 6 ? P03_U_ROCKET : P03_U_RIFLE;
            }
        }
        if (want >= 0 && me->credits < k_units[want].cost && want != P03_U_HARVESTER && barracks) {
            want = P03_U_RIFLE; /* do not stall: cheap infantry while saving */
        }
        if (want >= 0 && me->credits >= k_units[want].cost) {
            bot_buy(s, tab_of_unit(want), out);
            return true;
        }
    }
    return false;
}

/* Where the army should push next: the bridge towers, then the Spire. */
static bool bot_front(const p03_state *s, float *fx, float *fz)
{
    const cadv_rts *r = &s->rts;
    int i;
    float best = 1e9f;
    cadv_vec3 e = cadv_p03_eye(s);
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        if (b->alive && b->owner == P03_ORDER && (b->def == P03_B_TOWER || b->def == P03_B_PYLON || b->def == P03_B_SPIRE)) {
            cadv_vec3 c = centre_of(s, i);
            float d = (c.x - e.x) * (c.x - e.x) + (c.z - e.z) * (c.z - e.z);
            if (d < best) {
                best = d;
                *fx = c.x;
                *fz = c.z;
            }
        }
    }
    return best < 1e9f;
}

void cadv_p03_bot(const void *state, cadv_input_frame *out)
{
    const p03_state *s = (const p03_state *)state;
    cadv_vec3 e;
    int phase;
    int route;
    int army;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0) {
        if (s->win_timer < 170) {
            bot_press(out, CADV_ACT_USE);
        }
        return;
    }
    if (s->commander < 0) {
        return;
    }
    e = cadv_p03_eye(s);
    route = bot_route_index(s);
    if (s->star_state == 0 && e.z / P03_CELL > 50.0f && route > 1) {
        route = 1; /* still at home: the base post is the furthest point */
    }
    army = army_count(s);
    phase = s->ticks % 120;
    /* 1. the star: escort the commander to the vault, send the engineers in */
    if (s->star_state >= 1) {
        cadv_vec3 vault = cadv_v3(s->star_x, 0, s->star_z);
        bool at_vault = route == ROUTE_LEN - 1 && cadv_absf(e.z / P03_CELL - k_route[ROUTE_LEN - 1][1]) < 0.6f;
        if (phase >= 60 && bot_economy(s, out)) {
            return;
        }
        if (s->star_state == 1 && at_vault) {
            if (phase == 0) {
                bot_press(out, CADV_ACT_WEAPON_6);
            } else if (bot_aim(s, vault, out) < 0.02f && phase % 20 == 1) {
                bot_press(out, CADV_ACT_FIRE);
            }
        } else if (s->star_state == 1) {
            if (phase == 0) {
                bot_press(out, CADV_ACT_WEAPON_5);
            } else if (phase < 30) {
                float d = cadv_sqrtf((vault.x - e.x) * (vault.x - e.x) + (vault.z - e.z) * (vault.z - e.z));
                float k = d > 35.0f ? 35.0f / d : 1.0f;
                if (bot_aim(s, cadv_v3(e.x + (vault.x - e.x) * k, 0, e.z + (vault.z - e.z) * k), out) < 0.02f && phase % 10 == 1) {
                    bot_press(out, CADV_ACT_FIRE);
                }
            } else {
                bot_walk(s, k_route[route][0], k_route[route][1], out);
            }
        }
        return;
    }
    /* 2. at home: build the whole base and an army before marching */
    if (route <= 1) {
        bool base_done = true;
        int i;
        for (i = 0; i < BOT_SPOTS; ++i) {
            base_done = base_done && bot_standing(s, &k_bot_base[i]);
        }
        if (bot_economy(s, out)) {
            return;
        }
        if (route == 1 && base_done && army >= (s->cfg.difficulty == CADV_DIFF_HARD ? 22 : 16)) {
            bot_walk(s, k_route[2][0], k_route[2][1], out); /* time to march */
        } else {
            bot_walk(s, k_route[route][0], k_route[route][1], out);
        }
        return;
    }
    if (phase >= 60 && bot_economy(s, out)) {
        return; /* keep reinforcements coming */
    }
    if (army < 5) {
        /* too thin: fall back over the bridge and regroup at home */
        if (e.z / P03_CELL < 37.0f) {
            bot_walk(s, k_route[3][0], 37.5f, out);
        } else {
            bot_walk(s, k_route[1][0], k_route[1][1], out);
        }
        return;
    }
    /* 3. the push: select the army, order it to the front, follow at a distance */
    {
        float fx = 0.0f;
        float fz = 0.0f;
        bool front = bot_front(s, &fx, &fz);
        float d = front ? cadv_sqrtf((fx - e.x) * (fx - e.x) + (fz - e.z) * (fz - e.z)) : 0.0f;
        if (phase == 0) {
            bot_press(out, CADV_ACT_WEAPON_5);
            return;
        }
        int i;
        int near = 0;
        float sum_z = 0.0f;
        bool gathered;
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            const cadv_rts_unit *u = &s->rts.u[i];
            if (is_army(s, i)) {
                sum_z += u->z;
                if ((u->x - e.x) * (u->x - e.x) + (u->z - e.z) * (u->z - e.z) < 22.0f * 22.0f) {
                    ++near;
                }
            }
        }
        gathered = near * 5 >= army * 3;
        if (front && phase < 40) {
            /* regroup on the commander, then order a point up to 35 m ahead */
            float reach = gathered ? 35.0f : 10.0f;
            float k = d > reach ? reach / d : 1.0f;
            cadv_vec3 p = cadv_v3(e.x + (fx - e.x) * k, 0, e.z + (fz - e.z) * k);
            if (bot_aim(s, p, out) < 0.02f && phase % 10 == 1) {
                bot_press(out, CADV_ACT_FIRE);
            }
            return;
        }
        if ((!front || d > 32.0f) && army >= 4 && gathered && sum_z / (float)army < e.z - 4.0f) {
            bot_walk(s, k_route[route][0], k_route[route][1], out); /* follow behind the army */
        }
    }
}
