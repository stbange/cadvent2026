/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P07_INTERNAL_H
#define CADV_PORTALS_P07_INTERNAL_H

#include "core/rng.h"
#include "gfx/gl_util.h"
#include "kits/fps/player.h"
#include "kits/fps/tactics.h"
#include "portals/p07_tinsel_strike.h"

#define P07_DT (1.0f / 60.0f)
#define P07_W 40          /* cells */
#define P07_H 32
#define P07_CELL 2.0f     /* metres */
#define P07_AGENTS 10
#define P07_TEAM 5
#define P07_HOUSE_H 7.0f
#define P07_CRATE_H 1.15f
#define P07_FREEZE (15 * 60)
#define P07_ROUND (105 * 60)
#define P07_BOMB (40 * 60)
#define P07_PLANT (3 * 60)
#define P07_DEFUSE (10 * 60)
#define P07_DEFUSE_KIT (5 * 60)
#define P07_AFTER (5 * 60)
#define P07_TO_WIN 5
#define P07_HALF 4        /* rounds before the sides switch */
#define P07_SMOKES 6
#define P07_NADES 6
#define P07_PATH 96
#define P07_STEPS 12      /* celebration platforms up the tree */
#define P07_FEED 4

/* weapons: knife, two pistols, smg, shotgun, rifle, sniper; then equipment */
enum { P07_KNIFE, P07_PISTOL, P07_DEAGLE, P07_SMG, P07_SHOTGUN, P07_RIFLE, P07_SNIPER, P07_WEAPONS };
enum { P07_BUY_DEAGLE, P07_BUY_SMG, P07_BUY_SHOTGUN, P07_BUY_RIFLE, P07_BUY_SNIPER, P07_BUY_ARMOUR, P07_BUY_KIT, P07_BUY_FLASH,
       P07_BUY_SMOKE, P07_BUY_ITEMS };
enum { P07_SLOT_KNIFE, P07_SLOT_PISTOL, P07_SLOT_PRIMARY, P07_SLOT_FLASH, P07_SLOT_SMOKE };
enum { P07_HUMBUG = 0, P07_GUARD = 1 };       /* sides: attackers, defenders */
enum { P07_PH_FREEZE, P07_PH_LIVE, P07_PH_AFTER, P07_PH_CELEBRATE, P07_PH_OVER };
enum { P07_ROLE_ENTRY, P07_ROLE_SUPPORT, P07_ROLE_LURK, P07_ROLE_ANCHOR_A, P07_ROLE_ANCHOR_B, P07_ROLE_MID };

typedef struct p07_weapon_def {
    const char *name;
    int price;
    int damage;
    int pellets;
    float rate;       /* seconds between shots */
    bool automatic;
    int mag, reserve;
    float range;
    float spread;     /* standing still */
    float move_spread;
    float head;       /* headshot multiplier */
    float speed;      /* move speed factor */
    cadv_recoil recoil;
} p07_weapon_def;

typedef struct p07_agent {
    cadv_player body;
    bool alive;
    int team;          /* 0 = the player's team, 1 = the other */
    int name;          /* roster index */
    int money;
    int primary;       /* weapon id or -1 */
    int pistol;
    int slot;          /* held slot */
    int mag[3], reserve[3];  /* by slot: knife (unused), pistol, primary */
    bool kit;
    int flashes, smokes;
    float shot_cd;
    int spray;
    float since_shot;
    float blind;       /* seconds of flash left */
    bool scoped;
    int kills, deaths, round_kills;
    bool knife_kill;
    float plant_t, defuse_t;
    /* bots */
    int role;
    cadv_nav_cell path[P07_PATH];
    int path_len, path_pos;
    cadv_nav_cell goal;
    int target;
    float react;       /* seconds before a new target is engaged */
    float burst;
    float strafe;
    float hold;        /* seconds to hold the current spot */
    cadv_vec3 heard;
    float heard_t;
    float look_yaw;    /* where to watch when nothing is seen */
    int scan;
    int stuck;
    cadv_vec3 last_pos;
    bool met;          /* the player has seen this one (friends' marks) */
} p07_agent;

typedef struct p07_smoke {
    cadv_vec3 pos;
    int ticks;         /* left */
} p07_smoke;

typedef struct p07_nade {
    bool active;
    bool smoke;
    int owner;
    cadv_vec3 pos, vel;
    float fuse;
} p07_nade;

typedef struct p07_tracer {
    cadv_vec3 a, b;
    int ttl;
} p07_tracer;

typedef struct p07_gpu {
    cadv_gpu_mesh ground, houses, roofs, crates, tree, trunk, cube, plaque, star, steps, bomb, lights, sites;
    unsigned tex_cobble, tex_timber, tex_mark, tex_wood;
    int mark_w;
    int steps_built;
} p07_gpu;

typedef struct p07_state {
    cadv_portal_cfg cfg;
    cadv_rng rng;
    cadv_world world;
    cadv_nav nav;
    uint8_t cells[P07_W * P07_H];      /* 0 street, 1 house, 2 crate, 3 tree */
    p07_agent a[P07_AGENTS];
    cadv_econ_rules econ;
    int round;                         /* 1-based */
    int phase;
    int timer;                         /* ticks left in the phase */
    int score[2];                      /* by team */
    int half_won[2][2];                /* [half][team] */
    int loss_streak[2];
    int player_side;                   /* the player's team's side this half */
    int site_choice;                   /* 0 = A, 1 = B for the attackers' plan */
    int alert_site;                    /* a site where defenders saw attackers, -1 */
    int carrier;                       /* agent with the bomb, -1 none */
    bool bomb_dropped;
    cadv_vec3 bomb_pos;
    bool planted;
    int bomb_timer;
    int bomb_site;
    int winner;                        /* team that won the last round, -1 */
    int win_reason;
    bool clutch_chance;
    bool clutch;
    bool ace;
    int spectate;
    bool buy_open;
    bool map_held;                     /* the scoreboard while TAB is held */
    int buy_cursor;
    p07_smoke smokes[P07_SMOKES];
    p07_nade nades[P07_NADES];
    p07_tracer tracers[16];
    char feed[P07_FEED][48];
    int feed_t[P07_FEED];
    bool steps;                        /* the celebration route up the tree */
    int climb;                         /* the bot's next step */
    cadv_vec3 step_pos[P07_STEPS];
    cadv_vec3 star_pos;
    bool star_taken;
    bool spray_seen;
    int confetti_t;
    cadv_vec3 confetti_pos;
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    int end_timer;
    char message[72];
    int message_timer;
    char radio[96];
    int radio_timer;
    char names[P07_AGENTS][24];
    char mark_text[6][64];
    int hurt_flash;
    int side_wins[2];                  /* rounds won by side (balance) */
    bool even;                         /* every bot, the player's too, at the same skill */
    bool bench;                        /* bots stand still (tests) */
    bool gpu_ready;
    p07_gpu g;
} p07_state;

const p07_weapon_def *cadv_p07_weapons(void);
const char *cadv_p07_buy_name(int item);
int cadv_p07_buy_price(int item);
int cadv_p07_side_of(const p07_state *s, int team);
cadv_vec3 cadv_p07_eye(const p07_state *s, int agent);
int cadv_p07_alive(const p07_state *s, int team);
const char *cadv_p07_area(const p07_state *s, cadv_vec3 p);
bool cadv_p07_in_site(const p07_state *s, cadv_vec3 p, int *site);
bool cadv_p07_can_see(const p07_state *s, int from, int to);

#endif
