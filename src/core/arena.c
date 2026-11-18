/* SPDX-License-Identifier: MIT */
#include "core/arena.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CADV_ARENA_DEFAULT_ALIGN 16u

bool cadv_arena_init(cadv_arena *a, size_t capacity)
{
    memset(a, 0, sizeof *a);
    a->base = (unsigned char *)malloc(capacity ? capacity : 1);
    if (!a->base) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return false; /* GCOVR_EXCL_LINE: allocation failure not reproducible in tests */
    }
    a->capacity = capacity;
    a->owns = true;
    return true;
}

void cadv_arena_init_buffer(cadv_arena *a, void *buffer, size_t capacity)
{
    memset(a, 0, sizeof *a);
    a->base = (unsigned char *)buffer;
    a->capacity = capacity;
}

void cadv_arena_free(cadv_arena *a)
{
    if (a->owns) {
        free(a->base);
    }
    memset(a, 0, sizeof *a);
}

void *cadv_arena_alloc(cadv_arena *a, size_t size, size_t align)
{
    uintptr_t start;
    uintptr_t aligned;
    size_t offset;
    if (align == 0) {
        align = CADV_ARENA_DEFAULT_ALIGN;
    }
    if ((align & (align - 1)) != 0 || !a->base) {
        return NULL;
    }
    start = (uintptr_t)a->base + a->used;
    aligned = (start + (align - 1)) & ~(uintptr_t)(align - 1);
    offset = (size_t)(aligned - (uintptr_t)a->base);
    if (offset > a->capacity || size > a->capacity - offset) {
        return NULL;
    }
    a->used = offset + size;
    if (a->used > a->peak) {
        a->peak = a->used;
    }
    memset(a->base + offset, 0, size);
    return a->base + offset;
}

size_t cadv_arena_mark(const cadv_arena *a) { return a->used; }

void cadv_arena_reset_to(cadv_arena *a, size_t mark)
{
    if (mark <= a->used) {
        a->used = mark;
    }
}

void cadv_arena_reset(cadv_arena *a) { a->used = 0; }
