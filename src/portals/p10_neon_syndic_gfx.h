/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P10_NEON_SYNDIC_GFX_H
#define CADV_PORTALS_P10_NEON_SYNDIC_GFX_H

#include "portal/portal.h"

void cadv_p10_gpu_init(void *state);
void cadv_p10_gpu_free(void *state);
void cadv_p10_draw(void *state, cadv_scene *scene, cadv_camera *cam, cadv_ui *ui, float alpha);

#endif
