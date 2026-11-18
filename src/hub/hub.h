/* SPDX-License-Identifier: MIT */
#ifndef CADV_HUB_HUB_H
#define CADV_HUB_HUB_H

#include <stdbool.h>

#include "core/calendar.h"
#include "hub/friends.h"
#include "hub/progress.h"
#include "kits/fps/player.h"
#include "sim/collide.h"

/* The Hall of Long Nights: walking, portal states, entering portals, the
 * Star Arch, the tree, the Wall of Legends and the Christmas Eve epilogue. */

typedef enum cadv_portal_state {
    CADV_PS_SEALED = 0,        /* its day has not come */
    CADV_PS_STILL_THAWING = 1, /* its day came but the game is not ready yet */
    CADV_PS_STARS_MISSING = 2, /* portal 24: Star Arch incomplete */
    CADV_PS_THAWING = 3,       /* open, never entered (reveal not yet seen) */
    CADV_PS_OPEN = 4,
    CADV_PS_MASTERED = 5       /* 3 snowflakes */
} cadv_portal_state;

enum {
    CADV_HUB_EV_ENTER = 1 << 0,      /* wants to enter hub->enter_day */
    CADV_HUB_EV_DENIED = 1 << 1,
    CADV_HUB_EV_SOCKETED = 1 << 2,
    CADV_HUB_EV_CROWNED = 1 << 3,
    CADV_HUB_EV_REVEAL = 1 << 4,     /* a thawing reveal started */
    CADV_HUB_EV_EPILOGUE_END = 1 << 5,
    CADV_HUB_EV_STEP = 1 << 6,
    CADV_HUB_EV_JUMP = 1 << 7,
    CADV_HUB_EV_LANDED = 1 << 8,
    CADV_HUB_EV_SAVE = 1 << 9        /* progress changed, persist it */
};

#define CADV_HUB_EPILOGUE_TICKS (60 * 24)

typedef struct cadv_hub {
    cadv_world world;
    cadv_player player;
    cadv_progress *progress;   /* owned by the app */
    const cadv_friends *friends;
    cadv_datetime now;
    int season_year;
    bool unlock_all;
    cadv_portal_state state[CADV_PORTAL_COUNT];
    int reveal_timer[CADV_PORTAL_COUNT]; /* ticks left of the ice-cracking reveal */
    int enter_day;             /* set with CADV_HUB_EV_ENTER */
    int focus_day;             /* portal the player looks at / stands near, 0 = none */
    int focus_legend;          /* Wall of Legends plaque looked at, -1 = none */
    bool focus_arch;
    bool focus_tree;
    char prompt[96];           /* context line shown at the bottom */
    char message[96];          /* transient message */
    int message_timer;
    int epilogue_timer;        /* > 0 while the epilogue plays */
    int ticks;
    int cooldown;              /* ticks before another portal entry is allowed */
    int last_socketed;         /* stars set by the last USE at the arch */
} cadv_hub;

void cadv_hub_init(cadv_hub *h, cadv_progress *progress, const cadv_friends *friends,
                   cadv_datetime now, int season_year, bool unlock_all);
/* Places the player in front of a portal (after returning from it). */
void cadv_hub_return_to(cadv_hub *h, int day);
void cadv_hub_set_time(cadv_hub *h, cadv_datetime now);
/* Recomputes all portal states from calendar + progress. */
void cadv_hub_update_states(cadv_hub *h);
uint32_t cadv_hub_tick(cadv_hub *h, const cadv_input_frame *in);
bool cadv_hub_query(const cadv_hub *h, const char *key, double *out);
void cadv_hub_teleport(cadv_hub *h, float x, float y, float z, float yaw);
/* Walks to the newest open portal and enters it. */
void cadv_hub_bot(const cadv_hub *h, cadv_input_frame *out);
/* Days whose portal should get a guiding light beam (newest open portal, or
 * on/after Christmas Eve the portals whose stars are missing). Returns count. */
int cadv_hub_beams(const cadv_hub *h, int *days, int max);
const char *cadv_portal_state_name(cadv_portal_state s);

#endif
