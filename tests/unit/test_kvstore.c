/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <string.h>

#include "core/kvstore.h"
#include "unity.h"

static cadv_kvstore g_kv;
static const char *k_tmp = "test_kv_tmp.sav";

void setUp(void) { cadv_kv_clear(&g_kv); }
void tearDown(void) { remove(k_tmp); }

static void test_set_get_replace_remove(void)
{
    TEST_ASSERT_TRUE(cadv_kv_set(&g_kv, "portal.05.star", "1"));
    TEST_ASSERT_TRUE(cadv_kv_set_int(&g_kv, "portal.05.flakes", 2));
    TEST_ASSERT_EQUAL_STRING("1", cadv_kv_get(&g_kv, "portal.05.star"));
    TEST_ASSERT_EQUAL_INT64(2, cadv_kv_get_int(&g_kv, "portal.05.flakes", -1));
    TEST_ASSERT_TRUE(cadv_kv_set(&g_kv, "portal.05.star", "0"));
    TEST_ASSERT_EQUAL_INT(2, g_kv.count);
    TEST_ASSERT_TRUE(cadv_kv_remove(&g_kv, "portal.05.star"));
    TEST_ASSERT_FALSE(cadv_kv_remove(&g_kv, "portal.05.star"));
    TEST_ASSERT_NULL(cadv_kv_get(&g_kv, "portal.05.star"));
    TEST_ASSERT_EQUAL_INT64(7, cadv_kv_get_int(&g_kv, "missing", 7));
}

static void test_rejects_bad_keys_and_values(void)
{
    char longkey[CADV_KV_KEY + 4];
    char longval[CADV_KV_VAL + 4];
    memset(longkey, 'k', sizeof longkey - 1);
    longkey[sizeof longkey - 1] = '\0';
    memset(longval, 'v', sizeof longval - 1);
    longval[sizeof longval - 1] = '\0';
    TEST_ASSERT_FALSE(cadv_kv_set(&g_kv, "", "x"));
    TEST_ASSERT_FALSE(cadv_kv_set(&g_kv, "has space", "x"));
    TEST_ASSERT_FALSE(cadv_kv_set(&g_kv, "a=b", "x"));
    TEST_ASSERT_FALSE(cadv_kv_set(&g_kv, longkey, "x"));
    TEST_ASSERT_FALSE(cadv_kv_set(&g_kv, "k", longval));
    TEST_ASSERT_FALSE(cadv_kv_set(&g_kv, "k", "line\nbreak"));
    TEST_ASSERT_TRUE(cadv_kv_set(&g_kv, "Az09_.-", "spaces and = are fine"));
}

static void test_capacity_limit(void)
{
    char key[16];
    int i;
    for (i = 0; i < CADV_KV_MAX; ++i) {
        snprintf(key, sizeof key, "k%d", i);
        TEST_ASSERT_TRUE(cadv_kv_set(&g_kv, key, "v"));
    }
    TEST_ASSERT_FALSE(cadv_kv_set(&g_kv, "one.more", "v"));
    TEST_ASSERT_TRUE(cadv_kv_set(&g_kv, "k3", "updated")); /* replacing still works */
}

static void test_get_int_rejects_garbage(void)
{
    cadv_kv_set(&g_kv, "n", "12x");
    cadv_kv_set(&g_kv, "e", "");
    TEST_ASSERT_EQUAL_INT64(-1, cadv_kv_get_int(&g_kv, "n", -1));
    TEST_ASSERT_EQUAL_INT64(-1, cadv_kv_get_int(&g_kv, "e", -1));
}

static void test_serialize_parse_round_trip(void)
{
    char buf[256];
    cadv_kvstore b;
    size_t n;
    cadv_kv_set(&g_kv, "a", "1");
    cadv_kv_set(&g_kv, "name", "Messerjocke");
    g_kv.version = 3;
    n = cadv_kv_serialize(&g_kv, "cadvent-save", buf, sizeof buf);
    TEST_ASSERT_EQUAL_UINT(strlen(buf), n);
    TEST_ASSERT_EQUAL_STRING("cadvent-save 3\na=1\nname=Messerjocke\n", buf);
    TEST_ASSERT_TRUE(cadv_kv_parse(&b, buf, "cadvent-save"));
    TEST_ASSERT_EQUAL_INT(3, b.version);
    TEST_ASSERT_EQUAL_STRING("Messerjocke", cadv_kv_get(&b, "name"));
    /* size query without buffer */
    TEST_ASSERT_EQUAL_UINT(n, cadv_kv_serialize(&g_kv, "cadvent-save", NULL, 0));
    /* truncated buffer still reports full size */
    TEST_ASSERT_EQUAL_UINT(n, cadv_kv_serialize(&g_kv, "cadvent-save", buf, 10));
}

static void test_parse_comments_crlf_and_errors(void)
{
    cadv_kvstore b;
    char huge[400];
    TEST_ASSERT_TRUE(cadv_kv_parse(&b, "m 1\r\n# comment\r\n\r\nx=2\r\n", "m"));
    TEST_ASSERT_EQUAL_STRING("2", cadv_kv_get(&b, "x"));
    TEST_ASSERT_TRUE(cadv_kv_parse(&b, "m 1", "m"));
    TEST_ASSERT_FALSE(cadv_kv_parse(&b, "", "m"));
    TEST_ASSERT_FALSE(cadv_kv_parse(&b, "other 1\n", "m"));
    TEST_ASSERT_FALSE(cadv_kv_parse(&b, "mm 1\n", "m"));
    TEST_ASSERT_FALSE(cadv_kv_parse(&b, "m x\n", "m"));
    TEST_ASSERT_FALSE(cadv_kv_parse(&b, "m 0\n", "m"));
    TEST_ASSERT_FALSE(cadv_kv_parse(&b, "m 1\nnoequals\n", "m"));
    TEST_ASSERT_FALSE(cadv_kv_parse(&b, "m 1\nbad key=1\n", "m"));
    memset(huge, 'a', sizeof huge - 1);
    huge[0] = 'm';
    huge[1] = ' ';
    huge[2] = '1';
    huge[3] = '\n';
    huge[sizeof huge - 1] = '\0';
    TEST_ASSERT_FALSE(cadv_kv_parse(&b, huge, "m"));
}

static void test_file_round_trip_and_failures(void)
{
    cadv_kvstore b;
    FILE *f;
    cadv_kv_set(&g_kv, "stars", "23");
    TEST_ASSERT_TRUE(cadv_kv_save_file(&g_kv, k_tmp, "cadvent-save"));
    TEST_ASSERT_TRUE(cadv_kv_load_file(&b, k_tmp, "cadvent-save"));
    TEST_ASSERT_EQUAL_INT64(23, cadv_kv_get_int(&b, "stars", 0));
    TEST_ASSERT_FALSE(cadv_kv_load_file(&b, k_tmp, "other-magic"));
    TEST_ASSERT_FALSE(cadv_kv_load_file(&b, "no-such-file.sav", "cadvent-save"));
    TEST_ASSERT_FALSE(cadv_kv_save_file(&g_kv, "/nonexistent-dir/x.sav", "cadvent-save"));
    f = fopen(k_tmp, "wb");
    fclose(f);
    TEST_ASSERT_FALSE(cadv_kv_load_file(&b, k_tmp, "cadvent-save")); /* empty file */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_set_get_replace_remove);
    RUN_TEST(test_rejects_bad_keys_and_values);
    RUN_TEST(test_capacity_limit);
    RUN_TEST(test_get_int_rejects_garbage);
    RUN_TEST(test_serialize_parse_round_trip);
    RUN_TEST(test_parse_comments_crlf_and_errors);
    RUN_TEST(test_file_round_trip_and_failures);
    return UNITY_END();
}
