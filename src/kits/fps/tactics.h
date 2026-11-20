/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_FPS_TACTICS_H
#define CADV_KITS_FPS_TACTICS_H

#include <stdbool.h>
#include <stdint.h>

#include "core/mathx.h"

/* Round-based tactical shooter pieces: learnable recoil patterns, movement
 * inaccuracy, the economy rules, smoke that blocks sight, and a walk grid
 * with cover cells and A* paths for bots. */

/* ---- recoil ------------------------------------------------------------------------ */

typedef struct cadv_recoil {
    float climb;   /* pitch kick per shot for the first shots, radians */
    float sway;    /* sideways drift amplitude once the climb tops out, radians */
    int top;       /* shots until the climb tops out */
} cadv_recoil;

/* Cumulative aim offset after 'shot' rounds of a spray (shot 0 = first round,
 * which goes where you aim). The same shot index always gives the same
 * offset, so the pattern can be learnt and pulled against. */
void cadv_recoil_offset(const cadv_recoil *r, int shot, float *dyaw, float *dpitch);

/* Cone half-angle of inaccuracy: a base plus penalties for moving (by the
 * fraction of full speed), for being in the air, halved when crouched. */
float cadv_spread(float base, float move_penalty, float speed, float max_speed, bool airborne, bool crouched);

/* ---- economy -------------------------------------------------------------------------- */

typedef enum cadv_win_reason {
    CADV_WIN_ELIMINATION = 0,
    CADV_WIN_BOMB,     /* the bomb went off */
    CADV_WIN_DEFUSE,
    CADV_WIN_TIME
} cadv_win_reason;

typedef struct cadv_econ_rules {
    int start, max;
    int kill, knife_kill;
    int win_elimination, win_bomb, win_defuse, win_time;
    int loss_base, loss_step, loss_max;
    int plant_bonus;   /* to the attackers when they planted, even if they lose */
} cadv_econ_rules;

cadv_econ_rules cadv_econ_default(void);
/* Money a player receives at the end of a round. 'loss_streak' counts the
 * team's consecutive losses including this one (1 = first loss). */
int cadv_econ_round_reward(const cadv_econ_rules *r, bool won, cadv_win_reason why, int loss_streak, bool attacker_planted,
                           bool is_attacker);
int cadv_econ_add(const cadv_econ_rules *r, int money, int amount); /* clamped to [0, max] */

/* ---- smoke ------------------------------------------------------------------------------ */

/* True when the segment a-b passes through the sphere (centre c, radius r). */
bool cadv_segment_sphere(cadv_vec3 a, cadv_vec3 b, cadv_vec3 c, float r);

/* ---- walk grid ---------------------------------------------------------------------------- */

#define CADV_NAV_MAX 64

typedef struct cadv_nav_cell {
    int8_t x, z;
} cadv_nav_cell;

typedef struct cadv_nav {
    int w, h;
    float cell;            /* metres per cell; world x = cx * cell */
    uint8_t walk[CADV_NAV_MAX * CADV_NAV_MAX];
    uint8_t cover[CADV_NAV_MAX * CADV_NAV_MAX]; /* walkable and next to something to hide behind */
} cadv_nav;

bool cadv_nav_init(cadv_nav *n, int w, int h, float cell);
void cadv_nav_set(cadv_nav *n, int x, int z, bool walkable);
bool cadv_nav_walkable(const cadv_nav *n, int x, int z);
void cadv_nav_mark_cover(cadv_nav *n);
cadv_nav_cell cadv_nav_cell_at(const cadv_nav *n, float x, float z);
cadv_vec3 cadv_nav_centre(const cadv_nav *n, cadv_nav_cell c);
/* 8-way A* without corner cutting; writes the cells after 'a' up to 'b' and
 * returns their count (0 when a == b, -1 when unreachable or longer than max). */
int cadv_nav_path(const cadv_nav *n, cadv_nav_cell a, cadv_nav_cell b, cadv_nav_cell *out, int max);
/* The nearest cover cell to (x, z) within 'radius' cells, or false. */
bool cadv_nav_nearest_cover(const cadv_nav *n, cadv_nav_cell from, int radius, cadv_nav_cell *out);

#endif
