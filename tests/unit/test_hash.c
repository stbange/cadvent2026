/* SPDX-License-Identifier: MIT */
#include "core/hash.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* Published FNV-1a 64-bit test vectors. */
static void test_fnv1a_reference_vectors(void)
{
    TEST_ASSERT_EQUAL_HEX64(0xcbf29ce484222325ULL, cadv_hash_str(CADV_FNV64_OFFSET, ""));
    TEST_ASSERT_EQUAL_HEX64(0xaf63dc4c8601ec8cULL, cadv_hash_str(CADV_FNV64_OFFSET, "a"));
    TEST_ASSERT_EQUAL_HEX64(0x85944171f73967e8ULL, cadv_hash_str(CADV_FNV64_OFFSET, "foobar"));
}

static void test_bytes_and_str_agree(void)
{
    TEST_ASSERT_EQUAL_HEX64(cadv_hash_str(CADV_FNV64_OFFSET, "foobar"),
                            cadv_hash_bytes(CADV_FNV64_OFFSET, "foobar", 6));
}

static void test_integers_are_little_endian_canonical(void)
{
    static const unsigned char le32[4] = {0x78, 0x56, 0x34, 0x12};
    static const unsigned char le64[8] = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    TEST_ASSERT_EQUAL_HEX64(cadv_hash_bytes(CADV_FNV64_OFFSET, le32, 4),
                            cadv_hash_u32(CADV_FNV64_OFFSET, 0x12345678u));
    TEST_ASSERT_EQUAL_HEX64(cadv_hash_bytes(CADV_FNV64_OFFSET, le64, 8),
                            cadv_hash_u64(CADV_FNV64_OFFSET, 0x0102030405060708ULL));
}

static void test_chaining_differs_by_order(void)
{
    uint64_t ab = cadv_hash_u32(cadv_hash_u32(CADV_FNV64_OFFSET, 1u), 2u);
    uint64_t ba = cadv_hash_u32(cadv_hash_u32(CADV_FNV64_OFFSET, 2u), 1u);
    TEST_ASSERT_TRUE(ab != ba);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fnv1a_reference_vectors);
    RUN_TEST(test_bytes_and_str_agree);
    RUN_TEST(test_integers_are_little_endian_canonical);
    RUN_TEST(test_chaining_differs_by_order);
    return UNITY_END();
}
