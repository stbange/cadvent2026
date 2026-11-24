/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P08_INTERNAL_H
#define CADV_PORTALS_P08_INTERNAL_H

#include "core/rng.h"
#include "gfx/gl_util.h"
#include "kits/fps/player.h"
#include "kits/fps/story.h"
#include "kits/fps/tactics.h"
#include "portals/p08_cascade.h"

#define P08_DT (1.0f / 60.0f)
#define P08_W 80
#define P08_H 22
#define P08_CELL 2.0f
#define P08_WALL_H 4.0f
#define P08_ENEMIES 32
#define P08_NPCS 5           /* three scientists, the guard, the maintenance bot */
#define P08_PICKUPS 24
#define P08_NADES 6
#define P08_STEPS 6

/* the story, in order; never a cutscene, always the player's own pace */
enum { P08_ACT_TRAM, P08_ACT_LOBBY, P08_ACT_TEST, P08_ACT_RUINS, P08_ACT_BLACKICE, P08_ACT_ARRAY, P08_ACT_DONE };
enum { P08_W_AXE, P08_W_PISTOL, P08_W_SHOTGUN, P08_W_CROSSBOW, P08_W_GRENADE, P08_WEAPONS };
enum { P08_E_SKITTER, P08_E_SOLDIER };
enum { P08_PK_PISTOL, P08_PK_SHOTGUN, P08_PK_CROSSBOW, P08_PK_GRENADE, P08_PK_AMMO, P08_PK_MEDKIT, P08_PK_CELL, P08_PK_AXE };
enum { P08_NPC_MAREN, P08_NPC_OKAFOR, P08_NPC_LINDQVIST, P08_NPC_BRANDT, P08_NPC_C6P };
/* doors and movable things (world solid tags) */
enum { P08_TAG_WALL = 1, P08_TAG_CHAMBER = 10, P08_TAG_BREACH, P08_TAG_GATE, P08_TAG_LAB, P08_TAG_SCANNER, P08_TAG_CRATE, P08_TAG_LEDGE,
       P08_TAG_STEP, P08_TAG_TOWER, P08_TAG_TRAM };

typedef struct p08_weapon_def {
    const char *name;
    int damage;
    int pellets;
    float rate;
    int mag, reserve;
    float range;
    float spread;
} p08_weapon_def;

typedef struct p08_enemy {
    bool active;
    int kind;
    cadv_vec3 pos;
    cadv_vec3 vel;
    float yaw;
    int hp;
    float cd;
    float grenade_cd;
    int state;              /* cadv_squad_state for soldiers */
    float think;
    float hidden;           /* seconds the target has been out of sight */
    int target;             /* -1 player, else npc index */
    cadv_nav_cell path[48];
    int path_len, path_pos;
    int squad;
    int hurt;
    bool hidden_until_scan; /* radiology skitterlings */
} p08_enemy;

typedef struct p08_npc {
    bool alive;
    bool met;
    cadv_follower f;
    float yaw;
    int hp;
    float cd;
    int hurt;
} p08_npc;

typedef struct p08_pickup {
    bool active;
    int kind;
    cadv_vec3 pos;
} p08_pickup;

typedef struct p08_nade {
    bool active;
    bool enemy;
    cadv_vec3 pos, vel;
    float fuse;
} p08_nade;

typedef struct p08_slam {
    bool active;
    cadv_vec3 pos;
    int timer;              /* ticks until it lands; then a few ticks of impact */
} p08_slam;

typedef struct p08_gpu {
    cadv_gpu_mesh floor, walls, ceiling, props, cube, plaque, star, tram, skitter, crate;
    unsigned tex_panel, tex_floor, tex_hazard, tex_sign[3], tex_snow;
    int sign_w[3];
} p08_gpu;

typedef struct p08_state {
    cadv_portal_cfg cfg;
    cadv_rng rng;
    cadv_world world;
    cadv_nav nav;
    uint8_t cells[P08_W * P08_H];   /* 0 floor, 1 wall, 2 prop */
    cadv_player body;
    int act;
    int weapon;
    bool owned[P08_WEAPONS];
    int mag[P08_WEAPONS], reserve[P08_WEAPONS];
    float shot_cd;
    bool zoom;
    bool suit;
    float suit_frac;                /* fraction of a point from the chargers */
    int cells_held;                 /* power cells carried */
    bool generator_on;
    bool lab_open;
    int lab_t;                      /* ticks since the lab opened */
    bool scanner_open;
    int breakers;                   /* bit per breaker */
    float tram_x;
    bool tram_moving;
    float cart_x;
    bool cart_pushed;
    int cascade_t;                  /* ticks since the cascade, 0 = not yet */
    int s_chamber, s_breach, s_gate, s_lab, s_scanner, s_crate, s_tram[3];
    float crate_x, crate_z;
    p08_enemy e[P08_ENEMIES];
    p08_npc npc[P08_NPCS];
    p08_pickup pk[P08_PICKUPS];
    p08_nade nades[P08_NADES];
    p08_slam slam;
    int slam_cd;
    int skitter_cd;
    cadv_charger chargers[3];
    cadv_vec3 charger_pos[3];
    cadv_trigger t_ruins, t_blackice, t_array, t_lobby, t_radiology;
    int squad_sent;
    bool star_active, star_taken;
    cadv_vec3 star_pos;
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    int win_timer;
    int lose_timer;
    int hurt_flash;
    int flash;                      /* the cascade's white flash */
    char message[72];
    int message_timer;
    char radio[110];
    int radio_timer;
    char mark_text[3][64];
    /* bot bookkeeping */
    int bot_step;
    int bot_detour, bot_charger;   /* the medkit / charger being fetched, +1 */
    int bot_stuck;
    cadv_vec3 bot_last;
    cadv_nav_cell bot_path[96];
    int bot_path_len, bot_path_pos, bot_goal_x, bot_goal_z;
    bool gpu_ready;
    p08_gpu g;
} p08_state;

const p08_weapon_def *cadv_p08_weapons(void);
cadv_vec3 cadv_p08_eye(const p08_state *s);
cadv_vec3 cadv_p08_npc_pos(const p08_state *s, int i);
const char *cadv_p08_npc_name(int i);
int cadv_p08_enemies_alive(const p08_state *s);
int cadv_p08_scientists_alive(const p08_state *s);
cadv_vec3 cadv_p08_cell(int cx, int cz); /* centre of a cell at floor level */

#endif
