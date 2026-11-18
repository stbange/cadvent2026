/* SPDX-License-Identifier: MIT */
#include <stdlib.h>

#include "gfx/palette.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_ramps_fill_palette(void)
{
    cadv_palette p;
    cadv_ramp r[2] = {{{0, 0, 0, 255}, {255, 0, 0, 255}}, {{0, 0, 0, 255}, {0, 0, 255, 255}}};
    cadv_palette_from_ramps(&p, r, 2);
    TEST_ASSERT_EQUAL_UINT8(0, p.c[0].r);
    TEST_ASSERT_EQUAL_UINT8(255, p.c[127].r);
    TEST_ASSERT_EQUAL_UINT8(255, p.c[255].b);
    cadv_palette_from_ramps(&p, r, 3); /* invalid count falls back to one ramp */
    TEST_ASSERT_EQUAL_UINT8(255, p.c[255].r);
    cadv_palette_from_ramps(&p, r, 0);
    TEST_ASSERT_EQUAL_UINT8(0, p.c[0].r);
}

static void test_single_entry_ramps(void)
{
    cadv_palette p;
    cadv_ramp r[256];
    int i;
    for (i = 0; i < 256; ++i) {
        r[i].dark = cadv_rgba_make((uint8_t)i, 0, 0, 255);
        r[i].light = cadv_rgba_make(0, 0, 0, 255);
    }
    cadv_palette_from_ramps(&p, r, 256);
    TEST_ASSERT_EQUAL_UINT8(200, p.c[200].r);
}

static void test_nearest_and_lut_quantise_to_palette(void)
{
    cadv_palette p;
    uint8_t *lut = malloc(CADV_LUT_DIM * CADV_LUT_DIM * CADV_LUT_DIM * 3);
    int i;
    cadv_palette_hall(&p);
    TEST_ASSERT_EQUAL_INT(255 - 16 * 2, 255 - 32); /* sanity */
    TEST_ASSERT_EQUAL_INT(255, p.c[cadv_palette_nearest(&p, 255, 255, 255)].g);
    TEST_ASSERT_TRUE(p.c[cadv_palette_nearest(&p, 0, 0, 0)].r <= 4);
    {
        /* dark grey stays (nearly) grey */
        const cadv_rgba *c = &p.c[cadv_palette_nearest(&p, 40, 40, 44)];
        TEST_ASSERT_TRUE(abs((int)c->r - (int)c->g) < 12 && abs((int)c->b - (int)c->g) < 12);
    }
    cadv_palette_build_lut(&p, lut);
    for (i = 0; i < CADV_LUT_DIM * CADV_LUT_DIM * CADV_LUT_DIM; i += 97) {
        int k;
        int found = 0;
        for (k = 0; k < 256 && !found; ++k) {
            found = p.c[k].r == lut[i * 3] && p.c[k].g == lut[i * 3 + 1] && p.c[k].b == lut[i * 3 + 2];
        }
        TEST_ASSERT_TRUE(found);
    }
    free(lut);
}

static void test_bayer(void)
{
    int x, y;
    float sum = 0.0f;
    for (y = 0; y < 4; ++y) {
        for (x = 0; x < 4; ++x) {
            sum += cadv_bayer4(x, y);
        }
    }
    TEST_ASSERT_EQUAL_FLOAT(7.5f, sum);
    TEST_ASSERT_EQUAL_FLOAT(cadv_bayer4(1, 2), cadv_bayer4(5, 6));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ramps_fill_palette);
    RUN_TEST(test_single_entry_ramps);
    RUN_TEST(test_nearest_and_lut_quantise_to_palette);
    RUN_TEST(test_bayer);
    return UNITY_END();
}
