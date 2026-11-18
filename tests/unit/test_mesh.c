/* SPDX-License-Identifier: MIT */
#include "gfx/mesh.h"
#include "unity.h"

static cadv_mesh m;
static const cadv_rgba W = {255, 255, 255, 255};

void setUp(void) { cadv_mesh_init(&m); }
void tearDown(void) { cadv_mesh_free(&m); }

static cadv_vec3 normal_of(uint32_t vi) { return cadv_v3(m.v[vi].normal[0], m.v[vi].normal[1], m.v[vi].normal[2]); }

static void test_quad_normal_and_uv(void)
{
    cadv_mesh_quad(&m, cadv_v3(0, 0, 0), cadv_v3(1, 0, 0), cadv_v3(1, 1, 0), cadv_v3(0, 1, 0), 2, 3, W);
    TEST_ASSERT_EQUAL_UINT(4, m.nv);
    TEST_ASSERT_EQUAL_UINT(6, m.ni);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, normal_of(0).z);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, m.v[1].uv[0]);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, m.v[0].uv[1]);
    cadv_mesh_clear(&m);
    TEST_ASSERT_EQUAL_UINT(0, m.nv);
}

static void test_box_normals_point_outward_or_inward(void)
{
    uint32_t f;
    cadv_mesh_box(&m, cadv_v3(-1, -1, -1), cadv_v3(1, 1, 1), 0.5f, W, false);
    TEST_ASSERT_EQUAL_UINT(24, m.nv);
    for (f = 0; f < 6; ++f) {
        uint32_t vi = f * 4;
        cadv_vec3 centre = cadv_v3(0, 0, 0);
        int k;
        for (k = 0; k < 4; ++k) {
            centre = cadv_v3_add(centre, cadv_v3_scale(cadv_v3(m.v[vi + k].pos[0], m.v[vi + k].pos[1], m.v[vi + k].pos[2]), 0.25f));
        }
        TEST_ASSERT_TRUE(cadv_v3_dot(normal_of(vi), centre) > 0.9f);
    }
    TEST_ASSERT_EQUAL_FLOAT(1.0f, m.v[1].uv[0]); /* 2 units * 0.5 texels per unit */
    cadv_mesh_clear(&m);
    cadv_mesh_box(&m, cadv_v3(-1, -1, -1), cadv_v3(1, 1, 1), 0.0f, W, true);
    for (f = 0; f < 6; ++f) {
        cadv_vec3 p = cadv_v3(m.v[f * 4].pos[0], m.v[f * 4].pos[1], m.v[f * 4].pos[2]);
        TEST_ASSERT_TRUE(cadv_v3_dot(normal_of(f * 4), p) < 0.0f);
    }
}

static void test_prism_and_min_sides(void)
{
    cadv_mesh_prism(&m, cadv_v3(0, 0, 0), 1.0f, 2.0f, 8, 1.0f, W);
    TEST_ASSERT_EQUAL_UINT(32, m.nv);
    TEST_ASSERT_TRUE(normal_of(0).x > 0.9f);
    cadv_mesh_clear(&m);
    cadv_mesh_prism(&m, cadv_v3(0, 0, 0), 1.0f, 2.0f, 1, 0.0f, W); /* clamped to 3 */
    TEST_ASSERT_EQUAL_UINT(12, m.nv);
}

static void test_arch_bounds(void)
{
    cadv_vec3 lo;
    cadv_vec3 hi;
    cadv_mesh_arch(&m, cadv_v3(0, 0, 0), 2.0f, 3.0f, 0.5f, 0.4f, 1, W); /* segments clamped to 2 */
    TEST_ASSERT_TRUE(cadv_mesh_bounds(&m, &lo, &hi));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -1.5f, lo.x);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.5f, hi.x);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 3.5f, hi.y);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.2f, hi.z);
}

static void test_append_transforms(void)
{
    cadv_mesh src;
    cadv_vec3 lo, hi;
    cadv_mesh_init(&src);
    cadv_mesh_box(&src, cadv_v3(0, 0, 0), cadv_v3(1, 1, 1), 0, W, false);
    cadv_mesh_append(&m, &src, cadv_m4_translate(cadv_v3(5, 0, 0)));
    cadv_mesh_append(&m, &src, cadv_m4_rotate_y(CADV_PI));
    TEST_ASSERT_EQUAL_UINT(48, m.nv);
    TEST_ASSERT_EQUAL_UINT(72, m.ni);
    TEST_ASSERT_EQUAL_UINT(24, m.idx[36]); /* second copy is offset */
    cadv_mesh_bounds(&m, &lo, &hi);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -1.0f, lo.x);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 6.0f, hi.x);
    cadv_mesh_free(&src);
}

static void test_bounds_empty_and_growth(void)
{
    cadv_vec3 lo, hi;
    int i;
    TEST_ASSERT_FALSE(cadv_mesh_bounds(&m, &lo, &hi));
    for (i = 0; i < 300; ++i) {
        cadv_mesh_box(&m, cadv_v3((float)i, 0, 0), cadv_v3((float)i + 1, 1, 1), 0, W, false);
    }
    TEST_ASSERT_EQUAL_UINT(7200, m.nv);
    TEST_ASSERT_FALSE(m.oom);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_quad_normal_and_uv);
    RUN_TEST(test_box_normals_point_outward_or_inward);
    RUN_TEST(test_prism_and_min_sides);
    RUN_TEST(test_arch_bounds);
    RUN_TEST(test_append_transforms);
    RUN_TEST(test_bounds_empty_and_growth);
    return UNITY_END();
}
