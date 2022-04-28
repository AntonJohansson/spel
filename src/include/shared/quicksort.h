#pragma once

#include <shared/types.h>
#include <shared/mem.h>

typedef bool QuicksortCompareFunc(const void *a, const void *b);

static inline u64 hoare_partition(u8 *array, u64 element_size, u64 low, u64 high, QuicksortCompareFunc *less_than) {
    u64 pivot_index = (low + high)/2;
    u8 pivot[element_size];
    memcpy(pivot, array + pivot_index*element_size, element_size);

    i64 left_index = (i64)low - 1;
    i64 right_index = (i64)high + 1;

    u8 temp[element_size];

    while (true) {
        do {
            left_index++;
        } while (less_than(array + left_index*element_size, pivot));

        do {
            right_index--;
        } while (memcmp(array + right_index*element_size, pivot, element_size) != 0 && !less_than(array + right_index*element_size, pivot));

        if (left_index < right_index) {
            memcpy(temp, array + left_index*element_size, element_size);
            memcpy(array + left_index*element_size, array + right_index*element_size, element_size);
            memcpy(array + right_index*element_size, temp, element_size);
        } else {
            return right_index;
        }
    }
}

static inline void quicksort_recurse(u8 *array, u64 element_size, u64 low, u64 high, QuicksortCompareFunc *less_than) {
    if (low >= high) {
        return;
    }

    u64 partition = hoare_partition(array, element_size, low, high, less_than);
    quicksort_recurse(array, element_size, low, partition, less_than);
    quicksort_recurse(array, element_size, partition+1, high, less_than);
}

static inline void quicksort(void *array, u64 element_size, u64 array_size, QuicksortCompareFunc *less_than) {
    quicksort_recurse(array, element_size, 0, array_size-1, less_than);
}
