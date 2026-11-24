/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P08_CASCADE_H
#define CADV_PORTALS_P08_CASCADE_H

#include "portal/portal.h"

/* Day 8 - Cascade. Junior technician Ines Kowalczyk arrives at Polaris Deep,
 * an Arctic station drilled into a meteorite, on the day the Lumen Shard
 * resonance test goes wrong. Told entirely in first person: no cutscene ever
 * takes control. Original setting and characters. */

#define CADV_P08_STATE_SIZE (512 * 1024)

bool cadv_p08_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p08_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p08_result(const void *state);
bool cadv_p08_query(const void *state, const char *key, double *out);
void cadv_p08_bot(const void *state, cadv_input_frame *out);
void cadv_p08_teleport(void *state, float x, float y, float z, float yaw);

#endif
