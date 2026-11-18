/* SPDX-License-Identifier: MIT */
#include <string.h>

#include "gfx/procgen.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static const cadv_rgba A = {120, 110, 100, 255};
static const cadv_rgba B = {40, 40, 50, 255};

static void test_noise_range_and_tiling(void)
{
    float x;
    for (x = -3.0f; x < 10.0f; x += 0.37f) {
        float n = cadv_noise2(1u, x, x * 0.5f, 4);
        TEST_ASSERT_TRUE(n >= 0.0f && n <= 1.0f);
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, cadv_noise2(1u, x, 1.25f, 4), cadv_noise2(1u, x + 4.0f, 1.25f, 4));
    }
    TEST_ASSERT_TRUE(cadv_noise2(1u, 0.5f, 0.5f, 0) >= 0.0f); /* period clamped to 1 */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_fbm2(3u, 0.1f, 0.2f, 4, 0));
    TEST_ASSERT_TRUE(cadv_fbm2(3u, 0.1f, 0.2f, 4, 4) <= 1.0f);
}

static void test_all_kinds_generate_deterministically(void)
{
    int k;
    for (k = 0; k < CADV_TEX_KIND_COUNT; ++k) {
        cadv_texdata a, b;
        TEST_ASSERT_TRUE(cadv_procgen_texture(&a, (cadv_tex_kind)k, 32, 42u, A, B));
        TEST_ASSERT_TRUE(cadv_procgen_texture(&b, (cadv_tex_kind)k, 32, 42u, A, B));
        TEST_ASSERT_EQUAL_INT(32, a.w);
        TEST_ASSERT_EQUAL_MEMORY(a.rgba, b.rgba, 32 * 32 * 4);
        TEST_ASSERT_EQUAL_UINT8(255, a.rgba[3]);
        cadv_texdata_free(&a);
        cadv_texdata_free(&b);
    }
}

static void test_bricks_have_mortar_lines(void)
{
    cadv_texdata t;
    TEST_ASSERT_TRUE(cadv_procgen_texture(&t, CADV_TEX_STONE_BRICKS, 64, 1u, A, cadv_rgba_make(0, 0, 0, 255)));
    TEST_ASSERT_EQUAL_UINT8(0, cadv_texdata_get(&t, 10, 0)[0]); /* top row is mortar */
    TEST_ASSERT_TRUE(cadv_texdata_get(&t, 10, 5)[0] > 0);
    cadv_texdata_free(&t);
}

static void test_rejects_bad_parameters(void)
{
    cadv_texdata t;
    TEST_ASSERT_FALSE(cadv_procgen_texture(&t, CADV_TEX_ICE, 48, 1u, A, B));
    TEST_ASSERT_FALSE(cadv_procgen_texture(&t, CADV_TEX_ICE, 4, 1u, A, B));
    TEST_ASSERT_FALSE(cadv_procgen_texture(&t, CADV_TEX_ICE, 512, 1u, A, B));
    TEST_ASSERT_FALSE(cadv_procgen_texture(&t, CADV_TEX_KIND_COUNT, 32, 1u, A, B));
    TEST_ASSERT_FALSE(cadv_procgen_texture(&t, (cadv_tex_kind)-1, 32, 1u, A, B));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_noise_range_and_tiling);
    RUN_TEST(test_all_kinds_generate_deterministically);
    RUN_TEST(test_bricks_have_mortar_lines);
    RUN_TEST(test_rejects_bad_parameters);
    return UNITY_END();
}
