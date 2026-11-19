/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P05_INTERNAL_H
#define CADV_PORTALS_P05_INTERNAL_H

#include "gfx/gl_util.h"
#include "kits/rts/rts.h"
#include "kits/rts/rts_ai.h"
#include "portals/p05_hearthguard.h"

#define P05_DT (1.0f / 60.0f)
#define P05_W 64
#define P05_H 64
#define P05_CELL 3.0f
#define P05_EYE 1.5f
#define P05_GNOMES 0
#define P05_TROLLS 1
#define P05_LAKE_Z0 24
#define P05_LAKE_Z1 33

/* building flags beyond the kit's own */
#define P05_BF_SHIPYARD (1u << 6)
#define P05_BF_WORKSHOP (1u << 7)

enum { P05_B_HALL, P05_B_FARM, P05_B_BARRACKS, P05_B_MILL, P05_B_TOWER, P05_B_SHIPYARD, P05_B_WORKSHOP,
       P05_B_KEEP, P05_B_LONGHALL, P05_B_HUT, P05_B_TOTEM, P05_B_TAVERN, P05_BUILDINGS };
enum { P05_U_WORKER, P05_U_FOOTMAN, P05_U_ARCHER, P05_U_KNIGHT, P05_U_BALLISTA, P05_U_FERRY, P05_U_CAPTAIN,
       P05_U_AXE, P05_U_THROWER, P05_U_WARLORD, P05_UNITS };
enum { P05_T_SNOW, P05_T_LAKE, P05_T_ROCK };
/* command stave: seven items for structures, six for units, the keep upgrade */
#define P05_STAVE_STRUCTURES 6
#define P05_STAVE_UNITS 6
#define P05_STAVE_ITEMS (P05_STAVE_STRUCTURES + P05_STAVE_UNITS + 1)
#define P05_STAVE_KEEP (P05_STAVE_ITEMS - 1)

typedef struct p05_gpu {
    cadv_gpu_mesh ground, lake, rocks, tree, mine, platform, cube, ring, plaque, star, fire;
    uint32_t grid_version;
    cadv_gpu_mesh building[P05_BUILDINGS];
    unsigned tex_snow, tex_ice, tex_water, tex_rock, tex_sign;
    int sign_w;
    unsigned tex_sprite[P05_UNITS], tex_normal[P05_UNITS], tex_owl;
    int fw[P05_UNITS], fh[P05_UNITS], owl_w, owl_h;
} p05_gpu;

typedef struct p05_state {
    cadv_portal_cfg cfg;
    cadv_rts rts;
    cadv_rts_ai ai;
    uint8_t terrain[P05_W * P05_H];
    int captain;                 /* kit unit index, -1 while down */
    float yaw, pitch;
    int respawn;
    float bash_cd;
    uint8_t selected[CADV_RTS_MAX_UNITS];
    int stave_item;
    bool placing;
    bool map_open;
    int keep_timer;              /* ticks left of the keep upgrade, 0 none */
    bool keep;
    int longhall;
    int tavern;
    int hall;
    int raid;                    /* raids sent so far */
    int raid_timer;              /* ticks until the next raid */
    uint8_t raider[CADV_RTS_MAX_UNITS];
    bool thawed;
    int star_state;              /* 0 none, 1 in the fire pit, 2 carried by the Captain, 3 home */
    float star_x, star_z;
    bool building_lost_in_raids;
    bool hero_fell;
    int worker_clicks;
    int last_pick;
    float owl_t;
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    int win_timer;
    int lose_timer;
    char message[72];
    int message_timer;
    char quip[80];
    int quip_timer;
    char mark_text[4][64];
    uint8_t was_alive[CADV_RTS_MAX_UNITS];
    bool gpu_ready;
    p05_gpu g;
} p05_state;

const cadv_rts_bdef *cadv_p05_buildings(void);
const cadv_rts_udef *cadv_p05_units(void);
const char *cadv_p05_stave_name(int item);
void cadv_p05_stave_cost(int item, int *gold, int *lumber, int *tallow);
bool cadv_p05_aim_ground(const p05_state *s, float *x, float *z);
int cadv_p05_aim_target(const p05_state *s, bool own);
cadv_vec3 cadv_p05_eye(const p05_state *s);
cadv_vec3 cadv_p05_owl(const p05_state *s);
int cadv_p05_raid_ticks(const p05_state *s, int raid);
int cadv_p05_raiders_alive(const p05_state *s);

#endif
