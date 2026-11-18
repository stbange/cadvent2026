/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_RTS_RTS_H
#define CADV_KITS_RTS_RTS_H

#include <stdbool.h>
#include <stdint.h>

#include "kits/rts/grid.h"

/* RTS kit: sides with credits and power, build queues, buildings, units with
 * orders and control groups, harvesters, simple combat. Deterministic: all
 * state lives in cadv_rts and advances in fixed ticks; cadv_rts_hash()
 * fingerprints it for lockstep checks. Portals supply the unit and building
 * tables. */

#define CADV_RTS_SIDES 2
#define CADV_RTS_MAX_BUILDINGS 64
#define CADV_RTS_MAX_UNITS 96
#define CADV_RTS_UNIT_PATH 64
#define CADV_RTS_GROUPS 4
#define CADV_RTS_QUEUE 5

enum { CADV_BF_REFINERY = 1 << 0, CADV_BF_FACTORY = 1 << 1, CADV_BF_DEFENCE = 1 << 2, CADV_BF_HQ = 1 << 3 };

typedef struct cadv_rts_bdef {
    const char *name;
    int fw, fh;         /* footprint in cells */
    int cost;
    int power;          /* produced (> 0) or consumed (< 0) */
    int hp;
    int build_ticks;    /* at full power */
    uint32_t flags;
    float range;        /* defences: firing range in metres */
    int damage;
    int cooldown;       /* ticks between shots */
} cadv_rts_bdef;

typedef struct cadv_rts_udef {
    const char *name;
    int cost;
    int hp;
    float speed;        /* metres per tick */
    float range;
    int damage;
    int cooldown;
    int build_ticks;
    bool harvester;
    int carry;          /* harvesters: ore per trip */
} cadv_rts_udef;

typedef enum cadv_order { CADV_ORDER_IDLE = 0, CADV_ORDER_MOVE, CADV_ORDER_ATTACK, CADV_ORDER_ATTACK_MOVE, CADV_ORDER_HARVEST } cadv_order;

typedef enum cadv_harvest_state { CADV_HARV_SEEK = 0, CADV_HARV_TO_ORE, CADV_HARV_MINING, CADV_HARV_RETURN, CADV_HARV_UNLOAD } cadv_harvest_state;

typedef struct cadv_rts_building {
    bool alive;
    int def;
    int owner;
    int x, z;
    int hp;
    int cooldown;
} cadv_rts_building;

typedef struct cadv_rts_unit {
    bool alive;
    int def;
    int owner;
    float x, z;         /* world metres on the ground plane */
    float yaw;
    int hp;
    int cooldown;
    cadv_order order;
    cadv_cell goal;
    int target;         /* unit index, or CADV_RTS_TARGET_BUILDING + building index, -1 none */
    cadv_cell path[CADV_RTS_UNIT_PATH];
    int path_len, path_pos;
    int repath;         /* ticks until a blocked path is retried */
    cadv_harvest_state hstate;
    int cargo;
    int timer;
} cadv_rts_unit;

#define CADV_RTS_TARGET_BUILDING 1000

typedef struct cadv_rts_queue_item {
    bool building;      /* structure (placed when ready) or unit (spawns) */
    int def;
    int progress;       /* ticks of work done */
} cadv_rts_queue_item;

typedef struct cadv_rts_side {
    int credits;
    int power_made, power_used;
    cadv_rts_queue_item queue[CADV_RTS_QUEUE];
    int queued;
    int ready_building; /* finished structure waiting for placement, -1 none */
    int rally_x, rally_z;
    uint8_t group[CADV_RTS_GROUPS][CADV_RTS_MAX_UNITS]; /* membership flags */
    int harvested;      /* total ore delivered */
    int units_lost;
} cadv_rts_side;

typedef enum cadv_rts_event {
    CADV_RTS_EV_BUILT = 1 << 0,       /* structure ready to place */
    CADV_RTS_EV_UNIT_READY = 1 << 1,
    CADV_RTS_EV_UNIT_LOST = 1 << 2,
    CADV_RTS_EV_BUILDING_LOST = 1 << 3,
    CADV_RTS_EV_LOW_POWER = 1 << 4,
    CADV_RTS_EV_UNLOAD = 1 << 5,
    CADV_RTS_EV_SHOT = 1 << 6
} cadv_rts_event;

typedef struct cadv_rts {
    cadv_rts_grid grid;
    const cadv_rts_bdef *bdefs;
    int nbdefs;
    const cadv_rts_udef *udefs;
    int nudefs;
    cadv_rts_side side[CADV_RTS_SIDES];
    cadv_rts_building b[CADV_RTS_MAX_BUILDINGS];
    cadv_rts_unit u[CADV_RTS_MAX_UNITS];
    int ticks;
    uint32_t events[CADV_RTS_SIDES]; /* this tick */
    int last_lost_unit;
} cadv_rts;

bool cadv_rts_init(cadv_rts *r, int w, int h, float cell, const cadv_rts_bdef *bdefs, int nbdefs,
                   const cadv_rts_udef *udefs, int nudefs);
/* Places a finished building immediately (setup, or after the queue). */
int cadv_rts_add_building(cadv_rts *r, int owner, int def, int x, int z);
int cadv_rts_add_unit(cadv_rts *r, int owner, int def, float x, float z);
void cadv_rts_destroy_building(cadv_rts *r, int i);

/* Power balance: 1.0 when made >= used, otherwise made/used, at least 0.25. */
float cadv_rts_power_factor(const cadv_rts *r, int side);
bool cadv_rts_defences_online(const cadv_rts *r, int side);
void cadv_rts_update_power(cadv_rts *r, int side);

/* Build queue: pays up front, refuses when broke, full or (units) without a
 * factory. Returns false on refusal. */
bool cadv_rts_queue(cadv_rts *r, int side, bool building, int def);
/* Cancels the last queued item and refunds its cost. */
bool cadv_rts_cancel(cadv_rts *r, int side);
/* Places the finished structure; false if the spot is invalid. */
cadv_place_result cadv_rts_place_ready(cadv_rts *r, int side, int x, int z);

void cadv_rts_order_move(cadv_rts *r, int unit, cadv_cell goal, bool attack_move);
void cadv_rts_order_attack(cadv_rts *r, int unit, int target);
void cadv_rts_order_stop(cadv_rts *r, int unit);
void cadv_rts_order_harvest(cadv_rts *r, int unit);
void cadv_rts_group_set(cadv_rts *r, int side, int group, const int *units, int count);
/* Orders every live member of a group; returns how many were ordered. */
int cadv_rts_group_move(cadv_rts *r, int side, int group, cadv_cell goal, bool attack_move);

void cadv_rts_tick(cadv_rts *r);
int cadv_rts_count_units(const cadv_rts *r, int side, int def); /* def < 0 = any */
int cadv_rts_count_buildings(const cadv_rts *r, int side, int def);
uint64_t cadv_rts_hash(const cadv_rts *r);

#endif
