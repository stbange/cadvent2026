/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_TEST_RANGE_GFX_H
#define CADV_PORTALS_TEST_RANGE_GFX_H

#include "gfx/scene.h"
#include "gfx/view.h"

void cadv_test_range_gpu_init(void *state);
void cadv_test_range_gpu_free(void *state);
void cadv_test_range_draw(void *state, cadv_scene *scene, cadv_camera *cam, cadv_ui *ui, float alpha);

#endif
