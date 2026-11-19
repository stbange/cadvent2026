/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P04_INTERNAL_H
#define CADV_PORTALS_P04_INTERNAL_H

#include "gfx/gl_util.h"
#include "kits/platformer/climber.h"
#include "kits/platformer/tiles.h"
#include "portals/p04_ice_tomb.h"

#define P04_DT (1.0f / 60.0f)
#define P04_W 40
#define P04_H 16
#define P04_D 100
#define P04_MAX_ENEMIES 12
#define P04_MAX_PICKUPS 16
#define P04_HP 100

enum { P04_WOLF, P04_BAT, P04_BEAR, P04_COLOSSUS, P04_ENEMY_TYPES };
enum { P04_PK_GEAR, P04_PK_JADE, P04_PK_HEALTH };
enum { P04_LEVER_HALL, P04_LEVER_POOL, P04_LEVER_VENT, P04_LEVERS };
enum { P04_PLAQUE_TALLY, P04_PLAQUE_BLADE, P04_PLAQUE_JOURNAL, P04_PLAQUES };
#define P04_CRYSTALS 3

/* bot route steps */
enum { P04_STEP_WALK, P04_STEP_JUMP, P04_STEP_CLIMB, P04_STEP_PUSH, P04_STEP_USE, P04_STEP_SWIM, P04_STEP_OUT, P04_STEP_WAIT };
enum { P04_DONE_NEAR, P04_DONE_FLAG };

typedef struct p04_step {
    int kind;
    float x, y, z;     /* where to stand / swim to */
    float yaw;         /* facing for jumps, climbs, pushes and levers */
    int run;
    int done;          /* completion test */
    int flag;          /* for P04_DONE_FLAG: index into p04_state flags */
} p04_step;

typedef struct p04_enemy {
    bool active;
    int type;
    cadv_vec3 pos, home;
    float yaw;
    int hp;
    float cooldown;
    int hurt;
} p04_enemy;

typedef struct p04_pickup {
    bool active;
    int kind;
    int value;
    cadv_vec3 pos;
} p04_pickup;

typedef struct p04_gpu {
    cadv_gpu_mesh tiles[CADV_TILE_KINDS];
    uint32_t tiles_version;
    cadv_gpu_mesh cube, gear, jade, crystal, lever, blade, plaque, star, door, hand, colossus;
    unsigned tex_rock, tex_ice, tex_water, tex_block, tex_door;
    unsigned tex_plaque[P04_PLAQUES];
    int plaque_w[P04_PLAQUES];
    unsigned tex_sprite[P04_ENEMY_TYPES], tex_normal[P04_ENEMY_TYPES];
    int fw[P04_ENEMY_TYPES], fh[P04_ENEMY_TYPES];
} p04_gpu;

typedef struct p04_state {
    cadv_portal_cfg cfg;
    cadv_tiles tiles;
    cadv_climber_def cdef;
    cadv_climber me;
    int hp;
    float gun_cd;
    int gun_side;              /* alternating pistols */
    int muzzle;                /* ticks of muzzle flash */
    int hurt_flash;
    p04_enemy enemies[P04_MAX_ENEMIES];
    p04_pickup pickups[P04_MAX_PICKUPS];
    int npickups;
    bool lever[P04_LEVERS];
    int door_timer;            /* timed door open ticks left */
    bool door_jammed;          /* stays open once gear 1 is out */
    int vent_timer;            /* ticks until the ice wall melts */
    bool ice_melted;
    int gears;
    uint32_t gear_bits;
    int jade;
    uint32_t jade_bits;
    int crystal_uses;
    int crystal;               /* last crystal used, -1 none */
    bool crystal_used[P04_CRYSTALS];
    cadv_vec3 respawn;
    float respawn_yaw;
    bool chase;                /* the Colossus is awake */
    float collapse_z;          /* floor rows north of this are gone */
    int deaths;
    bool gears_set;            /* placed in the solar door */
    bool star_active;
    bool star_taken;
    cadv_vec3 star_pos;
    int win_timer;
    int route;
    int route_base;            /* where the route starts (checkpoint resumes) */
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    char message[72];
    int message_timer;
    char quip[80];
    int quip_timer;
    char mark_text[4][64];
    bool gpu_ready;
    p04_gpu g;
} p04_state;

/* Exposed for unit tests and the renderer. */
cadv_vec3 cadv_p04_lever_pos(int lever);
cadv_vec3 cadv_p04_crystal_pos(int crystal);
cadv_vec3 cadv_p04_plaque_pos(int plaque, float *yaw);
cadv_vec3 cadv_p04_blade_pos(const p04_state *s);
cadv_vec3 cadv_p04_solar_door(void);
const p04_step *cadv_p04_route(int *count);
bool cadv_p04_flag(const p04_state *s, int flag);

#endif
