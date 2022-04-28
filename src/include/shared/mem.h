#pragma once

#include <shared/types.h>
#include <string.h>

/*
 *  We still rely on string.h
static inline void memcpy(void *restrict dst, const void *restrict src, u64 size) {
    for (; size; size--) {
        *dst++ = *src++;
    }
}
*/
