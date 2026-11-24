/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P10_INTERNAL_H
#define CADV_PORTALS_P10_INTERNAL_H

#include "core/rng.h"
#include "gfx/gl_util.h"
#include "kits/fps/facade.h"
#include "kits/fps/player.h"
#include "kits/fps/story.h"
#include "kits/fps/tactics.h"
#include "kits/rpg/party.h"
#include "kits/rpg/persuade.h"
#include "portals/p10_neon_syndic.h"

#define P10_DT (1.0f / 60.0f)
#define P10_W 96
#define P10_H 64
#define P10_CELL 2.0f
#define P10_AGENTS 3
#define P10_CIVS 48
#define P10_ENEMIES 40
#define P10_FACADES 8
#define P10_SHOTS 12
#define P10_TRACERS 16
#define P10_HOLD (60 * 60)        /* the VTOL is a minute out */
#define P10_LAMPS 24

enum { P10_ACT_MARKET, P10_ACT_INDUSTRIAL, P10_ACT_TOWER, P10_ACT_STAR, P10_ACT_PLAZA, P10_ACT_DONE };
enum { P10_W_MINIGUN, P10_W_LASER, P10_W_GAUSS, P10_WEAPONS };
enum { P10_E_GUARD, P10_E_ENFORCER, P10_E_TURRET };
enum { P10_CIV_WANDER, P10_CIV_FOLLOW, P10_CIV_FLEE, P10_CIV_DOWN };
enum { P10_FAC_BARRIER = 0 };  /* the road barrier is facade 0 */

typedef struct p10_agent {
    bool alive;
    cadv_follower f;
    float yaw;
    int hp;
    int order;              /* cadv_order */
    int target;             /* enemy index or -1 */
    float cd;
    cadv_vec3 hold;
    int hurt;
} p10_agent;

typedef struct p10_civ {
    bool active;
    cadv_follower f;
    float yaw;
    int state;
    float charm;            /* persuasion progress */
    int hp;
    float think;
} p10_civ;

typedef struct p10_enemy {
    bool active;
    int kind;
    bool ally;              /* persuaded: fights for you */
    cadv_follower f;        /* turrets never move */
    float yaw;
    int hp;
    float cd;
    float charm;
    int target;             /* -1 player, 0..2 agent, 100+ civ, 200+ enemy */
    cadv_vec3 post;
    float think;
    int hurt;
} p10_enemy;

typedef struct p10_car {
    cadv_vec3 pos;
    float yaw;
    float speed;
    bool driven;
} p10_car;

typedef struct p10_shot {
    bool active;
    cadv_vec3 pos, vel;
    float fuse;
} p10_shot;

typedef struct p10_tracer {
    cadv_vec3 a, b;
    int ttl;
    int kind;               /* 0 bullet, 1 laser */
} p10_tracer;

typedef struct p10_gpu {
    cadv_gpu_mesh street, blocks, roofs, windows, props, cube, plaque, star, car, vtol, facade[P10_FACADES];
    uint32_t facade_ver[P10_FACADES];
    unsigned tex_street, tex_wall, tex_sign[3], tex_ad[4];
    int sign_w[3], ad_w[4];
} p10_gpu;

typedef struct p10_state {
    cadv_portal_cfg cfg;
    cadv_rng rng;
    cadv_world world;
    cadv_nav nav;
    uint8_t cells[P10_W * P10_H];   /* 0 street, 1 building, 2 prop, 3 hall floor */
    uint8_t height[P10_W * P10_H];  /* building heights in metres */
    cadv_player body;
    int hp;
    int act;
    int weapon;
    float cd;
    float spin;
    int gauss_ammo;
    cadv_overclock oc;
    cadv_ring ring;
    p10_agent ag[P10_AGENTS];
    p10_civ civ[P10_CIVS];
    p10_enemy e[P10_ENEMIES];
    cadv_facade fac[P10_FACADES];
    int facades;
    p10_car car;
    p10_shot shots[P10_SHOTS];
    p10_tracer tracers[P10_TRACERS];
    cadv_charm charm;
    bool beacon;
    int followers;
    int max_followers;
    float alert;                     /* 0..100: 40 suspicious, 80 alarm */
    int reinforce_t;
    bool relay_done[3];
    cadv_vec3 relay[3];
    int hacking;                     /* relay being hacked, -1 */
    cadv_hack hack;
    bool casing_open;
    cadv_vec3 star_pos;
    bool star_taken;
    int hold_t;                      /* plaza: ticks until the VTOL lands */
    bool vtol_down;
    cadv_vec3 pad;
    cadv_vec3 lamp[P10_LAMPS];
    cadv_vec3 lamp_col[P10_LAMPS];
    int lamps;
    bool agent_lost;
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    int win_timer;
    int lose_timer;
    int hurt_flash;
    char message[160];
    int message_timer;
    char radio[160];
    int radio_timer;
    char names[P10_AGENTS][32];
    char mark_text[3][64];
    bool bench;                      /* enemies and civilians stand still (tests) */
    /* bot bookkeeping */
    cadv_nav_cell bot_path[128];
    int bot_path_len, bot_path_pos, bot_goal_x, bot_goal_z;
    int bot_stuck;
    cadv_vec3 bot_last;
    int bot_wp;
    int bot_target, bot_target_hp, bot_since_hit, bot_push;
    bool gpu_ready;
    p10_gpu g;
} p10_state;

cadv_vec3 cadv_p10_cell(int cx, int cz);
cadv_vec3 cadv_p10_eye(const p10_state *s);
int cadv_p10_hostiles(const p10_state *s);
int cadv_p10_agents_alive(const p10_state *s);
bool cadv_p10_hostile(const p10_state *s, int i);

#endif
