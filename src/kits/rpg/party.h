/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_RPG_PARTY_H
#define CADV_KITS_RPG_PARTY_H

#include <stdbool.h>

#include "core/mathx.h"

/* RPG / tactics kit, the party: squad orders and formation slots around the
 * leader, the overclock sliders (speed, aim, awareness) that drain a stamina
 * pool and crash when it runs dry, and a radial ring menu steered by looking. */

typedef enum cadv_order { CADV_ORDER_FOLLOW, CADV_ORDER_HOLD, CADV_ORDER_ATTACK, CADV_ORDER_COVER, CADV_ORDERS } cadv_order;
typedef enum cadv_slider { CADV_OC_SPEED, CADV_OC_AIM, CADV_OC_AWARE, CADV_OC_COUNT } cadv_slider;

const char *cadv_order_name(cadv_order o);
/* Where squad member 'slot' of 'count' stands: a wedge behind the leader. */
cadv_vec3 cadv_formation_slot(cadv_vec3 leader, float yaw, int slot, int count, float spacing);

typedef struct cadv_overclock {
    bool on[CADV_OC_COUNT];
    float stamina, max;
    float drain;        /* per second for each slider that is on */
    float regen;        /* per second with every slider off */
    float crash_len;    /* seconds of crash when the pool runs dry */
    float crash_t;      /* seconds of crash left */
} cadv_overclock;

void cadv_overclock_init(cadv_overclock *o, float max, float drain, float regen, float crash_len);
/* Switches a slider; refused (false) while crashed. */
bool cadv_overclock_set(cadv_overclock *o, cadv_slider which, bool on);
int cadv_overclock_active(const cadv_overclock *o);
/* Advances; true on the tick the pool ran dry and the crash began. */
bool cadv_overclock_tick(cadv_overclock *o, float dt);
bool cadv_overclock_crashed(const cadv_overclock *o);
/* 1 when the slider is on, 0 when off; during a crash everything is -0.5. */
float cadv_overclock_level(const cadv_overclock *o, cadv_slider which);

/* Radial ring: hold to open, look to pick a sector, release to choose. */
typedef struct cadv_ring {
    bool open;
    int count;
    int sel;            /* -1 until the look leaves the dead zone */
    float x, y;         /* accumulated look */
} cadv_ring;

void cadv_ring_open(cadv_ring *r, int count);
/* Feeds this tick's look; sector 0 is up, counting clockwise. */
void cadv_ring_look(cadv_ring *r, float dx, float dy);
/* Closes; returns the chosen sector or -1. */
int cadv_ring_close(cadv_ring *r);

#endif
