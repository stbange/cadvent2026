/* SPDX-License-Identifier: MIT */
#include <math.h>

#include "core/mathx.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_trig_matches_libm_closely(void)
{
    float x;
    for (x = -20.0f; x <= 20.0f; x += 0.01f) {
        TEST_ASSERT_FLOAT_WITHIN(5e-6f, sinf(x), cadv_sinf(x));
        TEST_ASSERT_FLOAT_WITHIN(5e-6f, cosf(x), cadv_cosf(x));
    }
}

static void test_atan2_quadrants(void)
{
    float a;
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_atan2f(0.0f, 0.0f));
    for (a = -3.1f; a <= 3.1f; a += 0.05f) {
        float y = sinf(a) * 2.0f;
        float x = cosf(a) * 2.0f;
        TEST_ASSERT_FLOAT_WITHIN(3e-5f, atan2f(y, x), cadv_atan2f(y, x));
    }
}

static void test_wrap_angle(void)
{
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, cadv_wrap_angle(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, cadv_wrap_angle(0.5f + CADV_TAU * 3.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -0.5f, cadv_wrap_angle(-0.5f - CADV_TAU * 2.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -3.0f, cadv_wrap_angle(3.2831853f));
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 3.0f, cadv_wrap_angle(-3.2831853f));
}

static void test_scalar_helpers(void)
{
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_sqrtf(-4.0f));
    TEST_ASSERT_EQUAL_FLOAT(3.0f, cadv_sqrtf(9.0f));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cadv_clampf(5.0f, 0.0f, 1.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_clampf(-5.0f, 0.0f, 1.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.5f, cadv_clampf(0.5f, 0.0f, 1.0f));
    TEST_ASSERT_EQUAL_FLOAT(2.5f, cadv_lerpf(2.0f, 3.0f, 0.5f));
    TEST_ASSERT_EQUAL_INT(2, cadv_mini(2, 3));
    TEST_ASSERT_EQUAL_INT(3, cadv_maxi(2, 3));
    TEST_ASSERT_EQUAL_FLOAT(2.0f, cadv_minf(2.0f, 3.0f));
    TEST_ASSERT_EQUAL_FLOAT(3.0f, cadv_maxf(2.0f, 3.0f));
    TEST_ASSERT_EQUAL_FLOAT(2.0f, cadv_absf(-2.0f));
}

static void test_vectors(void)
{
    cadv_vec3 a = cadv_v3(1, 2, 3);
    cadv_vec3 b = cadv_v3(4, 5, 6);
    cadv_vec3 c = cadv_v3_cross(cadv_v3(1, 0, 0), cadv_v3(0, 1, 0));
    TEST_ASSERT_EQUAL_FLOAT(32.0f, cadv_v3_dot(a, b));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, c.z);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, cadv_v3_add(a, b).y);
    TEST_ASSERT_EQUAL_FLOAT(-3.0f, cadv_v3_sub(a, b).x);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, cadv_v3_scale(a, 2).z);
    TEST_ASSERT_EQUAL_FLOAT(18.0f, cadv_v3_mul(a, b).z);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 5.0f, cadv_v3_dist(cadv_v3(0, 0, 0), cadv_v3(3, 4, 0)));
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 1.0f, cadv_v3_len(cadv_v3_norm(b)));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_v3_norm(cadv_v3(0, 0, 0)).x);
    TEST_ASSERT_EQUAL_FLOAT(2.5f, cadv_v3_lerp(a, b, 0.5f).x);
}

static void test_yaw_pitch_direction(void)
{
    cadv_vec3 f = cadv_v3_from_yaw_pitch(0, 0);
    cadv_vec3 l = cadv_v3_from_yaw_pitch(0.5f * CADV_PI, 0);
    cadv_vec3 u = cadv_v3_from_yaw_pitch(0, 0.5f * CADV_PI);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -1.0f, f.z);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -1.0f, l.x);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, u.y);
}

static void test_matrices(void)
{
    cadv_mat4 t = cadv_m4_translate(cadv_v3(1, 2, 3));
    cadv_mat4 s = cadv_m4_scale(cadv_v3(2, 2, 2));
    cadv_vec3 p = cadv_m4_mul_point(cadv_m4_mul(t, s), cadv_v3(1, 1, 1));
    cadv_vec3 r = cadv_m4_mul_point(cadv_m4_rotate_y(0.5f * CADV_PI), cadv_v3(1, 0, 0));
    cadv_vec3 rx = cadv_m4_mul_point(cadv_m4_rotate_x(0.5f * CADV_PI), cadv_v3(0, 1, 0));
    cadv_mat4 id = cadv_m4_mul(cadv_m4_identity(), t);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, p.x);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, p.z);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -1.0f, r.z);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, rx.z);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(t.m, id.m, 16);
}

static void test_projection_and_view(void)
{
    cadv_mat4 proj = cadv_m4_perspective(CADV_PI / 2.0f, 1.0f, 1.0f, 100.0f);
    cadv_mat4 view = cadv_m4_look_at(cadv_v3(0, 0, 5), cadv_v3(0, 0, 0), cadv_v3(0, 1, 0));
    cadv_vec3 near_pt = cadv_m4_mul_point(proj, cadv_v3(0, 0, -1));
    cadv_vec3 far_pt = cadv_m4_mul_point(proj, cadv_v3(0, 0, -100));
    cadv_vec3 v = cadv_m4_mul_point(view, cadv_v3(0, 0, 0));
    cadv_mat4 o = cadv_m4_ortho(-1, 1, -1, 1, 0, 10);
    cadv_vec3 oc = cadv_m4_mul_point(o, cadv_v3(1, -1, -10));
    cadv_vec4 w0 = {1, 2, 3, 0};
    cadv_mat4 zero = {{0}};
    cadv_vec3 nodiv;
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -1.0f, near_pt.z);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, far_pt.z);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -5.0f, v.z);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, oc.x);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, oc.z);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cadv_m4_mul_v4(cadv_m4_identity(), w0).x);
    nodiv = cadv_m4_mul_point(zero, cadv_v3(1, 2, 3)); /* w == 0: no divide */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, nodiv.x);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_trig_matches_libm_closely);
    RUN_TEST(test_atan2_quadrants);
    RUN_TEST(test_wrap_angle);
    RUN_TEST(test_scalar_helpers);
    RUN_TEST(test_vectors);
    RUN_TEST(test_yaw_pitch_direction);
    RUN_TEST(test_matrices);
    RUN_TEST(test_projection_and_view);
    return UNITY_END();
}
