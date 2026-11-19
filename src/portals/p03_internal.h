/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P03_INTERNAL_H
#define CADV_PORTALS_P03_INTERNAL_H

#include "gfx/gl_util.h"
#include "kits/rts/rts.h"
#include "kits/rts/rts_ai.h"
#include "portals/p03_frostfront.h"

#define P03_DT (1.0f / 60.0f)
#define P03_W 64
#define P03_H 64
#define P03_CELL 3.0f
#define P03_EYE 1.7f
#define P03_ACCORD 0
#define P03_ORDER 1

enum { P03_B_YARD, P03_B_POWER, P03_B_REFINERY, P03_B_BARRACKS, P03_B_FACTORY, P03_B_TOWER,
       P03_B_SPIRE, P03_B_PYLON, P03_B_LAB, P03_BUILDINGS };
enum { P03_U_RIFLE, P03_U_ROCKET, P03_U_ENGINEER, P03_U_HARVESTER, P03_U_TANK, P03_U_BUGGY,
       P03_U_COMMANDER, P03_UNITS };
enum { P03_T_SNOW, P03_T_WATER, P03_T_ROCK, P03_T_BRIDGE };
/* Wrist-tablet items: five structures, then six units. */
#define P03_TAB_STRUCTURES 5
#define P03_TAB_ITEMS 11

typedef struct p03_gpu {
    cadv_gpu_mesh ground, water, rocks, bridge, crystal, cube, ring, plaque, star;
    cadv_gpu_mesh building[P03_BUILDINGS];
    unsigned tex_snow, tex_water, tex_rock, tex_mark[3];
    int mark_w[3];
    unsigned tex_sprite[P03_UNITS], tex_normal[P03_UNITS];
    int fw[P03_UNITS], fh[P03_UNITS];
} p03_gpu;

typedef struct p03_state {
    cadv_portal_cfg cfg;
    cadv_rts rts;
    cadv_rts_ai ai;
    uint8_t terrain[P03_W * P03_H];
    int commander;              /* kit unit index, -1 while down */
    float yaw, pitch;
    int respawn;
    float sidearm_cd;
    uint8_t selected[CADV_RTS_MAX_UNITS];
    int tab_item;
    bool placing;
    bool tablet_open;           /* large minimap */
    int drone;                  /* ticks of recon view left */
    int drone_cd;
    int spire;                  /* building index */
    int lab;
    int star_state;             /* 0 sealed, 1 in the vault, 2 carried, 3 delivered */
    float star_x, star_z;
    int star_carrier;
    bool harvester_lost;
    uint8_t was_alive[CADV_RTS_MAX_UNITS]; /* to notice losses between ticks */
    bool lab_captured;
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
    bool gpu_ready;
    p03_gpu g;
} p03_state;

/* Exposed for unit tests and the renderer. */
const cadv_rts_bdef *cadv_p03_buildings(void);
const cadv_rts_udef *cadv_p03_units(void);
const char *cadv_p03_tab_name(int item);
int cadv_p03_tab_cost(int item);
/* Ground point under the crosshair (false when looking at the sky / too far). */
bool cadv_p03_aim_ground(const p03_state *s, float *x, float *z);
/* What the crosshair is on: own unit, enemy unit or building target (kit
 * target encoding), or -1. */
int cadv_p03_aim_target(const p03_state *s, bool own);
cadv_vec3 cadv_p03_eye(const p03_state *s);
cadv_vec3 cadv_p03_forward(const p03_state *s);

#endif
