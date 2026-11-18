/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_GLTF_H
#define CADV_GFX_GLTF_H

#include <stdbool.h>

#include "gfx/mesh.h"

/* Loads all triangle primitives of a glTF 2.0 file (.gltf with embedded or
 * external buffers, or .glb) into one mesh: positions, normals, first UV set,
 * vertex colours or the material's base colour factor. Node transforms are
 * applied. */
bool cadv_gltf_load(const char *path, cadv_mesh *out);

#endif
