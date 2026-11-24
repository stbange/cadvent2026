/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P10_NEON_SYNDIC_H
#define CADV_PORTALS_P10_NEON_SYNDIC_H

#include "portal/portal.h"

/* Day 10 - Neon Syndic. In the rain-soaked megacity of Nivalis, Halcyon
 * Corp runs the city's mood through a cheer-broadcast network; you lead a
 * four-agent cell of the Sleepless Choir to cut its three relays and get out
 * by VTOL. Original city, factions and gear. */

#define CADV_P10_STATE_SIZE (1024 * 1024)

bool cadv_p10_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p10_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p10_result(const void *state);
bool cadv_p10_query(const void *state, const char *key, double *out);
void cadv_p10_bot(const void *state, cadv_input_frame *out);
void cadv_p10_teleport(void *state, float x, float y, float z, float yaw);

#endif
