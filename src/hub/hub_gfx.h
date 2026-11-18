/* SPDX-License-Identifier: MIT */
#ifndef CADV_HUB_HUB_GFX_H
#define CADV_HUB_HUB_GFX_H

#include "gfx/gl_util.h"
#include "gfx/scene.h"
#include "gfx/view.h"
#include "hub/hub.h"
#include "input/input.h"

#define CADV_HUB_SNOW 160

typedef struct cadv_hub_gfx {
    cadv_gpu_mesh shell, floor, balconies, columns, arches, ice, membrane, plate, tree;
    cadv_gpu_mesh star, socket, banner, plaque, candle, wreath, beam, flake, batch_gpu;
    cadv_mesh batch; /* per-frame small glowing bits: snow, empty sockets, notches */
    unsigned tex_stone, tex_floor, tex_wood, tex_ice, tex_cloth, tex_gold, tex_swirl, tex_snow;
    unsigned tex_day[CADV_PORTAL_COUNT];
    unsigned tex_legend[6];
    int tree_version;      /* progress hash the tree mesh was built for */
    cadv_vec3 snow[CADV_HUB_SNOW];
    bool ready;
} cadv_hub_gfx;

void cadv_hub_gfx_init(cadv_hub_gfx *g, const cadv_hub *h);
void cadv_hub_gfx_free(cadv_hub_gfx *g);
void cadv_hub_gfx_draw(cadv_hub_gfx *g, const cadv_hub *h, const cadv_input *input, cadv_scene *sc,
                       cadv_camera *cam, cadv_ui *ui, float alpha, bool dev_marker);

#endif
