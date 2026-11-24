/* SPDX-License-Identifier: MIT */
/* Day 8 - Cascade: a monorail arrival, the resonance test that goes wrong,
 * ruins with skitterlings and a power puzzle, the Blackice Detail's squads
 * while scientists are escorted to a retina scanner, and the antenna array
 * in a blizzard. Scripted moments are triggers; the player keeps control. */
#include "portals/p08_cascade.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "hub/friends.h"
#include "portals/p08_internal.h"

/* ---- tables ------------------------------------------------------------------------- */

static const p08_weapon_def k_weapons[P08_WEAPONS] = {
    /* name        dmg pel rate  mag res  range spread */
    {"ICE AXE", 25, 1, 0.45f, 0, 0, 1.9f, 0.0f},
    {"9MM", 12, 1, 0.18f, 17, 85, 60.0f, 0.012f},
    {"SHOTGUN", 8, 7, 0.9f, 8, 24, 25.0f, 0.07f},
    {"CROSSBOW", 90, 1, 1.6f, 5, 15, 90.0f, 0.004f},
    {"GRENADE", 100, 1, 1.0f, 0, 3, 0.0f, 0.0f},
};

static const char *const k_npc_names[P08_NPCS] = {"DR. MAREN", "DR. OKAFOR", "DR. LINDQVIST", "OFFICER BRANDT", "C6P"};

/* walkable rooms (cells, inclusive) */
static const int k_rooms[][4] = {
    {1, 9, 20, 12},   /* monorail tunnel */
    {21, 5, 30, 16},  /* lobby */
    {31, 10, 31, 11}, /* chamber door */
    {32, 7, 38, 14},  /* test chamber */
    {39, 10, 39, 11}, /* the breach */
    {40, 5, 48, 20},  /* ruins */
    {40, 1, 43, 3},   /* radiology */
    {42, 4, 42, 4},
    {49, 10, 49, 11}, /* freight gate */
    {50, 1, 61, 16},  /* Blackice offices */
    {57, 17, 61, 20},
    {52, 18, 55, 20}, /* safe lab */
    {53, 17, 53, 17}, /* lab door */
    {62, 10, 62, 11}, /* scanner door */
    {63, 3, 78, 19},  /* the antenna array */
};
static const int k_props[][2] = {{47, 6}, {53, 5}, {54, 5}, {57, 9}, {57, 10}, {52, 12}, {59, 14}, {55, 14}, {24, 11}, {27, 9}, {35, 8}, {35, 13}};

#define CC(c) (((float)(c) + 0.5f) * P08_CELL)

const p08_weapon_def *cadv_p08_weapons(void) { return k_weapons; }
const char *cadv_p08_npc_name(int i) { return i >= 0 && i < P08_NPCS ? k_npc_names[i] : "?"; }
cadv_vec3 cadv_p08_cell(int cx, int cz) { return cadv_v3(CC(cx), 0.0f, CC(cz)); }

/* ---- helpers ---------------------------------------------------------------------- */

static void copy_text(char *out, size_t cap, const char *text)
{
    size_t i;
    for (i = 0; text[i] && i + 1 < cap; ++i) {
        out[i] = text[i];
    }
    out[i] = '\0';
}

static void say(p08_state *s, const char *text)
{
    copy_text(s->message, sizeof s->message, text);
    s->message_timer = 180;
}

static void radio(p08_state *s, const char *text)
{
    copy_text(s->radio, sizeof s->radio, text);
    s->radio_timer = 300;
}

static void sfx_at(p08_state *s, cadv_sfx id, float vol, cadv_vec3 at)
{
    if (s->cfg.audio) {
        float d = cadv_v3_dist(at, s->body.pos);
        cadv_audio_sfx(s->cfg.audio, id, vol * cadv_clampf(1.0f - d / 45.0f, 0.05f, 1.0f), 0.0f);
    }
}

static float flat(cadv_vec3 a, cadv_vec3 b) { return cadv_sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }

static float yaw_to(cadv_vec3 from, cadv_vec3 to) { return cadv_atan2f(-(to.x - from.x), -(to.z - from.z)); }

static float diff_scale(const p08_state *s, float story, float normal, float hard)
{
    return s->cfg.difficulty == CADV_DIFF_STORY ? story : s->cfg.difficulty == CADV_DIFF_HARD ? hard : normal;
}

cadv_vec3 cadv_p08_eye(const p08_state *s) { return cadv_player_eye(&s->body); }

cadv_vec3 cadv_p08_npc_pos(const p08_state *s, int i) { return s->npc[i].f.pos; }

int cadv_p08_enemies_alive(const p08_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < P08_ENEMIES; ++i) {
        n += s->e[i].active && !s->e[i].hidden_until_scan;
    }
    return n;
}

int cadv_p08_scientists_alive(const p08_state *s)
{
    return (s->npc[P08_NPC_MAREN].alive ? 1 : 0) + (s->npc[P08_NPC_OKAFOR].alive ? 1 : 0) + (s->npc[P08_NPC_LINDQVIST].alive ? 1 : 0);
}

static void set_solid(p08_state *s, int idx, bool on)
{
    int c;
    if (idx < 0) {
        return;
    }
    s->world.s[idx].enabled = on;
    /* doors in the walk grid follow the solid */
    {
        cadv_aabb b = s->world.s[idx].box;
        int x;
        int z;
        for (z = (int)(b.lo.z / P08_CELL); z < (int)(b.hi.z / P08_CELL + 0.5f); ++z) {
            for (x = (int)(b.lo.x / P08_CELL); x < (int)(b.hi.x / P08_CELL + 0.5f); ++x) {
                c = z * P08_W + x;
                if (x >= 0 && z >= 0 && x < P08_W && z < P08_H && s->cells[c] == 0) {
                    cadv_nav_set(&s->nav, x, z, !on);
                }
            }
        }
    }
}

/* ---- the station -------------------------------------------------------------------- */

static int add_box(p08_state *s, float x0, float y0, float z0, float x1, float y1, float z1, int tag)
{
    return cadv_world_add(&s->world, cadv_v3(x0, y0, z0), cadv_v3(x1, y1, z1), tag);
}

static int door(p08_state *s, int x0, int z0, int x1, int z1, int tag)
{
    return add_box(s, (float)x0 * P08_CELL, 0, (float)z0 * P08_CELL, (float)(x1 + 1) * P08_CELL, P08_WALL_H, (float)(z1 + 1) * P08_CELL, tag);
}

static void build_station(p08_state *s)
{
    int x;
    int z;
    int k;
    memset(s->cells, 1, sizeof s->cells);
    for (k = 0; k < (int)(sizeof k_rooms / sizeof k_rooms[0]); ++k) {
        for (z = k_rooms[k][1]; z <= k_rooms[k][3]; ++z) {
            for (x = k_rooms[k][0]; x <= k_rooms[k][2]; ++x) {
                s->cells[z * P08_W + x] = 0;
            }
        }
    }
    for (k = 0; k < (int)(sizeof k_props / sizeof k_props[0]); ++k) {
        s->cells[k_props[k][1] * P08_W + k_props[k][0]] = 2;
    }
    /* the antenna tower core and the shotgun ledge */
    s->cells[9 * P08_W + 70] = s->cells[9 * P08_W + 71] = s->cells[10 * P08_W + 70] = s->cells[10 * P08_W + 71] = 2;
    for (z = 18; z <= 20; ++z) {
        s->cells[z * P08_W + 40] = s->cells[z * P08_W + 41] = 2;
    }
    cadv_world_clear(&s->world);
    add_box(s, 0, -1, 0, (float)P08_W * P08_CELL, 0, (float)P08_H * P08_CELL, 0);
    for (z = 0; z < P08_H; ++z) {
        for (x = 0; x < P08_W;) {
            int c = s->cells[z * P08_W + x];
            int x1 = x;
            float h;
            if (c == 0) {
                ++x;
                continue;
            }
            while (x1 + 1 < P08_W && s->cells[z * P08_W + x1 + 1] == c) {
                ++x1;
            }
            h = c == 1 ? P08_WALL_H : 1.2f;
            if (c == 2 && x >= 70 && x <= 71 && z >= 9 && z <= 10) {
                h = 14.0f; /* the tower */
            } else if (c == 2 && x == 40 && z >= 18) {
                h = 1.6f; /* the ledge */
            }
            add_box(s, (float)x * P08_CELL, 0, (float)z * P08_CELL, (float)(x1 + 1) * P08_CELL, h, (float)(z + 1) * P08_CELL, c == 1 ? P08_TAG_WALL : P08_TAG_LEDGE);
            x = x1 + 1;
        }
    }
    /* stairs up to the top platform of the array, half a metre at a time */
    for (k = 0; k < P08_STEPS; ++k) {
        add_box(s, (float)(66 + k) * P08_CELL, 0, 17.0f * P08_CELL, (float)(67 + k) * P08_CELL, 0.45f * (float)(k + 1), 19.0f * P08_CELL, P08_TAG_STEP);
    }
    add_box(s, 72.0f * P08_CELL, 0, 16.0f * P08_CELL, 76.0f * P08_CELL, 0.45f * (float)P08_STEPS, 19.0f * P08_CELL, P08_TAG_STEP);
    /* the ceiling everywhere but the array */
    add_box(s, 0, P08_WALL_H, 0, 63.0f * P08_CELL, P08_WALL_H + 1.0f, (float)P08_H * P08_CELL, 0);
    cadv_nav_init(&s->nav, P08_W, P08_H, P08_CELL);
    for (z = 0; z < P08_H; ++z) {
        for (x = 0; x < P08_W; ++x) {
            cadv_nav_set(&s->nav, x, z, s->cells[z * P08_W + x] == 0);
        }
    }
    /* the stairs and the top platform are entered from the bottom step only */
    for (x = 66; x <= 76; ++x) {
        cadv_nav_set(&s->nav, x, x >= 72 ? 15 : 16, false);
        cadv_nav_set(&s->nav, x, 19, false);
    }
    for (z = 16; z <= 18; ++z) {
        cadv_nav_set(&s->nav, 76, z, false);
    }
    cadv_nav_mark_cover(&s->nav);
    /* doors */
    s->s_chamber = door(s, 31, 10, 31, 11, P08_TAG_CHAMBER);
    s->s_breach = door(s, 39, 10, 39, 11, P08_TAG_BREACH);
    s->s_gate = door(s, 49, 10, 49, 11, P08_TAG_GATE);
    s->s_lab = door(s, 53, 17, 53, 17, P08_TAG_LAB);
    s->s_scanner = door(s, 62, 10, 62, 11, P08_TAG_SCANNER);
    s->crate_x = CC(44);
    s->crate_z = CC(16);
    s->s_crate = add_box(s, s->crate_x - 0.9f, 0, s->crate_z - 0.9f, s->crate_x + 0.9f, 0.8f, s->crate_z + 0.9f, P08_TAG_CRATE);
    set_solid(s, s->s_chamber, true);
    set_solid(s, s->s_breach, true);
    set_solid(s, s->s_gate, true);
    set_solid(s, s->s_lab, true);
    set_solid(s, s->s_scanner, true);
    /* the crate is a block in the walk grid */
    cadv_nav_set(&s->nav, 44, 16, false);
    /* the tram car: three walls that ride along */
    s->s_tram[0] = add_box(s, 0, 0, 18.2f, 1, 3, 18.5f, P08_TAG_TRAM);
    s->s_tram[1] = add_box(s, 0, 0, 25.5f, 1, 3, 25.8f, P08_TAG_TRAM);
    s->s_tram[2] = add_box(s, 0, 0, 18.2f, 1, 3, 25.8f, P08_TAG_TRAM);
}

static void place_tram(p08_state *s)
{
    float x = s->tram_x;
    s->world.s[s->s_tram[0]].box = cadv_aabb_make(cadv_v3(x - 3.0f, 0, 18.2f), cadv_v3(x + 3.0f, 3.0f, 18.6f));
    s->world.s[s->s_tram[1]].box = cadv_aabb_make(cadv_v3(x - 3.0f, 0, 25.4f), cadv_v3(x + 3.0f, 3.0f, 25.8f));
    s->world.s[s->s_tram[2]].box = cadv_aabb_make(cadv_v3(x + 2.8f, 0, 18.2f), cadv_v3(x + 3.2f, 3.0f, 25.8f)); /* the front: the doors */
    s->world.s[s->s_tram[2]].enabled = s->tram_moving;
}

static void add_pickup(p08_state *s, int kind, float x, float y, float z)
{
    int i;
    for (i = 0; i < P08_PICKUPS; ++i) {
        if (!s->pk[i].active) {
            s->pk[i].active = true;
            s->pk[i].kind = kind;
            s->pk[i].pos = cadv_v3(x, y, z);
            return;
        }
    }
}

static int spawn_enemy(p08_state *s, int kind, float x, float z, int squad)
{
    int i;
    for (i = 0; i < P08_ENEMIES; ++i) {
        p08_enemy *e = &s->e[i];
        if (!e->active) {
            memset(e, 0, sizeof *e);
            e->active = true;
            e->kind = kind;
            e->pos = cadv_v3(x, 0, z);
            e->hp = kind == P08_E_SKITTER ? 14 : (int)diff_scale(s, 50.0f, 70.0f, 90.0f);
            e->squad = squad;
            e->target = -1;
            e->think = 0.3f * (float)(i % 4);
            e->grenade_cd = 4.0f + (float)(i % 3) * 2.0f;
            return i;
        }
    }
    return -1; /* GCOVR_EXCL_LINE: the waves are capped well below the table */
}

static void npc_init(p08_state *s, int i, int cx, int cz, int hp)
{
    p08_npc *n = &s->npc[i];
    memset(n, 0, sizeof *n);
    n->alive = true;
    n->hp = hp;
    cadv_follower_init(&n->f, cadv_p08_cell(cx, cz), i == P08_NPC_BRANDT ? 4.6f : 4.2f, i == P08_NPC_BRANDT ? 2.5f : 2.0f + 0.6f * (float)i);
}

static void give(p08_state *s, int w)
{
    s->owned[w] = true;
    s->mag[w] = k_weapons[w].mag;
    s->reserve[w] = w == P08_W_GRENADE ? cadv_maxi(s->reserve[w], 2) : k_weapons[w].reserve;
}

/* ---- story beats ------------------------------------------------------------------------- */

static void start_cascade(p08_state *s)
{
    int k;
    s->act = P08_ACT_RUINS;
    s->cascade_t = 1;
    s->flash = 40;
    set_solid(s, s->s_breach, false);
    set_solid(s, s->s_chamber, false);
    say(s, "RESONANCE CASCADE - GET OUT!");
    radio(s, "MAREN: Shut it down! Shut it - the readings are folding in on themselves!");
    sfx_at(s, CADV_SFX_EXPLOSION, 1.0f, s->body.pos);
    /* the scientists flee to the safe lab beyond the ruins */
    for (k = P08_NPC_MAREN; k <= P08_NPC_LINDQVIST; ++k) {
        s->npc[k].f.pos = cadv_p08_cell(53 + k, 19);
        s->npc[k].f.mode = CADV_FOLLOW_WAIT;
    }
    for (k = 0; k < (int)diff_scale(s, 3.0f, 4.0f, 5.0f); ++k) {
        spawn_enemy(s, P08_E_SKITTER, CC(42 + (k % 4) * 2), CC(8 + (k / 4) * 6), 0);
    }
    /* the hidden ones the radiology scanner shows */
    for (k = 0; k < 3; ++k) {
        int e = spawn_enemy(s, P08_E_SKITTER, CC(40 + k), CC(2), 0);
        s->e[e].hidden_until_scan = true;
    }
}

static void send_squad(p08_state *s, int squad)
{
    int k;
    int n = (int)diff_scale(s, 2.0f, 3.0f, 4.0f);
    for (k = 0; k < n; ++k) {
        if (squad == 1) {
            spawn_enemy(s, P08_E_SOLDIER, CC(58 + (k % 2) * 2), CC(3 + k * 3), squad);
        } else {
            spawn_enemy(s, P08_E_SOLDIER, CC(60), CC(12 + k * 2), squad);
        }
    }
    s->squad_sent = squad;
    radio(s, squad == 1 ? "BLACKICE: Detail is on site. Sweep and clear - no witnesses." : "BLACKICE: Second team in. The science staff is a priority target.");
}

static void enter_array(p08_state *s)
{
    int k;
    s->act = P08_ACT_ARRAY;
    if (s->checkpoint < 3) {
        s->checkpoint = 3;
    }
    say(s, "THE ARRAY - FLIP THE THREE BREAKERS");
    radio(s, "OKAFOR: The beam has to be reversed from the breakers up there. We'll wait here - go!");
    for (k = P08_NPC_MAREN; k <= P08_NPC_BRANDT; ++k) {
        if (s->npc[k].alive) {
            cadv_follower_goto(&s->npc[k].f, cadv_p08_cell(58 + (k % 3), 18 + (k / 3)));
        }
    }
    s->slam_cd = 180;
}

/* Lay the station out for a fresh start or a checkpoint. */
static void stage(p08_state *s, int cp)
{
    int k;
    s->act = P08_ACT_TRAM;
    s->tram_x = 6.0f;
    s->tram_moving = true;
    cadv_player_spawn(&s->body, cadv_v3(6.0f, 0, 22.0f), -0.5f * CADV_PI);
    s->body.tune.walk_speed = 4.6f;
    s->body.tune.run_speed = 6.6f;
    s->weapon = P08_W_AXE;
    npc_init(s, P08_NPC_MAREN, 24, 8, 40);
    npc_init(s, P08_NPC_OKAFOR, 26, 14, 40);
    npc_init(s, P08_NPC_LINDQVIST, 28, 12, 40);
    npc_init(s, P08_NPC_BRANDT, 44, 10, 120);
    npc_init(s, P08_NPC_C6P, 22, 6, 1000);
    s->cart_x = CC(33);
    s->chargers[0].reserve = s->chargers[1].reserve = s->chargers[2].reserve = 75.0f;
    s->chargers[0].rate = s->chargers[1].rate = s->chargers[2].rate = 25.0f;
    s->charger_pos[0] = cadv_p08_cell(51, 1);
    s->charger_pos[1] = cadv_p08_cell(60, 16);
    s->charger_pos[2] = cadv_p08_cell(63, 9);
    add_pickup(s, P08_PK_AXE, CC(40), 0.3f, CC(10));
    add_pickup(s, P08_PK_PISTOL, CC(41), 0.3f, CC(13));
    add_pickup(s, P08_PK_SHOTGUN, CC(40) + 1.0f, 1.9f, CC(19));
    add_pickup(s, P08_PK_CROSSBOW, CC(56), 0.3f, CC(3));
    add_pickup(s, P08_PK_GRENADE, CC(51), 0.3f, CC(8));
    add_pickup(s, P08_PK_GRENADE, CC(60), 0.3f, CC(18));
    add_pickup(s, P08_PK_AMMO, CC(47), 0.3f, CC(18));
    add_pickup(s, P08_PK_AMMO, CC(58), 0.3f, CC(4));
    add_pickup(s, P08_PK_AMMO, CC(53), 0.3f, CC(15));
    add_pickup(s, P08_PK_AMMO, CC(64), 0.3f, CC(17));
    add_pickup(s, P08_PK_MEDKIT, CC(45), 0.3f, CC(6));
    add_pickup(s, P08_PK_MEDKIT, CC(60), 0.3f, CC(2));
    add_pickup(s, P08_PK_MEDKIT, CC(64), 0.3f, CC(4));
    add_pickup(s, P08_PK_CELL, CC(41), 0.3f, CC(2));
    add_pickup(s, P08_PK_CELL, CC(46), 0.3f, CC(19));
    cadv_trigger_make(&s->t_lobby, cadv_v3(42, 0, 10), cadv_v3(62, 4, 34), true);
    cadv_trigger_make(&s->t_ruins, cadv_v3(80, 0, 10), cadv_v3(98, 4, 42), true);
    cadv_trigger_make(&s->t_radiology, cadv_v3(80, 0, 2), cadv_v3(88, 4, 10), true);
    cadv_trigger_make(&s->t_blackice, cadv_v3(100, 0, 2), cadv_v3(124, 4, 42), true);
    cadv_trigger_make(&s->t_array, cadv_v3(126, 0, 6), cadv_v3(158, 20, 40), true);
    if (cp <= 0) {
        say(s, "WELCOME TO POLARIS DEEP");
        radio(s, "TRAM: Good morning. You are arriving at Polaris Deep, sublevel three. Mind the gap and the frost.");
        return;
    }
    /* checkpoints: the cascade has happened */
    s->tram_moving = false;
    s->tram_x = 36.0f;
    s->suit = true;
    s->body.armor = 40;
    s->cart_x = CC(37);
    s->cart_pushed = true;
    give(s, P08_W_AXE);
    give(s, P08_W_PISTOL);
    s->pk[0].active = s->pk[1].active = false;
    start_cascade(s);
    s->flash = 0;
    for (k = 0; k < P08_ENEMIES; ++k) {
        if (cp >= 2 || !s->e[k].hidden_until_scan) {
            s->e[k].active = cp < 2 && s->e[k].active;
        }
    }
    s->act = P08_ACT_RUINS;
    s->body.pos = cadv_p08_cell(40, 11);
    s->body.yaw = -0.5f * CADV_PI;
    if (cp >= 2) {
        give(s, P08_W_SHOTGUN);
        s->pk[2].active = false;
        s->generator_on = true;
        s->pk[13].active = s->pk[14].active = false;
        set_solid(s, s->s_gate, false);
        s->npc[P08_NPC_BRANDT].met = true;
        s->npc[P08_NPC_BRANDT].f.mode = CADV_FOLLOW_ON;
        s->npc[P08_NPC_BRANDT].f.pos = cadv_p08_cell(48, 12);
        s->body.pos = cadv_p08_cell(48, 11);
        s->act = P08_ACT_BLACKICE;
    }
    if (cp >= 3) {
        give(s, P08_W_CROSSBOW);
        give(s, P08_W_GRENADE);
        s->pk[3].active = false;
        s->lab_open = true;
        s->scanner_open = true;
        set_solid(s, s->s_lab, false);
        set_solid(s, s->s_scanner, false);
        s->squad_sent = 2;
        s->body.pos = cadv_p08_cell(63, 11);
        for (k = P08_NPC_MAREN; k <= P08_NPC_BRANDT; ++k) {
            s->npc[k].met = true;
            s->npc[k].f.pos = cadv_p08_cell(58 + (k % 3), 18 + (k / 3));
        }
        s->t_blackice.fired = 1;
        s->t_array.fired = 1;
        enter_array(s);
    }
    s->t_lobby.fired = s->t_ruins.fired = 1;
    say(s, "CHECKPOINT");
}

bool cadv_p08_init(void *state, const cadv_portal_cfg *cfg)
{
    p08_state *s = (p08_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    s->checkpoint = cfg->checkpoint;
    cadv_rng_seed(&s->rng, cfg->seed, 8u);
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
    build_station(s);
    stage(s, cfg->checkpoint);
    place_tram(s);
    return true;
}

/* ---- combat ---------------------------------------------------------------------------- */

static void hurt_player(p08_state *s, int amount)
{
    if (s->win_timer > 0 || s->lose_timer > 0) {
        return;
    }
    amount = (int)((float)amount * diff_scale(s, 0.55f, 1.0f, 1.15f) + 0.5f);
    s->hurt_flash = 10;
    sfx_at(s, CADV_SFX_HIT, 0.6f, s->body.pos);
    if (cadv_player_damage(&s->body, amount < 1 ? 1 : amount)) {
        s->lose_timer = 180;
        say(s, "INES KOWALCZYK - SIGNAL LOST");
    }
}

static void hurt_npc(p08_state *s, int i, int amount)
{
    p08_npc *n = &s->npc[i];
    if (!n->alive || i == P08_NPC_C6P) {
        return;
    }
    n->hp -= amount;
    n->hurt = 8;
    if (n->hp <= 0) {
        char buf[72];
        n->alive = false;
        snprintf(buf, sizeof buf, "%s IS DOWN", k_npc_names[i]);
        say(s, buf);
    }
}

static void hurt_enemy(p08_state *s, int i, int amount)
{
    p08_enemy *e = &s->e[i];
    if (!e->active) {
        return;
    }
    e->hp -= amount;
    e->hurt = 6;
    e->hidden = 0.0f;
    if (e->hp <= 0) {
        e->active = false;
        sfx_at(s, CADV_SFX_EXPLOSION, 0.3f, e->pos);
        if (e->kind == P08_E_SOLDIER && cadv_rng_below(&s->rng, 2) == 0) {
            add_pickup(s, P08_PK_AMMO, e->pos.x, 0.3f, e->pos.z);
        }
    }
}

static float enemy_height(int kind) { return kind == P08_E_SKITTER ? 0.7f : 1.75f; }

/* The player's hitscan: the nearest enemy along the ray, else the wall. */
static int player_ray(p08_state *s, cadv_vec3 e, cadv_vec3 dir, float range, float *dist)
{
    float best = range;
    int hit = -1;
    int i;
    cadv_world_raycast(&s->world, e, dir, range, &best, NULL, NULL);
    for (i = 0; i < P08_ENEMIES; ++i) {
        float t;
        if (s->e[i].active && !s->e[i].hidden_until_scan &&
            cadv_ray_cylinder(e, dir, s->e[i].pos, s->e[i].kind == P08_E_SKITTER ? 0.5f : 0.4f, enemy_height(s->e[i].kind), best, &t)) {
            best = t;
            hit = i;
        }
    }
    *dist = best;
    return hit;
}

static void throw_nade(p08_state *s, cadv_vec3 from, cadv_vec3 vel, bool enemy)
{
    int i;
    for (i = 0; i < P08_NADES; ++i) {
        if (!s->nades[i].active) {
            s->nades[i].active = true;
            s->nades[i].enemy = enemy;
            s->nades[i].pos = from;
            s->nades[i].vel = vel;
            s->nades[i].fuse = 2.0f;
            return;
        }
    }
}

static void fire(p08_state *s)
{
    const p08_weapon_def *w = &k_weapons[s->weapon];
    cadv_vec3 e = cadv_p08_eye(s);
    int p;
    if (s->weapon == P08_W_AXE) {
        float d;
        int hit = player_ray(s, e, cadv_player_forward(&s->body), w->range, &d);
        s->shot_cd = w->rate;
        if (hit >= 0) {
            hurt_enemy(s, hit, w->damage);
            sfx_at(s, CADV_SFX_HIT, 0.6f, e);
        } else {
            sfx_at(s, CADV_SFX_STEP, 0.4f, e);
        }
        return;
    }
    if (s->weapon == P08_W_GRENADE) {
        cadv_vec3 f = cadv_player_forward(&s->body);
        if (s->reserve[P08_W_GRENADE] <= 0) {
            return;
        }
        --s->reserve[P08_W_GRENADE];
        throw_nade(s, e, cadv_v3(f.x * 12.0f, f.y * 12.0f + 3.0f, f.z * 12.0f), false);
        s->shot_cd = w->rate;
        return;
    }
    if (s->mag[s->weapon] <= 0) {
        return;
    }
    for (p = 0; p < w->pellets; ++p) {
        float spread = w->spread * (s->zoom ? 0.2f : 1.0f) + 0.006f * cadv_minf(1.0f, cadv_sqrtf(s->body.vel.x * s->body.vel.x + s->body.vel.z * s->body.vel.z) / 4.0f);
        float yaw = s->body.yaw + (cadv_rng_float01(&s->rng) * 2.0f - 1.0f) * spread;
        float pitch = s->body.pitch + (cadv_rng_float01(&s->rng) * 2.0f - 1.0f) * spread;
        float d;
        int hit = player_ray(s, e, cadv_v3_from_yaw_pitch(yaw, pitch), w->range, &d);
        if (hit >= 0) {
            hurt_enemy(s, hit, w->damage);
        }
    }
    --s->mag[s->weapon];
    s->shot_cd = w->rate;
    if (s->mag[s->weapon] <= 0 && s->reserve[s->weapon] > 0) {
        int take = cadv_mini(w->mag, s->reserve[s->weapon]);
        s->mag[s->weapon] = take;
        s->reserve[s->weapon] -= take;
        s->shot_cd = 1.8f;
    }
    sfx_at(s, CADV_SFX_SHOT, s->weapon == P08_W_SHOTGUN ? 0.9f : 0.5f, e);
}

static void tick_nades(p08_state *s)
{
    int i;
    int k;
    for (i = 0; i < P08_NADES; ++i) {
        p08_nade *n = &s->nades[i];
        cadv_vec3 next;
        if (!n->active) {
            continue;
        }
        n->vel.y -= 14.0f * P08_DT;
        next = cadv_v3_add(n->pos, cadv_v3_scale(n->vel, P08_DT));
        if (!cadv_world_line_of_sight(&s->world, n->pos, next)) {
            n->vel = cadv_v3_scale(n->vel, -0.3f);
            next = n->pos;
        }
        n->pos = next;
        n->fuse -= P08_DT;
        if (n->fuse > 0.0f) {
            continue;
        }
        n->active = false;
        sfx_at(s, CADV_SFX_EXPLOSION, 0.9f, n->pos);
        if (cadv_v3_dist(n->pos, s->body.pos) < 5.0f) {
            hurt_player(s, n->enemy ? 45 : 30);
        }
        for (k = 0; k < P08_ENEMIES; ++k) {
            if (s->e[k].active && cadv_v3_dist(n->pos, s->e[k].pos) < 5.0f) {
                hurt_enemy(s, k, n->enemy ? 0 : 100);
            }
        }
        for (k = 0; k < P08_NPC_C6P; ++k) {
            if (cadv_v3_dist(n->pos, s->npc[k].f.pos) < 5.0f) {
                hurt_npc(s, k, 40);
            }
        }
    }
}

/* ---- enemies ------------------------------------------------------------------------------- */

static cadv_vec3 target_pos(const p08_state *s, int t) { return t < 0 ? s->body.pos : s->npc[t].f.pos; }

static bool sees(const p08_state *s, cadv_vec3 from, cadv_vec3 to)
{
    return cadv_world_line_of_sight(&s->world, cadv_v3(from.x, from.y + 1.4f, from.z), cadv_v3(to.x, to.y + 1.3f, to.z));
}

static void enemy_move(p08_state *s, p08_enemy *e, cadv_vec3 goal, float speed)
{
    cadv_vec3 step = goal;
    if (e->path_len == 0 || (s->ticks + (int)(e - s->e)) % 40 == 0) {
        int n = cadv_nav_path(&s->nav, cadv_nav_cell_at(&s->nav, e->pos.x, e->pos.z), cadv_nav_cell_at(&s->nav, goal.x, goal.z), e->path, 48);
        e->path_len = n > 0 ? n : 0;
        e->path_pos = 0;
    }
    while (e->path_pos < e->path_len) {
        cadv_vec3 c = cadv_nav_centre(&s->nav, e->path[e->path_pos]);
        if (flat(c, e->pos) < 0.6f && e->path_pos + 1 < e->path_len) {
            ++e->path_pos;
        } else {
            step = e->path_pos + 1 < e->path_len ? c : goal;
            break;
        }
    }
    {
        float dx = step.x - e->pos.x;
        float dz = step.z - e->pos.z;
        float d = cadv_sqrtf(dx * dx + dz * dz);
        if (d > 0.05f) {
            float v = cadv_minf(speed * P08_DT, d);
            cadv_world_move(&s->world, &e->pos, e->kind == P08_E_SKITTER ? 0.35f : 0.3f, enemy_height(e->kind), cadv_v3(dx / d * v, 0.0f, dz / d * v), 0.45f);
            e->yaw = yaw_to(e->pos, step);
        }
    }
}

static int pick_target(p08_state *s, const p08_enemy *e)
{
    int best = -1;
    float bd = flat(e->pos, s->body.pos);
    int k;
    if (!sees(s, e->pos, s->body.pos)) {
        bd = 1e9f;
    }
    for (k = 0; k < P08_NPC_C6P; ++k) {
        const p08_npc *n = &s->npc[k];
        float d;
        if (!n->alive || !n->met) {
            continue;
        }
        d = flat(e->pos, n->f.pos) * (e->kind == P08_E_SOLDIER && k != P08_NPC_BRANDT ? 1.3f : 1.6f);
        if (d < bd && sees(s, e->pos, n->f.pos)) {
            bd = d;
            best = k;
        }
    }
    return best;
}

static void tick_skitter(p08_state *s, int i)
{
    p08_enemy *e = &s->e[i];
    cadv_vec3 t;
    float d;
    if ((s->ticks + i) % 30 == 0) {
        e->target = pick_target(s, e);
    }
    t = target_pos(s, e->target);
    d = flat(e->pos, t);
    if (d > 32.0f && !sees(s, e->pos, t)) {
        return; /* lurking until something comes near */
    }
    e->cd -= P08_DT;
    if (d > 1.1f) {
        enemy_move(s, e, t, diff_scale(s, 4.2f, 5.4f, 6.2f));
        /* the leap */
        if (d < 4.0f && e->pos.y <= 0.01f && e->cd <= 0.0f) {
            e->vel.y = 4.0f;
        }
    } else if (e->cd <= 0.0f) {
        e->cd = 1.0f;
        if (e->target < 0) {
            hurt_player(s, 4);
        } else {
            hurt_npc(s, e->target, 4);
        }
    }
    if (e->vel.y != 0.0f || e->pos.y > 0.0f) {
        e->vel.y -= 14.0f * P08_DT;
        e->pos.y = cadv_maxf(0.0f, e->pos.y + e->vel.y * P08_DT);
        if (e->pos.y <= 0.0f) {
            e->vel.y = 0.0f;
            e->cd = cadv_maxf(e->cd, 0.5f);
        }
    }
}

static void tick_soldier(p08_state *s, int i)
{
    p08_enemy *e = &s->e[i];
    cadv_vec3 t;
    bool see;
    float d;
    int k;
    if ((s->ticks + i) % 30 == 0) {
        e->target = pick_target(s, e);
    }
    t = target_pos(s, e->target);
    see = sees(s, e->pos, t);
    d = flat(e->pos, t);
    e->hidden = see ? 0.0f : e->hidden + P08_DT;
    e->think -= P08_DT;
    e->cd -= P08_DT;
    e->grenade_cd -= P08_DT;
    if (e->think <= 0.0f) {
        cadv_squad_view v;
        int engaged = 0;
        for (k = 0; k < P08_ENEMIES; ++k) {
            if (k != i && s->e[k].active && s->e[k].kind == P08_E_SOLDIER && s->e[k].squad == e->squad &&
                (s->e[k].state == CADV_SQUAD_ENGAGE || s->e[k].state == CADV_SQUAD_FLANK || s->e[k].state == CADV_SQUAD_SUPPRESS)) {
                ++engaged;
            }
        }
        memset(&v, 0, sizeof v);
        v.sees_target = see && d < 45.0f;
        v.target_dist = d;
        v.hp = (float)e->hp / diff_scale(s, 50.0f, 70.0f, 90.0f);
        v.allies_engaged = engaged;
        v.has_grenade = e->grenade_cd <= 0.0f;
        v.target_hidden = e->hidden;
        v.flank_open = (i % 2) == 0;
        if (e->squad == 0 || (e->hidden > 30.0f && d > 40.0f)) {
            v.target_hidden = 30.0f;
        }
        e->state = (int)cadv_squad_decide((cadv_squad_state)e->state, &v);
        e->think = 0.5f;
        if (e->state == CADV_SQUAD_GRENADE && d < 25.0f) {
            cadv_vec3 to = cadv_v3_sub(t, e->pos);
            float fl = flat(t, e->pos) + 0.1f;
            throw_nade(s, cadv_v3(e->pos.x, 1.5f, e->pos.z), cadv_v3(to.x / fl * 10.0f, 4.0f, to.z / fl * 10.0f), true);
            e->grenade_cd = diff_scale(s, 14.0f, 9.0f, 6.0f);
            radio(s, "BLACKICE: Fire in the hole!");
        }
    }
    switch (e->state) {
    case CADV_SQUAD_FLANK: {
        float a = yaw_to(t, e->pos) + ((i % 2) ? 1.2f : -1.2f);
        cadv_vec3 g = cadv_v3(t.x - cadv_sinf(a) * 9.0f, 0, t.z - cadv_cosf(a) * 9.0f);
        cadv_nav_cell c = cadv_nav_cell_at(&s->nav, g.x, g.z);
        enemy_move(s, e, cadv_nav_walkable(&s->nav, c.x, c.z) ? g : t, 4.2f);
        break;
    }
    case CADV_SQUAD_RETREAT: {
        cadv_nav_cell c = cadv_nav_cell_at(&s->nav, e->pos.x, e->pos.z);
        cadv_nav_cell cov;
        c.x = (int8_t)(c.x + (e->pos.x > t.x ? 3 : -3));
        if (cadv_nav_nearest_cover(&s->nav, c, 4, &cov)) {
            enemy_move(s, e, cadv_nav_centre(&s->nav, cov), 4.4f);
        }
        break;
    }
    case CADV_SQUAD_PATROL:
        if (e->squad > 0) {
            enemy_move(s, e, t, 2.6f); /* hunting */
        }
        break;
    case CADV_SQUAD_ENGAGE:
        if (d > 18.0f) {
            enemy_move(s, e, t, 3.6f);
        }
        break;
    default:
        break;
    }
    if (see && d < 45.0f) {
        e->yaw = yaw_to(e->pos, t);
        if (e->cd <= 0.0f && (e->state == CADV_SQUAD_ENGAGE || e->state == CADV_SQUAD_SUPPRESS || e->state == CADV_SQUAD_RETREAT || e->state == CADV_SQUAD_FLANK)) {
            float moving = e->target < 0 ? cadv_sqrtf(s->body.vel.x * s->body.vel.x + s->body.vel.z * s->body.vel.z) : 0.0f;
            float chance = (e->state == CADV_SQUAD_SUPPRESS ? 0.3f : 0.55f) - d / 70.0f - moving / 25.0f;
            int shot;
            for (shot = 0; shot < 3; ++shot) {
                if (cadv_rng_float01(&s->rng) < chance) {
                    if (e->target < 0) {
                        hurt_player(s, 5);
                    } else {
                        hurt_npc(s, e->target, 5);
                    }
                }
            }
            e->cd = diff_scale(s, 1.8f, 1.3f, 1.0f);
            sfx_at(s, CADV_SFX_SHOT, 0.5f, e->pos);
        }
    }
}

/* ---- NPCs ---------------------------------------------------------------------------------- */

static void tick_npcs(p08_state *s)
{
    int i;
    for (i = 0; i < P08_NPCS; ++i) {
        p08_npc *n = &s->npc[i];
        if (!n->alive) {
            continue;
        }
        if (n->hurt > 0) {
            --n->hurt;
        }
        if (i == P08_NPC_C6P) {
            /* the maintenance bot sweeps the lobby, back and forth */
            float ph = (float)s->ticks * P08_DT * 0.2f;
            n->f.pos = cadv_v3(CC(22) + 6.0f + 5.0f * cadv_sinf(ph), 0, CC(6) + 1.0f);
            n->yaw = cadv_cosf(ph) > 0.0f ? -0.5f * CADV_PI : 0.5f * CADV_PI;
            continue;
        }
        if (flat(n->f.pos, s->body.pos) > 0.5f && n->f.mode != CADV_FOLLOW_WAIT) {
            n->yaw = yaw_to(n->f.pos, n->f.mode == CADV_FOLLOW_GOTO ? n->f.goal : s->body.pos);
        }
        cadv_follower_tick(&n->f, &s->nav, &s->world, s->body.pos, P08_DT);
        /* the guard shoots back */
        if (i == P08_NPC_BRANDT && n->met) {
            int k;
            n->cd -= P08_DT;
            for (k = 0; k < P08_ENEMIES && n->cd <= 0.0f; ++k) {
                p08_enemy *e = &s->e[k];
                if (e->active && !e->hidden_until_scan && flat(e->pos, n->f.pos) < 25.0f && sees(s, n->f.pos, e->pos)) {
                    n->yaw = yaw_to(n->f.pos, e->pos);
                    n->cd = 0.7f;
                    if (cadv_rng_float01(&s->rng) < 0.6f) {
                        hurt_enemy(s, k, 10);
                    }
                    sfx_at(s, CADV_SFX_SHOT, 0.4f, n->f.pos);
                }
            }
        }
    }
}

/* ---- the player's hands -------------------------------------------------------------------- */

static int nearest_npc(const p08_state *s, float within)
{
    int i;
    int best = -1;
    float bd = within;
    for (i = 0; i < P08_NPC_C6P; ++i) {
        float d = flat(s->npc[i].f.pos, s->body.pos);
        /* talk to whoever you are looking at */
        float off = cadv_absf(cadv_wrap_angle(yaw_to(s->body.pos, s->npc[i].f.pos) - s->body.yaw));
        if (s->npc[i].alive && d < bd && off < 0.6f) {
            bd = d;
            best = i;
        }
    }
    return best;
}

static bool near(const p08_state *s, cadv_vec3 p, float d) { return flat(s->body.pos, p) < d; }

static void use(p08_state *s)
{
    int i;
    /* the suit locker */
    if (!s->suit && near(s, cadv_p08_cell(29, 6), 2.6f)) {
        s->suit = true;
        s->body.armor = 25;
        set_solid(s, s->s_chamber, false);
        say(s, "THERMAL SUIT ON - THE TEST CHAMBER IS OPEN");
        radio(s, "MAREN: Suit's on? Good. Push the sample cart into the emitter when you're ready.");
        sfx_at(s, CADV_SFX_DOOR, 0.8f, s->body.pos);
        return;
    }
    if (!s->cart_pushed && s->suit && near(s, cadv_v3(s->cart_x, 0, 22.0f), 2.6f)) {
        s->cart_pushed = true;
        s->act = P08_ACT_TEST;
        say(s, "THE SAMPLE CART ROLLS INTO THE BEAM...");
        return;
    }
    if (s->act >= P08_ACT_RUINS && !s->generator_on && near(s, cadv_p08_cell(47, 6), 2.8f)) {
        if (s->cells_held >= 2) {
            s->generator_on = true;
            s->cells_held = 0;
            set_solid(s, s->s_gate, false);
            say(s, "POWER RESTORED - THE FREIGHT GATE OPENS");
            sfx_at(s, CADV_SFX_DOOR, 1.0f, s->body.pos);
        } else {
            char buf[64];
            snprintf(buf, sizeof buf, "THE GENERATOR NEEDS 2 POWER CELLS (%d)", s->cells_held);
            say(s, buf);
        }
        return;
    }
    if (!s->lab_open && near(s, cadv_p08_cell(53, 16), 3.0f)) {
        s->lab_open = true;
        set_solid(s, s->s_lab, false);
        for (i = P08_NPC_MAREN; i <= P08_NPC_LINDQVIST; ++i) {
            s->npc[i].met = true;
            s->npc[i].f.mode = CADV_FOLLOW_ON;
        }
        radio(s, "LINDQVIST: Ines! Thank God. The scanner by the array door only opens for staff - we'll come with you.");
        say(s, "THE SCIENTISTS FOLLOW YOU");
        return;
    }
    if (!s->scanner_open && near(s, cadv_p08_cell(61, 9), 3.0f)) {
        bool staff = false;
        for (i = P08_NPC_MAREN; i <= P08_NPC_LINDQVIST; ++i) {
            if (s->npc[i].alive && flat(s->npc[i].f.pos, cadv_p08_cell(61, 9)) < 3.5f) {
                staff = true;
            }
        }
        if (staff) {
            s->scanner_open = true;
            set_solid(s, s->s_scanner, false);
            say(s, "RETINA ACCEPTED - THE ARRAY DOOR OPENS");
            sfx_at(s, CADV_SFX_DOOR, 1.0f, s->body.pos);
        } else {
            say(s, "RETINA NOT RECOGNISED - BRING A SCIENTIST");
            sfx_at(s, CADV_SFX_DENIED, 0.8f, s->body.pos);
        }
        return;
    }
    if (s->act == P08_ACT_ARRAY) {
        static const int k_breakers[3][2] = {{65, 5}, {77, 11}, {74, 17}};
        for (i = 0; i < 3; ++i) {
            cadv_vec3 b = cadv_p08_cell(k_breakers[i][0], k_breakers[i][1]);
            if (!(s->breakers & (1 << i)) && near(s, b, 2.2f)) {
                char buf[48];
                s->breakers |= 1 << i;
                snprintf(buf, sizeof buf, "BREAKER %d OF 3", (s->breakers & 1) + ((s->breakers >> 1) & 1) + ((s->breakers >> 2) & 1));
                say(s, buf);
                sfx_at(s, CADV_SFX_BELL, 0.8f, s->body.pos);
                if (s->breakers == 7) {
                    s->star_active = true;
                    s->star_pos = cadv_v3(s->body.pos.x + 1.0f, s->body.pos.y + 6.0f, s->body.pos.z);
                    say(s, "THE BEAM REVERSES - SOMETHING FALLS FROM THE SKY");
                    radio(s, "OKAFOR: It's collapsing back into itself! Ines - the shard, it's coming down!");
                    sfx_at(s, CADV_SFX_EXPLOSION, 1.0f, s->body.pos);
                }
                return;
            }
        }
    }
    /* people: follow / wait */
    i = nearest_npc(s, 2.6f);
    if (i >= 0 && (i != P08_NPC_BRANDT || s->act >= P08_ACT_RUINS) && (s->npc[i].met || i == P08_NPC_BRANDT) && s->act >= P08_ACT_RUINS) {
        char buf[72];
        s->npc[i].met = true;
        if (s->npc[i].f.mode == CADV_FOLLOW_GOTO) {
            s->npc[i].f.mode = CADV_FOLLOW_WAIT;
        }
        cadv_follower_toggle(&s->npc[i].f);
        snprintf(buf, sizeof buf, "%s: %s", k_npc_names[i], s->npc[i].f.mode == CADV_FOLLOW_ON ? "RIGHT BEHIND YOU." : "I'LL WAIT HERE.");
        say(s, buf);
        if (i == P08_NPC_BRANDT && s->npc[i].f.mode == CADV_FOLLOW_ON) {
            radio(s, "BRANDT: Those things got half my team. I'm with you, tech - I've still got a sidearm.");
        }
    }
}

static void pickups(p08_state *s)
{
    int i;
    for (i = 0; i < P08_PICKUPS; ++i) {
        p08_pickup *p = &s->pk[i];
        if (!p->active || flat(p->pos, s->body.pos) > 1.1f || cadv_absf(p->pos.y - s->body.pos.y) > 1.3f) {
            continue;
        }
        p->active = false;
        switch (p->kind) {
        case P08_PK_AXE:
            give(s, P08_W_AXE);
            s->weapon = P08_W_AXE;
            say(s, "ICE AXE");
            break;
        case P08_PK_PISTOL:
            give(s, P08_W_PISTOL);
            s->weapon = P08_W_PISTOL;
            say(s, "9MM PISTOL");
            break;
        case P08_PK_SHOTGUN:
            give(s, P08_W_SHOTGUN);
            s->weapon = P08_W_SHOTGUN;
            say(s, "SHOTGUN");
            break;
        case P08_PK_CROSSBOW:
            give(s, P08_W_CROSSBOW);
            say(s, "CROSSBOW");
            break;
        case P08_PK_GRENADE:
            s->owned[P08_W_GRENADE] = true;
            s->reserve[P08_W_GRENADE] += 2;
            say(s, "GRENADES");
            break;
        case P08_PK_AMMO: {
            int w;
            for (w = P08_W_PISTOL; w <= P08_W_CROSSBOW; ++w) {
                s->reserve[w] += k_weapons[w].mag;
            }
            say(s, "AMMUNITION");
            break;
        }
        case P08_PK_MEDKIT:
            s->body.health = cadv_mini(100, s->body.health + 25);
            say(s, "MEDKIT");
            break;
        default:
            ++s->cells_held;
            say(s, "POWER CELL");
            break;
        }
        sfx_at(s, CADV_SFX_PICKUP, 0.6f, s->body.pos);
    }
}

/* the crate slides when walked into, so it can be climbed on */
static void push_crate(p08_state *s, const cadv_input_frame *in)
{
    cadv_aabb *b = &s->world.s[s->s_crate].box;
    /* walls zero the velocity, so the push follows what the player wants */
    float y = s->body.yaw;
    float mx = -cadv_sinf(y) * in->move_y + cadv_cosf(y) * in->move_x;
    float mz = -cadv_cosf(y) * in->move_y - cadv_sinf(y) * in->move_x;
    float dx = s->crate_x - s->body.pos.x;
    float dz = s->crate_z - s->body.pos.z;
    if (s->body.pos.y > 0.3f || cadv_absf(dx) > 1.35f || cadv_absf(dz) > 1.35f) {
        return;
    }
    if (cadv_absf(mx) > 0.5f && cadv_absf(dx) > cadv_absf(dz) && dx * mx > 0.0f) {
        float nx = s->crate_x + (mx > 0.0f ? 1.0f : -1.0f) * 2.0f * P08_DT;
        cadv_aabb t = cadv_aabb_make(cadv_v3(nx - 0.9f, 0.05f, s->crate_z - 0.9f), cadv_v3(nx + 0.9f, 0.8f, s->crate_z + 0.9f));
        s->world.s[s->s_crate].enabled = false;
        if (!cadv_world_overlaps(&s->world, t)) {
            s->crate_x = nx;
        }
        s->world.s[s->s_crate].enabled = true;
    } else if (cadv_absf(mz) > 0.5f && cadv_absf(dz) >= cadv_absf(dx) && dz * mz > 0.0f) {
        float nz = s->crate_z + (mz > 0.0f ? 1.0f : -1.0f) * 2.0f * P08_DT;
        cadv_aabb t = cadv_aabb_make(cadv_v3(s->crate_x - 0.9f, 0.05f, nz - 0.9f), cadv_v3(s->crate_x + 0.9f, 0.8f, nz + 0.9f));
        s->world.s[s->s_crate].enabled = false;
        if (!cadv_world_overlaps(&s->world, t)) {
            s->crate_z = nz;
        }
        s->world.s[s->s_crate].enabled = true;
    }
    *b = cadv_aabb_make(cadv_v3(s->crate_x - 0.9f, 0, s->crate_z - 0.9f), cadv_v3(s->crate_x + 0.9f, 0.8f, s->crate_z + 0.9f));
}

static void hands(p08_state *s, const cadv_input_frame *in)
{
    int i;
    for (i = 0; i < P08_WEAPONS; ++i) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + i)) && s->owned[i]) {
            s->weapon = i;
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        int step = cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) ? 1 : P08_WEAPONS - 1;
        for (i = 0; i < P08_WEAPONS; ++i) {
            s->weapon = (s->weapon + step) % P08_WEAPONS;
            if (s->owned[s->weapon]) {
                break;
            }
        }
    }
    s->zoom = s->weapon == P08_W_CROSSBOW && cadv_input_down(in, CADV_ACT_ALT_FIRE);
    s->shot_cd -= P08_DT;
    if (cadv_input_pressed(in, CADV_ACT_USE)) {
        use(s);
    }
    /* wall chargers: hold use */
    if (cadv_input_down(in, CADV_ACT_USE) && s->suit) {
        for (i = 0; i < 3; ++i) {
            if (near(s, s->charger_pos[i], 2.0f)) {
                float v = (float)s->body.armor + s->suit_frac;
                float before = v;
                cadv_charger_use(&s->chargers[i], &v, 100.0f, P08_DT * 3.0f);
                s->body.armor = (int)v;
                s->suit_frac = v - (float)s->body.armor; /* keep the fraction */
                if (v > before && s->ticks % 20 == 0) {
                    sfx_at(s, CADV_SFX_PICKUP, 0.2f, s->body.pos);
                }
            }
        }
    }
    if (cadv_input_down(in, CADV_ACT_FIRE) && s->shot_cd <= 0.0f && s->owned[s->weapon] &&
        (s->weapon != P08_W_GRENADE || cadv_input_pressed(in, CADV_ACT_FIRE))) {
        fire(s);
    }
}

/* ---- marks -------------------------------------------------------------------------------------- */

static void check_marks(p08_state *s)
{
    char buf[160];
    if (!(s->marks_found & 1u) && near(s, cadv_v3(CC(31) - 1.5f, 0, 22.0f), 4.0f) && s->act <= P08_ACT_TEST) {
        s->marks_found |= 1u;
        snprintf(buf, sizeof buf, "THE DOOR SIGN: \"%s\"", s->mark_text[0]);
        say(s, buf);
    }
    if (!(s->marks_found & 2u) && near(s, s->npc[P08_NPC_C6P].f.pos, 4.0f)) {
        s->marks_found |= 2u;
        snprintf(buf, sizeof buf, "%s: *BEEP* MIND THE WET FLOOR *BEEP*", s->mark_text[1]);
        radio(s, buf);
    }
    if (!(s->marks_found & 4u) && near(s, cadv_p08_cell(42, 4), 3.0f)) {
        int k;
        s->marks_found |= 4u;
        snprintf(buf, sizeof buf, "THE NAMEPLATE: \"%s\" - THE SCANNER LIGHTS UP", s->mark_text[2]);
        say(s, buf);
        /* the scanner shows what hides in the walls */
        for (k = 0; k < P08_ENEMIES; ++k) {
            s->e[k].hidden_until_scan = false;
        }
    }
}

/* ---- tick --------------------------------------------------------------------------------------- */

void cadv_p08_tick(void *state, const cadv_input_frame *in)
{
    p08_state *s = (p08_state *)state;
    cadv_vec3 before;
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
    if (s->flash > 0) {
        --s->flash;
    }
    if (s->win_timer > 0) {
        if (--s->win_timer == 0 || (s->win_timer < 200 && cadv_input_pressed(in, CADV_ACT_USE))) {
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
    /* the monorail: the player rides inside the moving car */
    if (s->tram_moving) {
        float v = 4.0f * P08_DT;
        s->tram_x += v;
        s->body.pos.x += v;
        if (s->tram_x >= 36.0f) {
            s->tram_moving = false;
            s->act = P08_ACT_LOBBY;
            say(s, "POLARIS DEEP - SUBLEVEL THREE. THE DOORS OPEN");
            sfx_at(s, CADV_SFX_DOOR, 0.8f, s->body.pos);
        }
        place_tram(s);
    }
    before = s->body.pos;
    cadv_player_tick(&s->body, &s->world, in, P08_DT);
    if (s->body.pos.y < -5.0f) {
        s->body.pos.y = 0.0f; /* GCOVR_EXCL_LINE: the floor is solid everywhere */
    }
    push_crate(s, in);
    (void)before;
    hands(s, in);
    pickups(s);
    /* the cart rolls on its rail into the emitter */
    if (s->cart_pushed && s->cascade_t == 0) {
        s->cart_x += 1.2f * P08_DT;
        if (s->cart_x >= CC(37)) {
            start_cascade(s);
        }
    }
    if (s->cascade_t > 0) {
        ++s->cascade_t;
    }
    for (i = 0; i < P08_ENEMIES; ++i) {
        p08_enemy *e = &s->e[i];
        if (!e->active || e->hidden_until_scan) {
            continue;
        }
        if (e->hurt > 0) {
            --e->hurt;
        }
        if (e->kind == P08_E_SKITTER) {
            tick_skitter(s, i);
        } else {
            tick_soldier(s, i);
        }
    }
    tick_npcs(s);
    tick_nades(s);
    check_marks(s);
    /* scripted beats: triggers that never take the controls away */
    if (cadv_trigger_update(&s->t_lobby, s->body.pos)) {
        radio(s, "OKAFOR: Morning, Ines. Suits are in the locker by the window - today's the big one.");
    }
    if (cadv_trigger_update(&s->t_ruins, s->body.pos) && s->cascade_t > 0) {
        if (s->checkpoint < 1) {
            s->checkpoint = 1;
        }
        radio(s, "BRANDT: Over here... tech? Help me up. Something's crawling in the ducts.");
    }
    cadv_trigger_update(&s->t_radiology, s->body.pos);
    if (cadv_trigger_update(&s->t_blackice, s->body.pos) && s->generator_on) {
        s->act = P08_ACT_BLACKICE;
        if (s->checkpoint < 2) {
            s->checkpoint = 2;
        }
        send_squad(s, 1);
    }
    if (s->lab_open && s->squad_sent == 1) {
        /* the second team comes once the first is down (or after a while) */
        int left = 0;
        for (i = 0; i < P08_ENEMIES; ++i) {
            left += s->e[i].active && s->e[i].kind == P08_E_SOLDIER && s->e[i].squad == 1;
        }
        if (left == 0 || ++s->lab_t > 40 * 60) {
            send_squad(s, 2);
        }
    }
    if (s->scanner_open && cadv_trigger_update(&s->t_array, s->body.pos)) {
        enter_array(s);
    }
    /* the array: a blizzard, skitterlings, and the thing under the ice */
    if (s->act == P08_ACT_ARRAY) {
        if (--s->slam_cd <= 0 && !s->slam.active && !s->star_active) {
            s->slam.active = true;
            s->slam.pos = s->body.pos;
            s->slam.timer = 70;
            s->slam_cd = (int)diff_scale(s, 260.0f, 190.0f, 140.0f);
            say(s, "THE ICE SPLITS - MOVE!");
        }
        if (s->slam.active && --s->slam.timer <= 0) {
            s->slam.active = false;
            sfx_at(s, CADV_SFX_EXPLOSION, 1.0f, s->slam.pos);
            if (flat(s->body.pos, s->slam.pos) < 3.0f) {
                hurt_player(s, 30);
            }
        }
        if (--s->skitter_cd <= 0 && cadv_p08_enemies_alive(s) < (int)diff_scale(s, 2.0f, 3.0f, 5.0f) && !s->star_active) {
            spawn_enemy(s, P08_E_SKITTER, CC(64 + (int)cadv_rng_below(&s->rng, 14)), CC(4), 0);
            s->skitter_cd = (int)diff_scale(s, 480.0f, 360.0f, 240.0f);
        }
    }
    /* the shard falls at your feet */
    if (s->star_active) {
        float ground = s->body.on_ground ? s->body.pos.y : 0.0f;
        s->star_pos.y = cadv_maxf(ground + 0.9f, s->star_pos.y - 4.0f * P08_DT);
        if (cadv_v3_dist(s->star_pos, cadv_v3(s->body.pos.x, s->body.pos.y + 0.9f, s->body.pos.z)) < 1.8f && s->star_pos.y <= ground + 1.5f) {
            s->star_active = false;
            s->star_taken = true;
            s->act = P08_ACT_DONE;
            s->win_timer = 300;
            say(s, "THE LUMEN SHARD - THE SHINING STAR!");
            sfx_at(s, CADV_SFX_STAR, 1.0f, s->body.pos);
        }
    }
}

/* ---- results & scripting -------------------------------------------------------------------- */

cadv_portal_result cadv_p08_result(const void *state)
{
    const p08_state *s = (const p08_state *)state;
    cadv_portal_result res;
    memset(&res, 0, sizeof res);
    res.outcome = s->outcome;
    res.star_collected = s->star_taken;
    res.checkpoint = s->checkpoint;
    res.marks_found = s->marks_found;
    res.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        res.snowflakes = 1 + (cadv_p08_scientists_alive(s) == 3 ? 1 : 0) + (s->ticks < 16 * 60 * 60 ? 1 : 0);
    }
    return res;
}

bool cadv_p08_query(const void *state, const char *key, double *out)
{
    const p08_state *s = (const p08_state *)state;
    cadv_vec3 e = cadv_p08_eye(s);
    struct kv { const char *k; double v; };
    const struct kv table[] = {
        {"player.x", e.x},
        {"player.y", e.y},
        {"player.z", e.z},
        {"player.yaw", s->body.yaw},
        {"player.health", s->body.health},
        {"suit", s->body.armor},
        {"act", s->act},
        {"weapon", s->weapon},
        {"cells", s->cells_held},
        {"generator", s->generator_on},
        {"lab", s->lab_open},
        {"scanner", s->scanner_open},
        {"breakers", s->breakers},
        {"scientists", cadv_p08_scientists_alive(s)},
        {"following", (s->npc[0].f.mode == CADV_FOLLOW_ON) + (s->npc[1].f.mode == CADV_FOLLOW_ON) + (s->npc[2].f.mode == CADV_FOLLOW_ON) + (s->npc[3].f.mode == CADV_FOLLOW_ON)},
        {"enemies", cadv_p08_enemies_alive(s)},
        {"cascade", s->cascade_t > 0},
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

void cadv_p08_teleport(void *state, float x, float y, float z, float yaw)
{
    p08_state *s = (p08_state *)state;
    s->body.pos = cadv_v3(x, y, z);
    s->body.vel = cadv_v3(0, 0, 0);
    s->body.yaw = yaw;
    s->body.pitch = 0.0f;
}

/* ---- completion bot -------------------------------------------------------------------------- */

static void bot_press(cadv_input_frame *out, cadv_action a)
{
    out->pressed |= CADV_ACT_BIT(a);
    out->held |= CADV_ACT_BIT(a);
}

static bool reachable(p08_state *s, cadv_vec3 p)
{
    return cadv_nav_path(&s->nav, cadv_nav_cell_at(&s->nav, s->body.pos.x, s->body.pos.z), cadv_nav_cell_at(&s->nav, p.x, p.z), s->bot_path, 96) >= 0;
}

/* Walk the grid towards a world point; returns the distance left. */
static float bot_walk(p08_state *s, cadv_vec3 goal, cadv_input_frame *out, bool look)
{
    cadv_nav_cell here = cadv_nav_cell_at(&s->nav, s->body.pos.x, s->body.pos.z);
    cadv_nav_cell g = cadv_nav_cell_at(&s->nav, goal.x, goal.z);
    cadv_vec3 step = goal;
    float d = flat(s->body.pos, goal);
    if (!cadv_nav_walkable(&s->nav, here.x, here.z)) {
        /* off the grid (dodged into a corner): back onto the nearest open cell */
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
        int n = cadv_nav_path(&s->nav, here, g, s->bot_path, 96);
        s->bot_path_len = n > 0 ? n : 0;
        s->bot_path_pos = 0;
        s->bot_goal_x = g.x;
        s->bot_goal_z = g.z;
    }
    while (s->bot_path_pos < s->bot_path_len) {
        cadv_vec3 c = cadv_nav_centre(&s->nav, s->bot_path[s->bot_path_pos]);
        if (flat(c, s->body.pos) < 0.7f && s->bot_path_pos + 1 < s->bot_path_len) {
            ++s->bot_path_pos;
        } else {
            step = s->bot_path_pos + 1 < s->bot_path_len ? c : goal;
            break;
        }
    }
    if (d > 0.4f) {
        float y = s->body.yaw;
        float sx = step.x - s->body.pos.x;
        float sz = step.z - s->body.pos.z;
        float sl = cadv_sqrtf(sx * sx + sz * sz) + 1e-5f;
        out->move_y = (sx * -cadv_sinf(y) + sz * -cadv_cosf(y)) / sl;
        out->move_x = (sx * cadv_cosf(y) + sz * -cadv_sinf(y)) / sl;
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

static int bot_enemy(const p08_state *s)
{
    int best = -1;
    float bd = 32.0f;
    int i;
    cadv_vec3 e = cadv_p08_eye(s);
    for (i = 0; i < P08_ENEMIES; ++i) {
        const p08_enemy *en = &s->e[i];
        float d;
        if (!en->active || en->hidden_until_scan) {
            continue;
        }
        d = flat(en->pos, s->body.pos);
        if (d < bd && cadv_world_line_of_sight(&s->world, e, cadv_v3(en->pos.x, en->pos.y + enemy_height(en->kind) * 0.6f, en->pos.z))) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void cadv_p08_bot(const void *state, cadv_input_frame *out)
{
    p08_state *s = (p08_state *)(uintptr_t)state; /* notes in the bot fields */
    static const int k_breakers[3][2] = {{65, 5}, {77, 11}, {74, 17}};
    cadv_vec3 goal = s->body.pos;
    bool use_here = false;
    int target;
    float d;
    int i;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0) {
        if (s->win_timer < 180) {
            bot_press(out, CADV_ACT_USE);
        }
        return;
    }
    /* the story graph */
    if (s->tram_moving) {
        return;
    }
    if (!s->suit) {
        goal = cadv_p08_cell(29, 6);
        use_here = true;
    } else if (!s->cart_pushed) {
        goal = cadv_v3(s->cart_x - 1.5f, 0, 22.0f);
        use_here = true;
    } else if (s->cascade_t == 0) {
        goal = cadv_p08_cell(30, 11); /* step back from the beam */
    } else if (!s->owned[P08_W_PISTOL] && s->pk[1].active) {
        goal = s->pk[0].active ? s->pk[0].pos : s->pk[1].pos;
    } else if (!s->generator_on && !s->npc[P08_NPC_BRANDT].met) {
        goal = s->npc[P08_NPC_BRANDT].f.pos;
        use_here = true;
    } else if (!s->generator_on && s->cells_held < 2) {
        goal = s->pk[13].active ? s->pk[13].pos : s->pk[14].pos;
    } else if (!s->generator_on) {
        goal = cadv_p08_cell(46, 6);
        use_here = true;
    } else if (!s->lab_open) {
        goal = cadv_p08_cell(53, 15);
        use_here = true;
    } else if (!s->scanner_open) {
        goal = cadv_p08_cell(61, 9);
        /* wait for a scientist to catch up before using the scanner */
        use_here = false;
        for (i = P08_NPC_MAREN; i <= P08_NPC_LINDQVIST; ++i) {
            if (s->npc[i].alive && flat(s->npc[i].f.pos, goal) < 3.0f) {
                use_here = true;
            }
            if (s->npc[i].alive && s->npc[i].f.mode == CADV_FOLLOW_WAIT && s->lab_open) {
                /* a scientist left behind: go and fetch them */
                goal = s->npc[i].f.pos;
                if (flat(goal, s->body.pos) < 2.0f && s->ticks % 30 == 0) {
                    bot_press(out, CADV_ACT_USE);
                }
                break;
            }
        }
    } else if (s->act != P08_ACT_ARRAY && s->act != P08_ACT_DONE) {
        goal = cadv_p08_cell(64, 11);
    } else if (s->star_active) {
        goal = s->star_pos;
    } else {
        for (i = 0; i < 3; ++i) {
            if (!(s->breakers & (1 << i))) {
                goal = cadv_p08_cell(k_breakers[i][0], k_breakers[i][1]);
                use_here = true;
                break;
            }
        }
    }
    target = bot_enemy(s);
    /* patch up on the way: a medkit or a suit charger close by */
    if (target < 0 && !s->star_active) {
        for (i = 0; i < P08_PICKUPS; ++i) {
            if (s->pk[i].active && s->pk[i].kind == P08_PK_MEDKIT && s->body.health < 70 && flat(s->pk[i].pos, s->body.pos) < 28.0f &&
                cadv_absf(s->pk[i].pos.y - s->body.pos.y) < 1.0f && s->ticks % 30 == 0 ? reachable(s, s->pk[i].pos) : s->pk[i].active && s->pk[i].kind == P08_PK_MEDKIT && s->body.health < 70 && flat(s->pk[i].pos, s->body.pos) < 28.0f && s->bot_detour == i + 1) {
                s->bot_detour = i + 1;
                goal = s->pk[i].pos;
                use_here = false;
                break;
            }
        }
        for (i = 0; i < 3; ++i) {
            if (s->body.armor < 50 && s->suit && s->act != P08_ACT_ARRAY && s->chargers[i].reserve > 5.0f && flat(s->charger_pos[i], s->body.pos) < 26.0f &&
                (s->bot_charger == i + 1 || (s->ticks % 30 == 0 && reachable(s, s->charger_pos[i])))) {
                s->bot_charger = i + 1;
                goal = s->charger_pos[i];
                use_here = false;
                break;
            }
        }
    }
    /* dodge the slam */
    if (s->slam.active && flat(s->body.pos, s->slam.pos) < 4.0f) {
        /* step aside to an open cell away from the impact */
        int k;
        for (k = 0; k < 8; ++k) {
            float a = (float)k * 0.785f;
            cadv_vec3 away = cadv_v3(s->slam.pos.x + cadv_cosf(a) * 4.5f, 0, s->slam.pos.z + cadv_sinf(a) * 4.5f);
            cadv_nav_cell c = cadv_nav_cell_at(&s->nav, away.x, away.z);
            if (cadv_nav_walkable(&s->nav, c.x, c.z)) {
                goal = away;
                break;
            }
        }
        use_here = false;
    }
    if (use_here && flat(s->body.pos, goal) < 1.0f) {
        d = flat(s->body.pos, goal); /* close enough: stop and face it */
    } else {
        d = bot_walk(s, goal, out, target < 0);
    }
    if (use_here && d < 1.6f) {
        if (target < 0) {
            out->look_x = cadv_clampf(cadv_wrap_angle(yaw_to(s->body.pos, goal) - s->body.yaw), -0.2f, 0.2f);
        }
        if (s->ticks % 15 == 0) {
            bot_press(out, CADV_ACT_USE);
        }
    }
    /* topping up the suit at a charger on the way */
    for (i = 0; i < 3 && target < 0; ++i) {
        if (s->body.armor < 60 && s->chargers[i].reserve > 1.0f && flat(s->body.pos, s->charger_pos[i]) < 1.9f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_USE);
        }
    }
    if (target >= 0) {
        const p08_enemy *en = &s->e[target];
        cadv_vec3 e = cadv_p08_eye(s);
        cadv_vec3 at = cadv_v3(en->pos.x, en->pos.y + enemy_height(en->kind) * 0.6f, en->pos.z);
        float ed = flat(e, at);
        float dy = cadv_wrap_angle(yaw_to(e, at) - s->body.yaw);
        float dp = cadv_atan2f(at.y - e.y, ed) - s->body.pitch;
        int want = s->owned[P08_W_SHOTGUN] && (s->mag[P08_W_SHOTGUN] + s->reserve[P08_W_SHOTGUN]) > 0 && ed < 10.0f ? P08_W_SHOTGUN
                   : s->owned[P08_W_PISTOL] && (s->mag[P08_W_PISTOL] + s->reserve[P08_W_PISTOL]) > 0 ? P08_W_PISTOL
                   : s->owned[P08_W_CROSSBOW] && (s->mag[P08_W_CROSSBOW] + s->reserve[P08_W_CROSSBOW]) > 0 ? P08_W_CROSSBOW
                                                                                                         : P08_W_AXE;
        out->look_x = cadv_clampf(dy, -0.2f, 0.2f);
        out->look_y = cadv_clampf(dp, -0.1f, 0.1f);
        if (s->weapon != want) {
            bot_press(out, (cadv_action)(CADV_ACT_WEAPON_1 + want));
        } else if (cadv_absf(dy) < 0.06f && cadv_absf(dp) < 0.08f && (want != P08_W_AXE || ed < 1.8f)) {
            bot_press(out, CADV_ACT_FIRE);
        }
        /* stand and fight unless it is a skitterling leaping in */
        if (ed < 12.0f && en->kind == P08_E_SOLDIER) {
            out->move_x = cadv_sinf((float)s->ticks * 0.05f);
            out->move_y = 0.0f;
        }
        if (want == P08_W_AXE && ed > 1.4f) {
            out->move_y = 1.0f;
            out->move_x = 0.0f;
        }
    }
}
