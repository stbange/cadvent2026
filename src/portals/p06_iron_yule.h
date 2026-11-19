/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P06_IRON_YULE_H
#define CADV_PORTALS_P06_IRON_YULE_H

#include "portal/portal.h"

/* Day 6 - Iron Yule. Pilot Sable of the Hollowfrost Militia walks a giant
 * machine through a three-mission campaign in the Winterhold valley, with the
 * convoy AI Santa Claude and his supply crawler Sleighbell on the radio.
 * Original setting, machines and characters. */

#define CADV_P06_STATE_SIZE (512 * 1024)

bool cadv_p06_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p06_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p06_result(const void *state);
bool cadv_p06_query(const void *state, const char *key, double *out);
void cadv_p06_bot(const void *state, cadv_input_frame *out);
void cadv_p06_teleport(void *state, float x, float y, float z, float yaw);

#endif
