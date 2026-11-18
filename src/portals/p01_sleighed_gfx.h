/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P01_SLEIGHED_GFX_H
#define CADV_PORTALS_P01_SLEIGHED_GFX_H

#include "gfx/scene.h"
#include "gfx/view.h"

void cadv_p01_gpu_init(void *state);
void cadv_p01_gpu_free(void *state);
void cadv_p01_draw(void *state, cadv_scene *scene, cadv_camera *cam, cadv_ui *ui, float alpha);

#endif
