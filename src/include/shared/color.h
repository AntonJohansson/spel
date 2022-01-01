#pragma once

#include <shared/math.h>
#include <math.h>

typedef struct ColorRGB {
    f32 r, g, b;
} ColorRGB;

typedef struct ColorHSL {
    f32 h, s, l;
} ColorHSL;

#define RGB(r,g,b) (ColorRGB) {r, g, b}
#define HSL(h,s,l) (ColorHSL) {h, s, l}

static inline f64 hslF(ColorHSL col, int n) {
    f64 k = fmod((n + col.h/30.0f), 12.0f);
    f64 a = col.s*fmin(col.l, 1.0f-col.l);
    return col.l - a*fmax(fmin(fmin(k-3.0f, 9.0f-k),1.0f),-1.0f);
}

static inline ColorRGB convertHSLToRGB(ColorHSL col) {
    return RGB(hslF(col, 0), hslF(col, 8), hslF(col, 4));
}

static inline void clampRGB(ColorRGB *col) {
    col->r = CLAMP(col->r, 0.0f, 1.0f);
    col->g = CLAMP(col->g, 0.0f, 1.0f);
    col->b = CLAMP(col->b, 0.0f, 1.0f);
}

static inline void clampHSL(ColorHSL *col) {
    col->h = CLAMP(col->h, 0.0f, 1.0f);
    col->s = CLAMP(col->s, 0.0f, 1.0f);
    col->l = CLAMP(col->l, 0.0f, 1.0f);
}

static inline void wrapRGB(ColorRGB *col) {
    col->r = fmod(col->r, 1.0f);
    col->g = fmod(col->g, 1.0f);
    col->b = fmod(col->b, 1.0f);
}

static inline void wrapHSL(ColorHSL *col) {
    col->h = fmod(col->h, 360.0f);
    col->s = fmod(col->s, 1.0f);
    col->l = fmod(col->l, 1.0f);
}

static inline void colorRGBAssignToArray(f32 *arr, ColorRGB col) {
    arr[0] = col.r;
    arr[1] = col.g;
    arr[2] = col.b;
}

static inline void colorHSLAssignToArray(f32 *arr, ColorHSL col) {
    arr[0] = col.h;
    arr[1] = col.s;
    arr[2] = col.l;
}
