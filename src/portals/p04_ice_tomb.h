/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P04_ICE_TOMB_H
#define CADV_PORTALS_P04_ICE_TOMB_H

#include "portal/portal.h"

/* Day 4 - Ice Tomb of the Winter King. Explorer-cartographer Mara Voss
 * searches a glacier tomb for the Winter King's three Sun Gears. First-person
 * grid traversal; original setting and cast. */

#define CADV_P04_STATE_SIZE (512 * 1024)
#define CADV_P04_JADE 3

bool cadv_p04_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p04_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p04_result(const void *state);
bool cadv_p04_query(const void *state, const char *key, double *out);
void cadv_p04_bot(const void *state, cadv_input_frame *out);
void cadv_p04_teleport(void *state, float x, float y, float z, float yaw);

#endif
