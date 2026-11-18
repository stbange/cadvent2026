/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P02_INTERNAL_H
#define CADV_PORTALS_P02_INTERNAL_H

#include "core/rng.h"
#include "gfx/gl_util.h"
#include "kits/vehicle/flyer.h"
#include "kits/vehicle/terrain.h"
#include "portals/p02_aurora_carpet.h"

#define P02_DT (1.0f / 60.0f)
#define P02_N 129
#define P02_CELL 2.0f
#define P02_MAX_ORBS 128
#define P02_MAX_CREATURES 32
#define P02_MAX_BALLOONS 24
#define P02_MAX_PROJ 64
#define P02_MAX_CASTLE_LEVEL 4
#define P02_PLAYER_HP 100
#define P02_VAEL_HP 160

enum { P02_NONE = 0, P02_PLAYER = 1, P02_VAEL = 2, P02_WILD = 3 };
enum { P02_SPELL_FIREBALL, P02_SPELL_CLAIM, P02_SPELL_RAISE, P02_SPELL_LOWER, P02_SPELL_CASTLE, P02_SPELL_VOLCANO, P02_SPELLS };
enum { P02_ORB_FREE, P02_ORB_CARRIED, P02_ORB_GONE };
enum { P02_WOLF, P02_GRIFFIN, P02_CREATURE_TYPES };
enum { P02_PHASE_DUSK, P02_PHASE_EXPANSION, P02_PHASE_NIGHT, P02_PHASE_SIEGE };
enum { P02_PROJ_FIREBALL, P02_PROJ_CLAIM };

typedef struct p02_orb {
    cadv_vec3 pos;
    int value;
    int owner;          /* P02_NONE / PLAYER / VAEL */
    int state;
    int balloon;        /* carrier, -1 if none */
} p02_orb;

typedef struct p02_creature {
    bool active;
    int type;
    cadv_vec3 pos;
    cadv_vec3 home;
    float yaw;
    int hp;
    int mana;
    float cooldown;
    float wander;       /* seconds until a new wander target */
    cadv_vec3 target;
    int hurt;           /* ticks of pain flash */
} p02_creature;

typedef struct p02_balloon {
    bool active;
    int owner;
    cadv_vec3 pos;
    int orb;            /* target or carried orb, -1 = none */
    bool carrying;
} p02_balloon;

typedef struct p02_castle {
    bool exists;
    bool destroyed;
    cadv_vec3 pos;      /* base centre */
    int mana;           /* stored mana (also the owner's spell fuel) */
    int level;          /* 0 = no castle, 1..4 */
    int peak_level;
    float turret_cd;
    float balloon_cd;
    int hit_flash;
} p02_castle;

typedef struct p02_proj {
    bool active;
    int kind;
    int owner;
    cadv_vec3 pos, vel;
    float life;
} p02_proj;

typedef struct p02_wizard {
    cadv_flyer fly;
    int hp;
    bool alive;
    int respawn;        /* ticks */
    float cast_cd;
    int target_orb;
    int mode;           /* 0 harvest, 1 hunt, 2 raid, 3 retreat */
} p02_wizard;

typedef struct p02_gpu {
    cadv_gpu_mesh chunk[CADV_TERRAIN_MAX_CHUNKS];
    uint32_t chunk_seen[CADV_TERRAIN_MAX_CHUNKS];
    cadv_gpu_mesh water, aurora, orb, balloon, carpet, hand, star, scroll, plaque, cube;
    cadv_gpu_mesh castle[2][P02_MAX_CASTLE_LEVEL];
    unsigned tex_ground, tex_water, tex_carpet, tex_mark;
    int mark_w;
    unsigned tex_sprite[3], tex_normal[3]; /* wolf, griffin, Vael */
    int fw[3], fh[3];
} p02_gpu;

typedef struct p02_state {
    cadv_portal_cfg cfg;
    cadv_rng rng;
    cadv_terrain terrain;
    cadv_flyer_def fdef;
    cadv_flyer player;
    int hp;
    int deaths;
    int spell;
    bool has_volcano;
    float cast_cd;
    float hurt_cd;
    int hurt_flash;
    p02_orb orbs[P02_MAX_ORBS];
    int norbs;
    p02_creature creatures[P02_MAX_CREATURES];
    p02_balloon balloons[P02_MAX_BALLOONS];
    p02_proj proj[P02_MAX_PROJ];
    p02_castle castle[3];       /* index by owner: [1] player, [2] Vael */
    p02_wizard vael;
    int wild_pool;              /* mana waiting to be reborn as creatures */
    float spawn_cd;
    int phase;
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    bool star_active;
    bool star_taken;
    cadv_vec3 star_pos;
    bool scroll_active;
    cadv_vec3 scroll_pos;
    int volcano_ticks;          /* eruption in progress */
    cadv_vec3 volcano_pos;
    int win_timer;
    int lose_timer;
    bool castle_kept;           /* castle reached level 2 and never fell below */
    bool reached_level2;
    int shake;
    cadv_vec3 home_spot;        /* the plateau suggested for the first castle */
    cadv_vec3 rune_centre;
    cadv_vec3 rock_pos;
    char message[72];
    int message_timer;
    char quip[80];
    int quip_timer;
    char mark_text[4][64];
    cadv_vec3 flash_pos[16];    /* short-lived explosion lights (visual only) */
    int flash_ttl[16];
    int last_events;            /* bit set of P02_EV_* this tick */
    bool gpu_ready;
    p02_gpu g;
} p02_state;

enum {
    P02_EV_CAST = 1 << 0,
    P02_EV_HIT = 1 << 1,
    P02_EV_KILL = 1 << 2,
    P02_EV_CLAIM = 1 << 3,
    P02_EV_BANK = 1 << 4,
    P02_EV_CASTLE = 1 << 5,
    P02_EV_HURT = 1 << 6,
    P02_EV_STAR = 1 << 7,
    P02_EV_DEFORM = 1 << 8,
    P02_EV_ERUPT = 1 << 9,
    P02_EV_DENIED = 1 << 10
};

/* Exposed for unit tests. Mana counts for an owner once it is banked in the
 * castle or rides in one of its balloons; claimed orbs still need fetching. */
int cadv_p02_owned_mana(const p02_state *s, int owner);
int cadv_p02_total_mana(const p02_state *s);
float cadv_p02_share(const p02_state *s, int owner);
int cadv_p02_castle_level_for(int mana);
int cadv_p02_spell_cost(int spell);
const char *cadv_p02_spell_name(int spell);
/* Releases 'amount' mana from a castle as free orbs around it. */
int cadv_p02_damage_castle(p02_state *s, int owner, int amount);
bool cadv_p02_cast(p02_state *s, int caster, int spell, cadv_vec3 from, cadv_vec3 dir);
int cadv_p02_add_orb(p02_state *s, cadv_vec3 pos, int value, int owner);
int cadv_p02_spawn_creature(p02_state *s, int type, cadv_vec3 pos, int mana);
bool cadv_p02_place_castle(p02_state *s, int owner, cadv_vec3 at);

#endif
