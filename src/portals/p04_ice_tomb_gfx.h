/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P04_ICE_TOMB_GFX_H
#define CADV_PORTALS_P04_ICE_TOMB_GFX_H

#include "portal/portal.h"

void cadv_p04_gpu_init(void *state);
void cadv_p04_gpu_free(void *state);
void cadv_p04_draw(void *state, cadv_scene *scene, cadv_camera *cam, cadv_ui *ui, float alpha);

#endif
