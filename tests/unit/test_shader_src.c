/* SPDX-License-Identifier: MIT */
#include <string.h>

#include "gfx/shader_src.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_desktop_preamble(void)
{
    char buf[256];
    size_t n = cadv_shader_build(buf, sizeof buf, CADV_GL_DESKTOP_33, CADV_SHADER_VERTEX,
                                 "void main(){}\n");
    TEST_ASSERT_EQUAL_UINT(strlen(buf), n);
    TEST_ASSERT_EQUAL_INT(0, strncmp(buf, "#version 330 core\n", 18));
    TEST_ASSERT_NOT_NULL(strstr(buf, "#define CADV_GLES 0\n"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "#define CADV_VERTEX 1\n"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "#line 1\nvoid main(){}\n"));
}

static void test_gles_preamble(void)
{
    char buf[256];
    cadv_shader_build(buf, sizeof buf, CADV_GL_ES_30, CADV_SHADER_FRAGMENT, "x");
    TEST_ASSERT_EQUAL_INT(0, strncmp(buf, "#version 300 es\n", 16));
    TEST_ASSERT_NOT_NULL(strstr(buf, "precision highp float;"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "#define CADV_GLES 1\n"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "#define CADV_FRAGMENT 1\n"));
}

static void test_truncation_reports_needed_length(void)
{
    char small[8];
    char big[256];
    size_t need = cadv_shader_build(small, sizeof small, CADV_GL_ES_30, CADV_SHADER_VERTEX, "abc");
    size_t full = cadv_shader_build(big, sizeof big, CADV_GL_ES_30, CADV_SHADER_VERTEX, "abc");
    TEST_ASSERT_EQUAL_UINT(full, need);
    TEST_ASSERT_TRUE(need >= sizeof small);
    TEST_ASSERT_EQUAL_UINT(7, strlen(small));
}

static void test_null_body(void)
{
    char buf[4] = "zz";
    TEST_ASSERT_EQUAL_UINT(0, cadv_shader_build(buf, sizeof buf, CADV_GL_DESKTOP_33,
                                                CADV_SHADER_VERTEX, NULL));
    TEST_ASSERT_EQUAL_STRING("", buf);
    TEST_ASSERT_EQUAL_UINT(0, cadv_shader_build(NULL, 0, CADV_GL_DESKTOP_33,
                                                CADV_SHADER_VERTEX, NULL));
    TEST_ASSERT_EQUAL_UINT(0, cadv_shader_build(buf, 0, CADV_GL_DESKTOP_33,
                                                CADV_SHADER_VERTEX, NULL));
    TEST_ASSERT_EQUAL_STRING("", buf); /* cap 0: buffer untouched */
}

static void test_profile_names_and_native(void)
{
    TEST_ASSERT_EQUAL_STRING("OpenGL 3.3 Core", cadv_gl_profile_name(CADV_GL_DESKTOP_33));
    TEST_ASSERT_EQUAL_STRING("OpenGL ES 3.0", cadv_gl_profile_name(CADV_GL_ES_30));
#if defined(__EMSCRIPTEN__)
    TEST_ASSERT_EQUAL_INT(CADV_GL_ES_30, cadv_gl_native_profile());
#else
    TEST_ASSERT_EQUAL_INT(CADV_GL_DESKTOP_33, cadv_gl_native_profile());
#endif
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_desktop_preamble);
    RUN_TEST(test_gles_preamble);
    RUN_TEST(test_truncation_reports_needed_length);
    RUN_TEST(test_null_body);
    RUN_TEST(test_profile_names_and_native);
    return UNITY_END();
}
