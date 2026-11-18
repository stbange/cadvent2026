/* SPDX-License-Identifier: MIT */
#include <stdio.h>

#include "gfx/texdata.h"
#include "unity.h"

static const uint8_t k_png2x2[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,2,8,6,0,0,0,114,182,13,36,0,0,0,19,73,68,65,84,120,156,99,248,207,192,240,31,12,129,52,8,52,0,0,73,73,9,120,40,160,219,119,0,0,0,0,73,69,78,68,174,66,96,130};
static const char *k_tmp = "test_tex_tmp.png";

void setUp(void) {}
void tearDown(void) { remove(k_tmp); }

static void test_alloc_set_get_wraps(void)
{
    cadv_texdata t;
    TEST_ASSERT_FALSE(cadv_texdata_alloc(&t, 0, 4));
    TEST_ASSERT_FALSE(cadv_texdata_alloc(&t, 4, 9000));
    TEST_ASSERT_TRUE(cadv_texdata_alloc(&t, 4, 4));
    cadv_texdata_set(&t, -1, 5, 9, 8, 7, 6); /* wraps to (3, 1) */
    TEST_ASSERT_EQUAL_UINT8(9, cadv_texdata_get(&t, 3, 1)[0]);
    TEST_ASSERT_EQUAL_UINT8(6, cadv_texdata_get(&t, 7, -3)[3]);
    cadv_texdata_free(&t);
    TEST_ASSERT_NULL(t.rgba);
}

static void test_load_png_from_memory_and_file(void)
{
    cadv_texdata t;
    FILE *f;
    TEST_ASSERT_TRUE(cadv_texdata_load_memory(&t, k_png2x2, (int)sizeof k_png2x2));
    TEST_ASSERT_EQUAL_INT(2, t.w);
    TEST_ASSERT_EQUAL_UINT8(255, cadv_texdata_get(&t, 0, 0)[0]);
    TEST_ASSERT_EQUAL_UINT8(255, cadv_texdata_get(&t, 1, 0)[1]);
    TEST_ASSERT_EQUAL_UINT8(128, cadv_texdata_get(&t, 1, 1)[3]);
    cadv_texdata_free(&t);
    f = fopen(k_tmp, "wb");
    fwrite(k_png2x2, 1, sizeof k_png2x2, f);
    fclose(f);
    TEST_ASSERT_TRUE(cadv_texdata_load(&t, k_tmp));
    TEST_ASSERT_EQUAL_UINT8(255, cadv_texdata_get(&t, 0, 1)[2]);
    cadv_texdata_free(&t);
}

static void test_load_failures(void)
{
    cadv_texdata t;
    static const uint8_t junk[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    TEST_ASSERT_FALSE(cadv_texdata_load(&t, "no-such-image.png"));
    TEST_ASSERT_FALSE(cadv_texdata_load_memory(&t, junk, 8));
    TEST_ASSERT_NULL(t.rgba);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_alloc_set_get_wraps);
    RUN_TEST(test_load_png_from_memory_and_file);
    RUN_TEST(test_load_failures);
    return UNITY_END();
}
