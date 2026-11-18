/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_TEST_RANGE_H
#define CADV_PORTALS_TEST_RANGE_H

#include "portal/portal.h"

/* The shared "test range": a compact FPS arena used for every day until that
 * day's real game ships. It exercises the whole kit (movement, weapons,
 * enemies, key door, Shining Star, friend marks, checkpoints, bot). */

size_t cadv_test_range_state_size(void);
bool cadv_test_range_init(void *state, const cadv_portal_cfg *cfg);
void cadv_test_range_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_test_range_result(const void *state);
bool cadv_test_range_query(const void *state, const char *key, double *out);
void cadv_test_range_bot(const void *state, cadv_input_frame *out);
void cadv_test_range_teleport(void *state, float x, float y, float z, float yaw);

#define CADV_TEST_RANGE_STATE_SIZE 262144

#define CADV_TEST_RANGE_DESC(day_, id_, title_, insp_, caps_, marks_)                          \
    {(day_), (id_), (title_), (insp_), (caps_), false, CADV_TEST_RANGE_STATE_SIZE,             \
     cadv_test_range_init, cadv_test_range_tick, cadv_test_range_result,                        \
     cadv_test_range_query, cadv_test_range_bot, cadv_test_range_teleport, marks_}

#endif
