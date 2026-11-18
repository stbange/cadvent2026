/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_ARENA_H
#define CADV_CORE_ARENA_H

#include <stdbool.h>
#include <stddef.h>

/* Linear allocator over one fixed block. A portal's whole state lives in one
 * arena, so leaving a portal frees everything at once. */
typedef struct cadv_arena {
    unsigned char *base;
    size_t capacity;
    size_t used;
    size_t peak;
    bool owns;
} cadv_arena;

/* Allocates the backing block with malloc. Returns false on failure. */
bool cadv_arena_init(cadv_arena *a, size_t capacity);
/* Uses caller memory; cadv_arena_free will not release it. */
void cadv_arena_init_buffer(cadv_arena *a, void *buffer, size_t capacity);
void cadv_arena_free(cadv_arena *a);

/* align must be a power of two (0 means default alignment). Memory is
 * zeroed. Returns NULL when the arena is exhausted. */
void *cadv_arena_alloc(cadv_arena *a, size_t size, size_t align);
#define CADV_ARENA_NEW(a, T) ((T *)cadv_arena_alloc((a), sizeof(T), _Alignof(T)))

size_t cadv_arena_mark(const cadv_arena *a);
void cadv_arena_reset_to(cadv_arena *a, size_t mark);
void cadv_arena_reset(cadv_arena *a);

#endif
