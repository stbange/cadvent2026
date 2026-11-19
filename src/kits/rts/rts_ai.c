/* SPDX-License-Identifier: MIT */
#include "kits/rts/rts_ai.h"

#include <string.h>

void cadv_rts_ai_init(cadv_rts_ai *ai, int side)
{
    memset(ai, 0, sizeof *ai);
    ai->side = side;
    ai->harvester_def = -1;
    ai->wave_every = 60 * 60;
    ai->wave_size = 3;
    ai->wave_max = 8;
    ai->wave_grow = 1;
    ai->home_radius = 12;
}

static bool planned_standing(const cadv_rts *r, const cadv_rts_ai_build *p, int side)
{
    int b = cadv_rts_building_at(r, p->x, p->z);
    return b >= 0 && r->b[b].alive && r->b[b].def == p->def && r->b[b].owner == side;
}

static bool is_army(const cadv_rts *r, const cadv_rts_unit *u, int side)
{
    return u->alive && u->owner == side && !r->udefs[u->def].harvester && r->udefs[u->def].damage > 0;
}

int cadv_rts_ai_army(const cadv_rts *r, int side)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        n += is_army(r, &r->u[i], side);
    }
    return n;
}

int cadv_rts_ai_idle_army(const cadv_rts *r, int side)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        n += is_army(r, &r->u[i], side) && r->u[i].order == CADV_ORDER_IDLE;
    }
    return n;
}

static void build(cadv_rts *r, cadv_rts_ai *ai)
{
    cadv_rts_side *sd = &r->side[ai->side];
    int i;
    if (sd->ready_building >= 0) {
        for (i = 0; i < ai->nplan; ++i) {
            const cadv_rts_ai_build *p = &ai->plan[i];
            if (p->def == sd->ready_building && !planned_standing(r, p, ai->side) &&
                cadv_rts_place_ready(r, ai->side, p->x, p->z) == CADV_PLACE_OK) {
                return;
            }
        }
        /* nowhere to put it: give the money back */
        sd->credits += r->bdefs[sd->ready_building].cost;
        sd->ready_building = -1;
        return;
    }
    if (sd->queued[0] > 0) {
        return;
    }
    for (i = 0; i < ai->nplan; ++i) {
        if (!planned_standing(r, &ai->plan[i], ai->side)) {
            cadv_rts_queue(r, ai->side, true, ai->plan[i].def);
            return; /* one at a time, in plan order */
        }
    }
}

static void train(cadv_rts *r, cadv_rts_ai *ai)
{
    cadv_rts_side *sd = &r->side[ai->side];
    if (ai->harvester_def >= 0 && cadv_rts_count_units(r, ai->side, ai->harvester_def) == 0) {
        int i;
        bool queued = false;
        for (i = 0; i < sd->queued[1]; ++i) {
            queued = queued || sd->queue[1][i].def == ai->harvester_def;
        }
        if (!queued && cadv_rts_can_train(r, ai->side, ai->harvester_def)) {
            cadv_rts_queue(r, ai->side, false, ai->harvester_def);
            return;
        }
    }
    if (ai->ntrain > 0 && sd->queued[1] < 2 && (ai->army_cap == 0 || cadv_rts_ai_army(r, ai->side) < ai->army_cap) &&
        cadv_rts_queue(r, ai->side, false, ai->train[ai->train_index % ai->ntrain])) {
        ++ai->train_index;
    }
}

static void defend(cadv_rts *r, cadv_rts_ai *ai)
{
    int i;
    int intruder = -1;
    for (i = 0; i < CADV_RTS_MAX_UNITS && intruder < 0; ++i) {
        const cadv_rts_unit *e = &r->u[i];
        if (e->alive && e->owner != ai->side && e->owner != CADV_RTS_NEUTRAL) {
            cadv_cell c = cadv_rts_cell_at(&r->grid, e->x, e->z);
            int dx = c.x - ai->home.x;
            int dz = c.z - ai->home.z;
            if (dx * dx + dz * dz <= ai->home_radius * ai->home_radius) {
                intruder = i;
            }
        }
    }
    if (intruder < 0) {
        return;
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (is_army(r, &r->u[i], ai->side) && r->u[i].order == CADV_ORDER_IDLE) {
            cadv_rts_order_move(r, i, cadv_rts_cell_at(&r->grid, r->u[intruder].x, r->u[intruder].z), true);
        }
    }
}

static void waves(cadv_rts *r, cadv_rts_ai *ai)
{
    int i;
    int sent = 0;
    if (++ai->wave_timer < ai->wave_every || cadv_rts_ai_idle_army(r, ai->side) < ai->wave_size) {
        return;
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS && sent < ai->wave_size; ++i) {
        if (is_army(r, &r->u[i], ai->side) && r->u[i].order == CADV_ORDER_IDLE) {
            cadv_rts_order_move(r, i, ai->target, true);
            ++sent;
        }
    }
    ai->last_wave = sent;
    ++ai->waves_sent;
    ai->wave_timer = 0;
    ai->wave_size = ai->wave_size + ai->wave_grow > ai->wave_max ? ai->wave_max : ai->wave_size + ai->wave_grow;
}

void cadv_rts_ai_tick(cadv_rts *r, cadv_rts_ai *ai)
{
    if (r->ticks % 60 == 0) {
        r->side[ai->side].credits += ai->income;
    }
    if (r->ticks % 15 == 0) {
        build(r, ai);
        train(r, ai);
        defend(r, ai);
    }
    waves(r, ai);
}
