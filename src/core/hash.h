/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_HASH_H
#define CADV_CORE_HASH_H

#include <stddef.h>
#include <stdint.h>

#define CADV_FNV64_OFFSET 0xcbf29ce484222325ULL

/* FNV-1a 64-bit. Used for state hashes (determinism, desync detection). */
uint64_t cadv_hash_bytes(uint64_t h, const void *data, size_t len);
uint64_t cadv_hash_u32(uint64_t h, uint32_t v); /* little-endian canonical */
uint64_t cadv_hash_u64(uint64_t h, uint64_t v);
uint64_t cadv_hash_str(uint64_t h, const char *s);

#endif
