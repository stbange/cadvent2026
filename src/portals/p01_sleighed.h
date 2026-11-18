/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P01_SLEIGHED_H
#define CADV_PORTALS_P01_SLEIGHED_H

#include "portal/portal.h"

/* Day 1 - Sleighed! An original FPS set in the Glitterdome Mall: retired mall
 * Santa Brick Holloway takes back his mall from light-stealing space hogs. */

#define CADV_P01_STATE_SIZE (512 * 1024)
#define CADV_P01_SECRETS 6

bool cadv_p01_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p01_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p01_result(const void *state);
bool cadv_p01_query(const void *state, const char *key, double *out);
void cadv_p01_bot(const void *state, cadv_input_frame *out);
void cadv_p01_teleport(void *state, float x, float y, float z, float yaw);

#endif
