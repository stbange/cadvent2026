/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P03_FROSTFRONT_GFX_H
#define CADV_PORTALS_P03_FROSTFRONT_GFX_H

#include "portal/portal.h"

void cadv_p03_gpu_init(void *state);
void cadv_p03_gpu_free(void *state);
void cadv_p03_draw(void *state, cadv_scene *scene, cadv_camera *cam, cadv_ui *ui, float alpha);

#endif
