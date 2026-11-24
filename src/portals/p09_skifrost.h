/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P09_SKIFROST_H
#define CADV_PORTALS_P09_SKIFROST_H

#include "portal/portal.h"

/* Day 9 - Skifrost. The Rime Wolves and the Ember Owls settle the festival
 * games of the Northern Reach with Capture the Flag on Snowglobe Basin:
 * skiing, jetpacks, slow discs, generators and turrets. Original setting,
 * tribes and weapons. */

#define CADV_P09_STATE_SIZE (512 * 1024)

bool cadv_p09_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p09_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p09_result(const void *state);
bool cadv_p09_query(const void *state, const char *key, double *out);
void cadv_p09_bot(const void *state, cadv_input_frame *out);
void cadv_p09_teleport(void *state, float x, float y, float z, float yaw);

#endif
