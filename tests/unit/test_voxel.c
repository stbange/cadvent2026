/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx/voxel.h"
#include "unity.h"

static const char *k_tmp = "test_tmp.vox";

void setUp(void) {}
void tearDown(void) { remove(k_tmp); }

static void make_l_shape(cadv_voxels *v)
{
    TEST_ASSERT_TRUE(cadv_voxels_alloc(v, 3, 2, 2));
    v->pal[1] = cadv_rgba_make(200, 10, 10, 255);
    v->pal[2] = cadv_rgba_make(10, 200, 10, 255);
    cadv_voxels_set(v, 0, 0, 0, 1);
    cadv_voxels_set(v, 1, 0, 0, 1);
    cadv_voxels_set(v, 2, 0, 0, 1);
    cadv_voxels_set(v, 0, 1, 0, 2);
}

static void test_alloc_get_set_count(void)
{
    cadv_voxels v;
    TEST_ASSERT_FALSE(cadv_voxels_alloc(&v, 0, 1, 1));
    TEST_ASSERT_FALSE(cadv_voxels_alloc(&v, 1, 300, 1));
    make_l_shape(&v);
    TEST_ASSERT_EQUAL_INT(4, cadv_voxels_count(&v));
    TEST_ASSERT_EQUAL_UINT8(2, cadv_voxels_get(&v, 0, 1, 0));
    TEST_ASSERT_EQUAL_UINT8(0, cadv_voxels_get(&v, -1, 0, 0));
    TEST_ASSERT_EQUAL_UINT8(0, cadv_voxels_get(&v, 0, 2, 0));
    TEST_ASSERT_EQUAL_UINT8(0, cadv_voxels_get(&v, 0, 0, 2));
    cadv_voxels_set(&v, 9, 9, 9, 1); /* ignored */
    cadv_voxels_set(&v, -1, 0, 0, 1);
    TEST_ASSERT_EQUAL_INT(4, cadv_voxels_count(&v));
    cadv_voxels_free(&v);
}

static void test_vox_round_trip_via_memory_and_file(void)
{
    cadv_voxels v, w;
    size_t n;
    uint8_t *buf;
    FILE *f;
    make_l_shape(&v);
    n = cadv_vox_write(&v, NULL, 0);
    buf = malloc(n);
    TEST_ASSERT_EQUAL_UINT(n, cadv_vox_write(&v, buf, n));
    TEST_ASSERT_EQUAL_MEMORY("VOX ", buf, 4);
    TEST_ASSERT_TRUE(cadv_vox_parse(&w, buf, n));
    TEST_ASSERT_EQUAL_INT(3, w.w);
    TEST_ASSERT_EQUAL_INT(2, w.h);
    TEST_ASSERT_EQUAL_UINT8(2, cadv_voxels_get(&w, 0, 1, 0));
    TEST_ASSERT_EQUAL_UINT8(200, w.pal[1].r);
    cadv_voxels_free(&w);
    f = fopen(k_tmp, "wb");
    fwrite(buf, 1, n, f);
    fclose(f);
    TEST_ASSERT_TRUE(cadv_vox_load(&w, k_tmp));
    TEST_ASSERT_EQUAL_INT(4, cadv_voxels_count(&w));
    cadv_voxels_free(&w);
    free(buf);
    cadv_voxels_free(&v);
}

static void test_vox_rejects_corrupt_data(void)
{
    cadv_voxels v, w;
    size_t n;
    uint8_t *buf;
    FILE *f;
    make_l_shape(&v);
    n = cadv_vox_write(&v, NULL, 0);
    buf = malloc(n);
    cadv_vox_write(&v, buf, n);
    TEST_ASSERT_FALSE(cadv_vox_parse(&w, buf, 10));     /* too short */
    TEST_ASSERT_FALSE(cadv_vox_parse(&w, buf, n - 1));  /* MAIN exceeds data */
    buf[0] = 'X';
    TEST_ASSERT_FALSE(cadv_vox_parse(&w, buf, n));
    buf[0] = 'V';
    buf[40 + 4] = 0xff; /* corrupt XYZI content size */
    TEST_ASSERT_FALSE(cadv_vox_parse(&w, buf, n));
    cadv_vox_write(&v, buf, n);
    buf[48] = 200; /* XYZI voxel count larger than content */
    TEST_ASSERT_FALSE(cadv_vox_parse(&w, buf, n));
    cadv_vox_write(&v, buf, n);
    memcpy(buf + 40, "ABCD", 4); /* no XYZI chunk */
    TEST_ASSERT_FALSE(cadv_vox_parse(&w, buf, n));
    cadv_vox_write(&v, buf, n);
    buf[24] = 0; /* SIZE x = 0 -> alloc fails */
    buf[25] = 0;
    TEST_ASSERT_FALSE(cadv_vox_parse(&w, buf, n));
    TEST_ASSERT_FALSE(cadv_vox_load(&w, "missing.vox"));
    f = fopen(k_tmp, "wb");
    fclose(f);
    TEST_ASSERT_FALSE(cadv_vox_load(&w, k_tmp));
    free(buf);
    cadv_voxels_free(&v);
}

static void test_vox_without_palette_uses_grey(void)
{
    static const uint8_t data[] = {
        'V', 'O', 'X', ' ', 150, 0, 0, 0, 'M', 'A', 'I', 'N', 0, 0, 0, 0, 44, 0, 0, 0,
        'S', 'I', 'Z', 'E', 12, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
        'X', 'Y', 'Z', 'I', 8, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 9};
    cadv_voxels v;
    TEST_ASSERT_TRUE(cadv_vox_parse(&v, data, sizeof data));
    TEST_ASSERT_EQUAL_UINT8(9, cadv_voxels_get(&v, 0, 0, 0));
    TEST_ASSERT_EQUAL_UINT8(9, v.pal[9].r);
    cadv_voxels_free(&v);
}

static void test_greedy_mesh_merges_faces(void)
{
    cadv_voxels v;
    cadv_mesh m;
    cadv_vec3 lo, hi;
    TEST_ASSERT_TRUE(cadv_voxels_alloc(&v, 4, 1, 4));
    {
        int x, z;
        for (z = 0; z < 4; ++z) {
            for (x = 0; x < 4; ++x) {
                cadv_voxels_set(&v, x, 0, z, 1);
            }
        }
    }
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.5f, &m);
    TEST_ASSERT_EQUAL_UINT(6 * 4, m.nv); /* a slab becomes exactly 6 quads */
    cadv_mesh_bounds(&m, &lo, &hi);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, lo.x);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, hi.x);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, lo.y);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, hi.y);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
}

static void test_greedy_mesh_colours_and_normals(void)
{
    cadv_voxels v;
    cadv_mesh m;
    uint32_t i;
    int up = 0, down = 0;
    make_l_shape(&v);
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 1.0f, &m);
    for (i = 0; i < m.nv; i += 4) {
        up += m.v[i].normal[1] > 0.5f;
        down += m.v[i].normal[1] < -0.5f;
    }
    TEST_ASSERT_TRUE(up >= 2);
    TEST_ASSERT_EQUAL_INT(1, down); /* bottom row merges into one quad */
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
}

static void test_bake_sprites(void)
{
    cadv_voxels v;
    cadv_texdata c, n;
    int fw, fh;
    make_l_shape(&v);
    TEST_ASSERT_FALSE(cadv_voxels_bake_sprites(&v, 0, 2, &c, &n, &fw, &fh));
    TEST_ASSERT_FALSE(cadv_voxels_bake_sprites(&v, 8, 9, &c, &n, &fw, &fh));
    TEST_ASSERT_TRUE(cadv_voxels_bake_sprites(&v, 8, 2, &c, &n, &fw, &fh));
    TEST_ASSERT_EQUAL_INT(8, fw); /* ceil-ish(sqrt(13)) = 4 voxels * 2 px */
    TEST_ASSERT_EQUAL_INT(4, fh);
    TEST_ASSERT_EQUAL_INT(64, c.w);
    {
        /* frame 0 (seen from +Z): bottom row has the red bar */
        int x, red = 0, green = 0;
        for (x = 0; x < fw; ++x) {
            const uint8_t *p = cadv_texdata_get(&c, x, fh - 1);
            red += p[0] == 200;
            p = cadv_texdata_get(&c, x, 0);
            green += p[1] == 200;
        }
        TEST_ASSERT_TRUE(red >= 5);
        TEST_ASSERT_TRUE(green >= 1);
        TEST_ASSERT_EQUAL_UINT8(255, cadv_texdata_get(&n, fw / 2, fh - 1)[3]);
    }
    cadv_texdata_free(&c);
    cadv_texdata_free(&n);
    TEST_ASSERT_TRUE(cadv_voxels_bake_sprites(&v, 4, 1, &c, NULL, &fw, &fh));
    cadv_texdata_free(&c);
    cadv_voxels_free(&v);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_alloc_get_set_count);
    RUN_TEST(test_vox_round_trip_via_memory_and_file);
    RUN_TEST(test_vox_rejects_corrupt_data);
    RUN_TEST(test_vox_without_palette_uses_grey);
    RUN_TEST(test_greedy_mesh_merges_faces);
    RUN_TEST(test_greedy_mesh_colours_and_normals);
    RUN_TEST(test_bake_sprites);
    return UNITY_END();
}
