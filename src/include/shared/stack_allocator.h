#pragma once

#include <shared/types.h>

typedef struct StackAllocator {
    void *memory;
    u64 memory_size;
    u64 top;
} StackAllocator;

static inline StackAllocator *stackCreate(void *memory, u64 memory_size) {
    /* 
     * NOTE(anjo): We have no way of reporing an error
     * if size is less than sizeof(stack)
     */
    StackAlloctor *stack = memory;
    stack->memory = memory;
    stack->memory_size = memory_size - sizeof(StackAllocator);
    stack->top = 0;
    return stack;
}

static inline void stackClear(StackAllocator *stack) {
    stack->top = 0;
}

static inline void *stackAllocate(StackAllocator *stack, u64 size) {
    /* 
     * NOTE(anjo): Likewise we have no way to check for
     * errors here
     */
    u8 *ptr = (u8 *) stack->memory + stack->top;
    stack->top += size;
    return ptr;
}
