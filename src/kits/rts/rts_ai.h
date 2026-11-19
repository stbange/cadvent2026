/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_RTS_RTS_AI_H
#define CADV_KITS_RTS_RTS_AI_H

#include "kits/rts/rts.h"

/* A simple RTS opponent: keeps a planned base standing (rebuilding lost
 * structures), keeps a harvester working, trains a cycling unit mix, defends
 * its home and sends growing attack waves at a target cell. */

typedef struct cadv_rts_ai_build {
    int def;
    int x, z;
} cadv_rts_ai_build;

typedef struct cadv_rts_ai {
    int side;
    const cadv_rts_ai_build *plan;
    int nplan;
    const int *train;      /* unit defs, trained in turn */
    int ntrain;
    int harvester_def;     /* -1: none */
    int income;            /* bonus credits per second (difficulty) */
    int wave_every;        /* ticks between waves */
    int wave_size, wave_max, wave_grow;
    int army_cap;          /* no more training at this many combat units (0 = no cap) */
    cadv_cell target;      /* where waves attack-move to */
    cadv_cell home;
    int home_radius;       /* cells: enemies inside are attacked by idle units */
    /* state */
    int wave_timer;
    int train_index;
    int waves_sent;
    int last_wave;         /* units sent in the last wave */
} cadv_rts_ai;

void cadv_rts_ai_init(cadv_rts_ai *ai, int side);
void cadv_rts_ai_tick(cadv_rts *r, cadv_rts_ai *ai);
/* Combat units (not harvesters) of a side, all or only the idle ones. */
int cadv_rts_ai_army(const cadv_rts *r, int side);
/* Idle combat units (not harvesters) of a side. */
int cadv_rts_ai_idle_army(const cadv_rts *r, int side);

#endif
