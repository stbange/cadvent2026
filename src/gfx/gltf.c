/* SPDX-License-Identifier: MIT */
#include "gfx/gltf.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#define CGLTF_IMPLEMENTATION
#include "cgltf.h" /* GCOVR_EXCL_LINE */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

static uint8_t to8(float f)
{
    if (f <= 0.0f) {
        return 0;
    }
    return f >= 1.0f ? 255 : (uint8_t)(f * 255.0f + 0.5f);
}

static void add_primitive(cadv_mesh *out, const cgltf_primitive *prim, cadv_mat4 xf)
{
    const cgltf_accessor *pos = NULL;
    const cgltf_accessor *nrm = NULL;
    const cgltf_accessor *uv = NULL;
    const cgltf_accessor *col = NULL;
    cadv_mesh tmp;
    float base[4] = {1, 1, 1, 1};
    cgltf_size i;
    if (prim->type != cgltf_primitive_type_triangles) {
        return;
    }
    for (i = 0; i < prim->attributes_count; ++i) {
        const cgltf_attribute *a = &prim->attributes[i];
        if (a->type == cgltf_attribute_type_position) {
            pos = a->data;
        } else if (a->type == cgltf_attribute_type_normal) {
            nrm = a->data;
        } else if (a->type == cgltf_attribute_type_texcoord && a->index == 0) {
            uv = a->data;
        } else if (a->type == cgltf_attribute_type_color && a->index == 0) {
            col = a->data;
        }
    }
    if (!pos) {
        return;
    }
    if (prim->material && prim->material->has_pbr_metallic_roughness) {
        const cgltf_float *f = prim->material->pbr_metallic_roughness.base_color_factor;
        base[0] = f[0];
        base[1] = f[1];
        base[2] = f[2];
        base[3] = f[3];
    }
    cadv_mesh_init(&tmp);
    for (i = 0; i < pos->count; ++i) {
        float p[3] = {0, 0, 0};
        float n[3] = {0, 1, 0};
        float t[2] = {0, 0};
        float c[4] = {1, 1, 1, 1};
        cgltf_accessor_read_float(pos, i, p, 3);
        if (nrm) {
            cgltf_accessor_read_float(nrm, i, n, 3);
        }
        if (uv) {
            cgltf_accessor_read_float(uv, i, t, 2);
        }
        if (col) {
            cgltf_accessor_read_float(col, i, c, 4);
        }
        cadv_mesh_vertex(&tmp, cadv_v3(p[0], p[1], p[2]), cadv_v3(n[0], n[1], n[2]), t[0], t[1],
                         cadv_rgba_make(to8(c[0] * base[0]), to8(c[1] * base[1]),
                                        to8(c[2] * base[2]), to8(c[3] * base[3])));
    }
    if (prim->indices) {
        for (i = 0; i + 2 < prim->indices->count; i += 3) {
            cadv_mesh_tri(&tmp, (uint32_t)cgltf_accessor_read_index(prim->indices, i),
                          (uint32_t)cgltf_accessor_read_index(prim->indices, i + 1),
                          (uint32_t)cgltf_accessor_read_index(prim->indices, i + 2));
        }
    } else {
        for (i = 0; i + 2 < pos->count; i += 3) {
            cadv_mesh_tri(&tmp, (uint32_t)i, (uint32_t)i + 1, (uint32_t)i + 2);
        }
    }
    cadv_mesh_append(out, &tmp, xf);
    cadv_mesh_free(&tmp);
}

bool cadv_gltf_load(const char *path, cadv_mesh *out)
{
    cgltf_options opt;
    cgltf_data *data = NULL;
    cgltf_size n;
    memset(&opt, 0, sizeof opt);
    if (cgltf_parse_file(&opt, path, &data) != cgltf_result_success) {
        return false;
    }
    if (cgltf_load_buffers(&opt, data, path) != cgltf_result_success) {
        cgltf_free(data);
        return false;
    }
    for (n = 0; n < data->nodes_count; ++n) {
        const cgltf_node *node = &data->nodes[n];
        cadv_mat4 xf;
        cgltf_size p;
        if (!node->mesh) {
            continue;
        }
        cgltf_node_transform_world(node, xf.m);
        for (p = 0; p < node->mesh->primitives_count; ++p) {
            add_primitive(out, &node->mesh->primitives[p], xf);
        }
    }
    cgltf_free(data);
    return true;
}
