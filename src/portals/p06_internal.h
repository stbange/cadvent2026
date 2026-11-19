/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P06_INTERNAL_H
#define CADV_PORTALS_P06_INTERNAL_H

#include "core/rng.h"
#include "gfx/gl_util.h"
#include "kits/vehicle/terrain.h"
#include "kits/vehicle/walker.h"
#include "portals/p06_iron_yule.h"

#define P06_DT (1.0f / 60.0f)
#define P06_N 129
#define P06_CELL 4.0f
#define P06_SEA 4.0f
#define P06_MAX_UNITS 40
#define P06_MAX_PROJ 96
#define P06_MAX_BEAMS 24
#define P06_SLOTS 4
#define P06_TONNAGE 10
#define P06_PATH_NODES 9
#define P06_VENTS 5

enum { P06_LAB, P06_M1, P06_M2, P06_M3, P06_DONE };
enum { P06_W_LASER, P06_W_PPC, P06_W_AC, P06_W_MISSILE, P06_WEAPONS };
enum { P06_G_ENERGY, P06_G_BALLISTIC, P06_G_MISSILE, P06_GROUPS };
enum { P06_K_JACKAL, P06_K_HOVER, P06_K_TURRET, P06_K_PUMP, P06_K_KRAMPUS, P06_K_WINGMAN, P06_K_CRAWLER, P06_K_WRECK, P06_KINDS };
enum { P06_TEAM_US = 0, P06_TEAM_GRAIL = 1 };
enum { P06_PROJ_SHELL, P06_PROJ_MISSILE, P06_PROJ_CANISTER };
/* who a projectile or beam belongs to / targets: units by index, the player as -2 */
#define P06_PLAYER (-2)

typedef struct p06_weapon_def {
    const char *name;
    int group;
    float heat;
    int damage;        /* per shot (per missile for missiles) */
    int count;         /* projectiles per shot */
    float cooldown;    /* seconds */
    float range;
    float speed;       /* 0 = beam */
    int ammo;          /* 0 = unlimited */
    int tons;
} p06_weapon_def;

typedef struct p06_unit {
    bool active;
    int kind;
    int team;
    cadv_walker w;     /* walkers: jackal, Krampus, wingmen */
    cadv_vec3 pos;     /* others (walkers mirror w.pos here) */
    float yaw;
    int hp, hp_max;    /* non-walkers */
    float cd[3];
    int target;        /* unit index, P06_PLAYER, or -1 */
    float ai_t;        /* seconds in the current manoeuvre */
    int ai_mode;
    float strafe;      /* -1 / 1 circling direction */
    int hurt;          /* ticks of damage flash */
    bool dead;         /* wreck left on the field */
    int group;         /* ambush group / mission tag */
} p06_unit;

typedef struct p06_proj {
    bool active;
    int kind;
    int team;
    int owner;         /* unit index or P06_PLAYER */
    int target;        /* homing target or -1 */
    cadv_vec3 pos, vel;
    int damage;
    float life;
} p06_proj;

typedef struct p06_beam {
    cadv_vec3 a, b;
    int kind;          /* weapon */
    int ttl;
} p06_beam;

typedef struct p06_gpu {
    cadv_gpu_mesh chunk[CADV_TERRAIN_MAX_CHUNKS];
    uint32_t chunk_seen[CADV_TERRAIN_MAX_CHUNKS];
    cadv_gpu_mesh water, cube, plaque, star, crawler, pump, turret, hover, canister, vent, cockpit, wreck;
    cadv_gpu_mesh walker_part[6];   /* hip, torso, arm, leg, head, jet */
    unsigned tex_snow, tex_water, tex_mark, tex_crawler;
    int mark_w;
} p06_gpu;

typedef struct p06_state {
    cadv_portal_cfg cfg;
    cadv_rng rng;
    cadv_terrain terrain;
    cadv_walker mech;
    int loadout[P06_SLOTS];        /* weapon per hardpoint: LA, RA, LT, RT */
    int ammo[P06_SLOTS];
    float cd[P06_SLOTS];
    int lab_slot;
    int paint;                     /* 0 white, 1 sleighbell red, 2 the friend's black */
    int group;                     /* selected weapon group */
    bool chain;                    /* chain fire: one weapon of the group per trigger */
    int chain_next;
    int lock;                      /* locked unit or -1 */
    float lock_t;
    int mission;
    int intro;                     /* ticks of the mission title card */
    p06_unit u[P06_MAX_UNITS];
    p06_proj proj[P06_MAX_PROJ];
    p06_beam beam[P06_MAX_BEAMS];
    int crawler;                   /* unit index */
    cadv_vec3 path[P06_PATH_NODES];
    int path_next;                 /* next node the crawler drives to */
    int crawler_cp;                /* path node of the last escort checkpoint */
    int ambush_sent;               /* ambush groups triggered so far */
    int wingmen[2];
    int krampus;
    int pumps_left;
    bool reinforced;
    cadv_vec3 vents[P06_VENTS];
    cadv_vec3 pump_station, crater, depot;
    int coolant_left;
    int coolant_timer;             /* ticks until the canister lands, 0 none */
    cadv_vec3 coolant_pos;
    bool coolant_down;             /* landed, waiting to be picked up */
    int cool_boost;                /* ticks of extra cooling */
    bool star_active, star_taken;
    cadv_vec3 star_pos;
    int restarts;                  /* times the convoy had to try again */
    bool m3_component_lost;
    int lost_at_m3;
    cadv_vec3 wreck_pos;
    bool map_open;
    bool was_shutdown;
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    int win_timer;
    int lose_timer;
    int hurt_flash;
    int shake;
    char message[72];
    int message_timer;
    char radio[120];
    int radio_timer;
    char mark_text[4][64];
    bool gpu_ready;
    p06_gpu g;
} p06_state;

const p06_weapon_def *cadv_p06_weapons(void);
const cadv_walker_def *cadv_p06_walker_def(int kind); /* P06_K_JACKAL, _KRAMPUS, _WINGMAN (= the player's) */
int cadv_p06_tonnage(const p06_state *s);
cadv_vec3 cadv_p06_eye(const p06_state *s);
cadv_vec3 cadv_p06_unit_pos(const p06_state *s, int i);
int cadv_p06_enemies_alive(const p06_state *s);
int cadv_p06_aim_target(const p06_state *s);
const char *cadv_p06_paint_name(const p06_state *s, int paint);

#endif
