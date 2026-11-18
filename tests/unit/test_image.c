/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <string.h>

#include "core/image.h"
#include "unity.h"

static const char *k_tmp = "test_image_tmp.ppm";

void setUp(void) {}
void tearDown(void) { remove(k_tmp); }

static void write_file(const char *bytes, size_t n)
{
    FILE *f = fopen(k_tmp, "wb");
    TEST_ASSERT_NOT_NULL(f);
    fwrite(bytes, 1, n, f);
    fclose(f);
}

static void test_alloc_rejects_bad_sizes(void)
{
    cadv_image img;
    TEST_ASSERT_FALSE(cadv_image_alloc(&img, 0, 4));
    TEST_ASSERT_FALSE(cadv_image_alloc(&img, 4, -1));
    TEST_ASSERT_FALSE(cadv_image_alloc(&img, 20000, 4));
    TEST_ASSERT_FALSE(cadv_image_alloc(&img, 4, 20000));
    TEST_ASSERT_TRUE(cadv_image_alloc(&img, 2, 2));
    TEST_ASSERT_EQUAL_UINT8(0, img.rgb[11]);
    cadv_image_free(&img);
    TEST_ASSERT_NULL(img.rgb);
}

static void test_get_and_bounds(void)
{
    cadv_image img;
    uint8_t px[3];
    TEST_ASSERT_TRUE(cadv_image_alloc(&img, 3, 2));
    img.rgb[(1 * 3 + 2) * 3 + 1] = 77;
    TEST_ASSERT_TRUE(cadv_image_get(&img, 2, 1, px));
    TEST_ASSERT_EQUAL_UINT8(77, px[1]);
    TEST_ASSERT_FALSE(cadv_image_get(&img, -1, 0, px));
    TEST_ASSERT_FALSE(cadv_image_get(&img, 0, -1, px));
    TEST_ASSERT_FALSE(cadv_image_get(&img, 3, 0, px));
    TEST_ASSERT_FALSE(cadv_image_get(&img, 0, 2, px));
    cadv_image_free(&img);
}

static void test_flip_rows_odd_and_wide(void)
{
    cadv_image img;
    uint8_t px[3];
    int y;
    /* 100 px wide (300 bytes per row) exercises the chunked swap. */
    TEST_ASSERT_TRUE(cadv_image_alloc(&img, 100, 3));
    for (y = 0; y < 3; ++y) {
        memset(img.rgb + (size_t)y * 300u, y + 1, 300);
    }
    cadv_image_flip_rows(&img);
    cadv_image_get(&img, 99, 0, px);
    TEST_ASSERT_EQUAL_UINT8(3, px[0]);
    cadv_image_get(&img, 50, 1, px);
    TEST_ASSERT_EQUAL_UINT8(2, px[0]);
    cadv_image_get(&img, 0, 2, px);
    TEST_ASSERT_EQUAL_UINT8(1, px[2]);
    cadv_image_free(&img);
}

static void test_from_rgba_drops_alpha(void)
{
    static const uint8_t rgba[8] = {1, 2, 3, 255, 4, 5, 6, 0};
    cadv_image img;
    uint8_t px[3];
    TEST_ASSERT_FALSE(cadv_image_from_rgba(&img, 2, 1, NULL));
    TEST_ASSERT_TRUE(cadv_image_from_rgba(&img, 2, 1, rgba));
    cadv_image_get(&img, 1, 0, px);
    TEST_ASSERT_EQUAL_UINT8(4, px[0]);
    TEST_ASSERT_EQUAL_UINT8(6, px[2]);
    cadv_image_free(&img);
}

static void test_ppm_round_trip(void)
{
    cadv_image a;
    cadv_image b;
    TEST_ASSERT_TRUE(cadv_image_alloc(&a, 4, 3));
    a.rgb[0] = 16;
    a.rgb[35] = 200;
    TEST_ASSERT_TRUE(cadv_image_write_ppm(&a, k_tmp));
    TEST_ASSERT_TRUE(cadv_image_read_ppm(&b, k_tmp));
    TEST_ASSERT_EQUAL_INT(4, b.width);
    TEST_ASSERT_EQUAL_INT(3, b.height);
    TEST_ASSERT_EQUAL_MEMORY(a.rgb, b.rgb, 36);
    cadv_image_free(&a);
    cadv_image_free(&b);
}

static void test_ppm_header_with_comment(void)
{
    static const char data[] = "P6\n# made by hand\n1 1\n255\n\x0a\x0b\x0c";
    cadv_image img;
    write_file(data, sizeof data - 1);
    TEST_ASSERT_TRUE(cadv_image_read_ppm(&img, k_tmp));
    TEST_ASSERT_EQUAL_UINT8(0x0b, img.rgb[1]);
    cadv_image_free(&img);
}

static void test_ppm_header_whitespace_variants(void)
{
    static const char data[] = "P6\t1\r\n1\n255\n\x01\x02\x03";
    static const char data_tab_end[] = "P6 1 1\t255\n\x04\x05\x06";
    cadv_image img;
    write_file(data, sizeof data - 1);
    TEST_ASSERT_TRUE(cadv_image_read_ppm(&img, k_tmp));
    TEST_ASSERT_EQUAL_UINT8(3, img.rgb[2]);
    cadv_image_free(&img);
    write_file(data_tab_end, sizeof data_tab_end - 1);
    TEST_ASSERT_TRUE(cadv_image_read_ppm(&img, k_tmp));
    TEST_ASSERT_EQUAL_UINT8(4, img.rgb[0]);
    cadv_image_free(&img);
}

static void test_ppm_rejects_oversized_and_comment_eof(void)
{
    cadv_image img;
    write_file("P6\n1234567 1\n255\n", 18); /* more than 6 digits */
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P6\n20000 1\n255\n", 16); /* too wide for the allocator */
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P6\n# comment without end", 24);
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P6\n1 1\n255", 10); /* number runs into EOF */
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
}

static void test_ppm_rejects_invalid_files(void)
{
    cadv_image img;
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, "definitely-missing.ppm"));
    write_file("P3\n1 1\n255\n", 11);
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("X6\n1 1\n255\n", 11);
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P", 1);
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P6\n1 1\n65535\n", 13);
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P6\nx 1\n255\n", 11);
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P6\n1x1\n255\n", 11);
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P6\n0 1\n255\n", 11);
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    write_file("P6\n2 2\n255\n\x01\x02", 13); /* truncated pixel data */
    TEST_ASSERT_FALSE(cadv_image_read_ppm(&img, k_tmp));
    TEST_ASSERT_NULL(img.rgb);
}

static void test_ppm_write_failures(void)
{
    cadv_image empty;
    cadv_image img;
    memset(&empty, 0, sizeof empty);
    TEST_ASSERT_FALSE(cadv_image_write_ppm(&empty, k_tmp));
    TEST_ASSERT_TRUE(cadv_image_alloc(&img, 1, 1));
    TEST_ASSERT_FALSE(cadv_image_write_ppm(&img, NULL));
    TEST_ASSERT_FALSE(cadv_image_write_ppm(&img, "/nonexistent-dir/x.ppm"));
    cadv_image_free(&img);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_alloc_rejects_bad_sizes);
    RUN_TEST(test_get_and_bounds);
    RUN_TEST(test_flip_rows_odd_and_wide);
    RUN_TEST(test_from_rgba_drops_alpha);
    RUN_TEST(test_ppm_round_trip);
    RUN_TEST(test_ppm_header_with_comment);
    RUN_TEST(test_ppm_header_whitespace_variants);
    RUN_TEST(test_ppm_rejects_oversized_and_comment_eof);
    RUN_TEST(test_ppm_rejects_invalid_files);
    RUN_TEST(test_ppm_write_failures);
    return UNITY_END();
}
