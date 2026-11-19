/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P03_FROSTFRONT_H
#define CADV_PORTALS_P03_FROSTFRONT_H

#include "portal/portal.h"

/* Day 3 - Frostfront. The Field Commander of the Polar Accord stops the
 * Order of the Pale Sun from weaponising Frostite, commanding on foot with a
 * wrist tablet. Original setting, factions and units. */

#define CADV_P03_STATE_SIZE (640 * 1024)

bool cadv_p03_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p03_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p03_result(const void *state);
bool cadv_p03_query(const void *state, const char *key, double *out);
void cadv_p03_bot(const void *state, cadv_input_frame *out);
void cadv_p03_teleport(void *state, float x, float y, float z, float yaw);

#endif
