/* SPDX-License-Identifier: MIT */
#include <stdio.h>

#include "gfx/gltf.h"
#include "unity.h"

static const char k_gltf[] = "{\"asset\": {\"version\": \"2.0\"}, \"scene\": 0, \"scenes\": [{\"nodes\": [0, 1]}], \"nodes\": [{\"mesh\": 0, \"translation\": [10, 0, 0]}, {\"name\": \"empty\"}], \"meshes\": [{\"primitives\": [{\"attributes\": {\"POSITION\": 0, \"NORMAL\": 1, \"TEXCOORD_0\": 2, \"COLOR_0\": 4}, \"indices\": 3, \"material\": 0}, {\"attributes\": {\"NORMAL\": 1}}, {\"attributes\": {\"POSITION\": 0}, \"mode\": 1}, {\"attributes\": {\"POSITION\": 0}}]}], \"materials\": [{\"pbrMetallicRoughness\": {\"baseColorFactor\": [1, 0.5, 0, 1]}}], \"buffers\": [{\"byteLength\": 152, \"uri\": \"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAABAAIAAAAAAIA/AACAPwAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgD8=\"}], \"bufferViews\": [{\"buffer\": 0, \"byteOffset\": 0, \"byteLength\": 36}, {\"buffer\": 0, \"byteOffset\": 36, \"byteLength\": 36}, {\"buffer\": 0, \"byteOffset\": 72, \"byteLength\": 24}, {\"buffer\": 0, \"byteOffset\": 96, \"byteLength\": 6}, {\"buffer\": 0, \"byteOffset\": 104, \"byteLength\": 48}], \"accessors\": [{\"bufferView\": 0, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC3\", \"min\": [0, 0, 0], \"max\": [1, 1, 0]}, {\"bufferView\": 1, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC3\"}, {\"bufferView\": 2, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC2\"}, {\"bufferView\": 3, \"componentType\": 5123, \"count\": 3, \"type\": \"SCALAR\"}, {\"bufferView\": 4, \"componentType\": 5126, \"count\": 3, \"type\": \"VEC4\"}]}";
static const char *k_tmp = "test_tmp.gltf";

void setUp(void) {}
void tearDown(void) { remove(k_tmp); }

static void write_text(const char *s)
{
    FILE *f = fopen(k_tmp, "wb");
    fputs(s, f);
    fclose(f);
}

static void test_loads_triangle_with_transform_and_material(void)
{
    cadv_mesh m;
    cadv_mesh_init(&m);
    write_text(k_gltf);
    TEST_ASSERT_TRUE(cadv_gltf_load(k_tmp, &m));
    /* primitive 0: indexed triangle; primitive 3: non-indexed triangle; lines and
     * position-less primitives are skipped */
    TEST_ASSERT_EQUAL_UINT(6, m.nv);
    TEST_ASSERT_EQUAL_UINT(6, m.ni);
    TEST_ASSERT_EQUAL_FLOAT(11.0f, m.v[1].pos[0]); /* node translation applied */
    TEST_ASSERT_EQUAL_FLOAT(1.0f, m.v[0].normal[2]);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, m.v[1].uv[0]);
    TEST_ASSERT_EQUAL_UINT8(255, m.v[0].color[0]);
    TEST_ASSERT_EQUAL_UINT8(128, m.v[0].color[1]); /* base colour factor 0.5 */
    TEST_ASSERT_EQUAL_UINT8(0, m.v[0].color[2]);
    cadv_mesh_free(&m);
}

static void test_failures(void)
{
    cadv_mesh m;
    cadv_mesh_init(&m);
    TEST_ASSERT_FALSE(cadv_gltf_load("missing.gltf", &m));
    write_text("{ not json");
    TEST_ASSERT_FALSE(cadv_gltf_load(k_tmp, &m));
    write_text("{\"asset\":{\"version\":\"2.0\"},\"buffers\":[{\"byteLength\":4,\"uri\":\"missing.bin\"}]}");
    TEST_ASSERT_FALSE(cadv_gltf_load(k_tmp, &m));
    TEST_ASSERT_EQUAL_UINT(0, m.nv);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_loads_triangle_with_transform_and_material);
    RUN_TEST(test_failures);
    return UNITY_END();
}
