#pragma once

#include "types.h"

#define MAX(a, b) \
    ((a >= b) ? a : b)

#define MIN(a, b) \
    ((a <= b) ? a : b)

#define CLAMP(value, min, max) \
    MAX(MIN(max, value), min)

#define VEC2(x,y) (Vec2) {x,y}

typedef struct Vec2 {
    f32 x, y;
} Vec2;

static inline Vec2 v2Add(Vec2 a, Vec2 b) {
    return VEC2(a.x + b.x, a.y + b.y);
}

static inline Vec2 v2Sub(Vec2 a, Vec2 b) {
    return VEC2(a.x - b.x, a.y - b.y);
}

static inline Vec2 v2Scale(f32 f, Vec2 v) {
    return VEC2(f*v.x, f*v.y);
}

static inline void v2AssignToArray(f32 *arr, Vec2 v) {
    arr[0] = v.x;
    arr[1] = v.y;
}
