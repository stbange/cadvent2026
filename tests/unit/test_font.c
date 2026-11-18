/* SPDX-License-Identifier: MIT */
#include <stdlib.h>

#include "gfx/font.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_glyph_lookup(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x0e, cadv_font_glyph('A')[0]); /* .###. */
    TEST_ASSERT_EQUAL_PTR(cadv_font_glyph('?'), cadv_font_glyph(7));
    TEST_ASSERT_EQUAL_PTR(cadv_font_glyph('?'), cadv_font_glyph(200));
    TEST_ASSERT_TRUE(cadv_font_pixel('I', 2, 3));
    TEST_ASSERT_FALSE(cadv_font_pixel('I', 0, 3));
    TEST_ASSERT_FALSE(cadv_font_pixel('A', -1, 0));
    TEST_ASSERT_FALSE(cadv_font_pixel('A', 5, 0));
    TEST_ASSERT_FALSE(cadv_font_pixel('A', 0, 7));
    TEST_ASSERT_FALSE(cadv_font_pixel('A', 0, -1));
}

static void test_every_visible_glyph_has_pixels(void)
{
    int ch;
    for (ch = 33; ch <= 126; ++ch) {
        int x, y, on = 0;
        for (y = 0; y < CADV_FONT_GLYPH_H; ++y) {
            for (x = 0; x < CADV_FONT_GLYPH_W; ++x) {
                on += cadv_font_pixel(ch, x, y);
            }
        }
        TEST_ASSERT_TRUE_MESSAGE(on > 0, "empty glyph");
    }
}

static void test_atlas_and_cells(void)
{
    uint8_t *rgba = malloc(CADV_FONT_ATLAS_W * CADV_FONT_ATLAS_H * 4);
    int x, y;
    cadv_font_build_atlas(rgba);
    cadv_font_cell(127, &x, &y); /* solid block */
    TEST_ASSERT_EQUAL_UINT8(255, rgba[((y + 7) * CADV_FONT_ATLAS_W + x + 7) * 4 + 3]);
    cadv_font_cell(' ', &x, &y);
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_UINT8(0, rgba[(y * CADV_FONT_ATLAS_W + x) * 4 + 3]);
    cadv_font_cell(5, &x, &y); /* maps to '?' */
    {
        int qx, qy;
        cadv_font_cell('?', &qx, &qy);
        TEST_ASSERT_EQUAL_INT(qx, x);
        TEST_ASSERT_EQUAL_INT(qy, y);
    }
    free(rgba);
}

static void test_measure(void)
{
    int w, h;
    cadv_font_measure("", &w, &h);
    TEST_ASSERT_EQUAL_INT(0, w);
    TEST_ASSERT_EQUAL_INT(7, h);
    cadv_font_measure("AB", &w, &h);
    TEST_ASSERT_EQUAL_INT(11, w);
    cadv_font_measure("ABC\nD", &w, &h);
    TEST_ASSERT_EQUAL_INT(17, w);
    TEST_ASSERT_EQUAL_INT(16, h);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_glyph_lookup);
    RUN_TEST(test_every_visible_glyph_has_pixels);
    RUN_TEST(test_atlas_and_cells);
    RUN_TEST(test_measure);
    return UNITY_END();
}
