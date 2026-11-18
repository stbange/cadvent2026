/* SPDX-License-Identifier: MIT */
#include "core/hash.h"

#define FNV64_PRIME 0x100000001b3ULL

uint64_t cadv_hash_bytes(uint64_t h, const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    size_t i;
    for (i = 0; i < len; ++i) {
        h ^= p[i];
        h *= FNV64_PRIME;
    }
    return h;
}

uint64_t cadv_hash_u32(uint64_t h, uint32_t v)
{
    unsigned char b[4];
    b[0] = (unsigned char)(v & 0xffu);
    b[1] = (unsigned char)((v >> 8) & 0xffu);
    b[2] = (unsigned char)((v >> 16) & 0xffu);
    b[3] = (unsigned char)((v >> 24) & 0xffu);
    return cadv_hash_bytes(h, b, sizeof b);
}

uint64_t cadv_hash_u64(uint64_t h, uint64_t v)
{
    h = cadv_hash_u32(h, (uint32_t)(v & 0xffffffffu));
    return cadv_hash_u32(h, (uint32_t)(v >> 32));
}

uint64_t cadv_hash_str(uint64_t h, const char *s)
{
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= FNV64_PRIME;
    }
    return h;
}
