/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P05_HEARTHGUARD_H
#define CADV_PORTALS_P05_HEARTHGUARD_H

#include "portal/portal.h"

/* Day 5 - Hearthguard. The gnome village of Kindlewick holds out against the
 * troll raiders of the Rimeback Clans until the midwinter fire is lit; its
 * Captain leads from the front with a signal horn and a command stave.
 * Original setting, factions and units. */

#define CADV_P05_STATE_SIZE (640 * 1024)
#define CADV_P05_RAIDS 3

bool cadv_p05_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p05_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p05_result(const void *state);
bool cadv_p05_query(const void *state, const char *key, double *out);
void cadv_p05_bot(const void *state, cadv_input_frame *out);
void cadv_p05_teleport(void *state, float x, float y, float z, float yaw);

#endif
