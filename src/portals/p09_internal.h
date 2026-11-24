/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P09_INTERNAL_H
#define CADV_PORTALS_P09_INTERNAL_H

#include "core/rng.h"
#include "gfx/gl_util.h"
#include "kits/fps/ctf.h"
#include "kits/vehicle/skier.h"
#include "kits/vehicle/terrain.h"
#include "portals/p09_skifrost.h"

#define P09_DT (1.0f / 60.0f)
#define P09_N 129
#define P09_CELL 4.0f            /* 512 m across */
#define P09_AGENTS 8
#define P09_TEAM 4
#define P09_TO_WIN 5
#define P09_MATCH (20 * 60 * 60)
#define P09_RESPAWN (6 * 60)
#define P09_COUNT (4 * 60)
#define P09_TRAIN_MAX (150 * 60)
#define P09_PROJ 48
#define P09_FEED 5
#define P09_ROUTE 18
#define P09_FAST 41.67f          /* 150 km/h in m/s */
#define P09_DISC_SPEED 65.0f
#define P09_LAP_CP 40

enum { P09_DISC, P09_CHAIN, P09_GRENADE, P09_WEAPONS };
enum { P09_PH_TRAIN, P09_PH_COUNT, P09_PH_MATCH, P09_PH_LAP };
enum { P09_TR_SKI, P09_TR_JET, P09_TR_DISC, P09_TR_DONE };
enum { P09_ROLE_CAP, P09_ROLE_HOFF, P09_ROLE_D, P09_ROLE_MID };
enum { P09_PJ_DISC, P09_PJ_GRENADE, P09_PJ_TURRET };

typedef struct p09_agent {
    cadv_skier k;
    bool alive;
    int team;
    int name;           /* friend index, or 10+ for tribesfolk */
    int weapon;
    float cd;           /* seconds until the weapon is ready */
    float spin;         /* chaingun spin-up */
    int respawn;        /* ticks */
    float invuln;
    int kills, deaths, caps;
    float carry_max;    /* top speed while carrying the flag */
    int last_hit_by;
    float calm;         /* seconds since last hurt */
    /* bots */
    int role;
    int route;          /* which route, and the next waypoint on it */
    int route_pos;
    int target;
    float react;
    float strafe;
    bool jet_ok;
    bool retreat;       /* falling back to patch up */
    float siege;        /* seconds spent near the enemy flag */
    int stuck;
    cadv_vec3 last_pos;
    float aim_err_x, aim_err_y;
    bool met;
} p09_agent;

typedef struct p09_proj {
    bool active;
    int kind;
    int owner;          /* agent, or -1 - team for turrets */
    int team;
    cadv_vec3 pos, vel;
    float fuse;
} p09_proj;

typedef struct p09_base {
    cadv_vec3 stand;    /* the flag's home, top of the stand */
    cadv_vec3 gen;      /* generator centre */
    cadv_vec3 station;
    cadv_vec3 spawn[P09_TEAM];
    float face;         /* yaw looking at the enemy base */
    cadv_generator power;
    cadv_turret turret;
    int turret_target;
} p09_base;

typedef struct p09_gpu {
    cadv_gpu_mesh chunk[CADV_TERRAIN_MAX_CHUNKS];
    uint32_t chunk_seen[CADV_TERRAIN_MAX_CHUNKS];
    cadv_gpu_mesh cube, aurora, star, flag, pole, armour[CADV_ARMOURS], head, pack, base, gen, turret, station, plaque, ring;
    unsigned tex_snow, tex_rock, tex_mark;
    int mark_w;
} p09_gpu;

typedef struct p09_state {
    cadv_portal_cfg cfg;
    cadv_rng rng;
    cadv_terrain terrain;
    cadv_world world;
    p09_agent a[P09_AGENTS];
    p09_base base[2];
    cadv_flag flag[2];       /* by team: flag[t] belongs to team t */
    int drop_agent[2];
    int drop_lock[2];        /* ticks before the dropper may take it again */
    p09_proj proj[P09_PROJ];
    cadv_vec3 route[2][P09_ROUTE]; /* two ski lines from base 0 to base 1 */
    int score[2];
    int phase;
    int timer;               /* match ticks left, or countdown */
    bool sudden;
    int train;
    int train_t;
    cadv_vec3 beacon;
    cadv_vec3 drone_c;
    float drone_a;
    cadv_vec3 drone;
    cadv_vec3 star_pos;
    bool star_carried;
    bool star_taken;
    bool fast_cap;
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    int end_timer;
    bool won;
    char message[96];
    int message_timer;
    char feed[P09_FEED][96];
    int feed_t[P09_FEED];
    char names[P09_AGENTS][32];
    char mark_text[4][64];
    int hurt_flash;
    float dmg_taken[P09_AGENTS + 2]; /* by source, the player's (balance) */
    int hit_mark;
    int boom_t;
    cadv_vec3 boom_pos;
    bool map_held;
    bool bench;              /* bots stand still (tests) */
    bool gpu_ready;
    p09_gpu g;
} p09_state;

float cadv_p09_skill(const p09_state *s, int i);
bool cadv_p09_in_halfpipe(cadv_vec3 p);
cadv_vec3 cadv_p09_centre(const p09_agent *a);

#endif
