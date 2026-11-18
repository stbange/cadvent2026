/* SPDX-License-Identifier: MIT */
#include "gfx/view.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_split_layouts(void)
{
    cadv_rect r = cadv_view_split(1, 0, 1280, 720);
    TEST_ASSERT_EQUAL_INT(1280, r.w);
    r = cadv_view_split(2, 1, 1281, 720);
    TEST_ASSERT_EQUAL_INT(640, r.x);
    TEST_ASSERT_EQUAL_INT(641, r.w);
    r = cadv_view_split(2, 0, 1280, 720);
    TEST_ASSERT_EQUAL_INT(640, r.w);
    r = cadv_view_split(3, 2, 1280, 720);
    TEST_ASSERT_EQUAL_INT(1280, r.w);
    TEST_ASSERT_EQUAL_INT(0, r.y);
    r = cadv_view_split(3, 0, 1280, 720);
    TEST_ASSERT_EQUAL_INT(360, r.y);
    r = cadv_view_split(4, 3, 1280, 721);
    TEST_ASSERT_EQUAL_INT(640, r.x);
    TEST_ASSERT_EQUAL_INT(0, r.y);
    TEST_ASSERT_EQUAL_INT(360, r.h);
    r = cadv_view_split(4, 1, 1280, 721);
    TEST_ASSERT_EQUAL_INT(360, r.y);
    TEST_ASSERT_EQUAL_INT(361, r.h);
    r = cadv_view_split(4, 9, 100, 100); /* bad index -> full */
    TEST_ASSERT_EQUAL_INT(100, r.w);
}

static void test_internal_size(void)
{
    int w, h;
    cadv_view_internal_size(1280, 720, 225, &w, &h);
    TEST_ASSERT_EQUAL_INT(400, w);
    TEST_ASSERT_EQUAL_INT(225, h);
    cadv_view_internal_size(640, 720, 225, &w, &h);
    TEST_ASSERT_EQUAL_INT(200, w);
    cadv_view_internal_size(100, 50, 225, &w, &h); /* never upscale beyond the view */
    TEST_ASSERT_EQUAL_INT(50, h);
    TEST_ASSERT_EQUAL_INT(100, w);
    cadv_view_internal_size(1, 1000, 1, &w, &h);
    TEST_ASSERT_EQUAL_INT(2, w);
    TEST_ASSERT_EQUAL_INT(2, h);
    cadv_view_internal_size(0, 0, 180, &w, &h);
    TEST_ASSERT_EQUAL_INT(180, h);
    cadv_view_internal_size(0, 0, 0, &w, &h);
    TEST_ASSERT_EQUAL_INT(2, h);
}

static void test_camera_matrices(void)
{
    cadv_camera c = {{0, 1, 0}, 0, 0, 1.2f, 0.1f, 100.0f};
    cadv_mat4 v = cadv_camera_view(&c);
    cadv_vec3 ahead = cadv_m4_mul_point(v, cadv_v3(0, 1, -5));
    cadv_vec3 ndc = cadv_m4_mul_point(cadv_camera_proj(&c, 16.0f / 9.0f), cadv_v3(0, 0, -5));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -5.0f, ahead.z);
    TEST_ASSERT_TRUE(ndc.z > -1.0f && ndc.z < 1.0f);
}

static void test_sprite_frames(void)
{
    cadv_vec3 s = cadv_v3(0, 0, 0);
    /* sprite faces -Z (yaw 0); viewer in front at -Z sees frame 0 */
    TEST_ASSERT_EQUAL_INT(0, cadv_sprite_frame(s, 0.0f, cadv_v3(0, 0, -5), 8));
    TEST_ASSERT_EQUAL_INT(4, cadv_sprite_frame(s, 0.0f, cadv_v3(0, 0, 5), 8));
    TEST_ASSERT_EQUAL_INT(2, cadv_sprite_frame(s, 0.0f, cadv_v3(-5, 0, 0), 8));
    TEST_ASSERT_EQUAL_INT(6, cadv_sprite_frame(s, 0.0f, cadv_v3(5, 0, 0), 8));
    TEST_ASSERT_EQUAL_INT(0, cadv_sprite_frame(s, 0.5f * CADV_PI, cadv_v3(-5, 0, 0), 8));
    TEST_ASSERT_EQUAL_INT(0, cadv_sprite_frame(s, 1.0f, cadv_v3(3, 0, 3), 1));
}

static void test_shadow_light_selection(void)
{
    cadv_light_ref l[6] = {
        {{0, 0, 10}, 5, 1, true},  /* far */
        {{0, 0, 1}, 5, 1, true},   /* near */
        {{0, 0, 2}, 5, 1, false},  /* near but no shadow */
        {{0, 0, 3}, 5, 0, true},   /* off */
        {{0, 0, 4}, 5, 1, true},
        {{0, 0, 6}, 5, 1, true},
    };
    int out[4];
    int n = cadv_select_shadow_lights(l, 6, cadv_v3(0, 0, 0), out, 2);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL_INT(1, out[0]);
    TEST_ASSERT_EQUAL_INT(4, out[1]);
    n = cadv_select_shadow_lights(l, 6, cadv_v3(0, 0, 0), out, 4);
    TEST_ASSERT_EQUAL_INT(4, n);
    TEST_ASSERT_EQUAL_INT(0, out[3]);
    TEST_ASSERT_EQUAL_INT(0, cadv_select_shadow_lights(l, 6, cadv_v3(0, 0, 0), out, 0));
}

static void test_shadow_selection_caps_input(void)
{
    cadv_light_ref many[70];
    int out[4];
    int i;
    for (i = 0; i < 70; ++i) {
        many[i].pos = cadv_v3((float)(70 - i), 0, 0);
        many[i].radius = 1;
        many[i].intensity = 1;
        many[i].casts_shadow = true;
    }
    TEST_ASSERT_EQUAL_INT(4, cadv_select_shadow_lights(many, 70, cadv_v3(0, 0, 0), out, 4));
    TEST_ASSERT_EQUAL_INT(63, out[0]); /* only the first 64 lights are considered */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_split_layouts);
    RUN_TEST(test_internal_size);
    RUN_TEST(test_camera_matrices);
    RUN_TEST(test_sprite_frames);
    RUN_TEST(test_shadow_light_selection);
    RUN_TEST(test_shadow_selection_caps_input);
    return UNITY_END();
}
