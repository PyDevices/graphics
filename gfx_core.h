/*
 * Shared graphics core types (framebuf formats + Area geometry).
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_CORE_H
#define GFX_CORE_H

#include <stdbool.h>
#include <stdint.h>

/* File I/O backends.
 *
 * The graphics image/font codecs are pure and always compiled; only the code
 * that actually touches a filesystem is gated by these flags:
 *
 *   GFX_ENABLE_HOST_STDIO — read/write through the host C library (fopen /
 *     fread / fwrite). Only meaningful where those hit a real filesystem:
 *     desktop and emscripten. Bare-metal cross toolchains (arm-none-eabi,
 *     xtensa, riscv, ...) either lack these symbols or they don't reach the
 *     board's flash, so this defaults off there.
 *
 *   GFX_ENABLE_MP_VFS — read/write through MicroPython's VFS (the same
 *     filesystem that Python `open()` sees). This is what lets image/font
 *     loaders reach on-device storage on esp32, rp2, etc. It is defined by the
 *     MicroPython binding layer (see gfx_bindings_mp.h), keyed off MICROPY_VFS,
 *     so it is not set here.
 *
 * Define GFX_ENABLE_HOST_STDIO before including this header to force it. */
#ifndef GFX_ENABLE_HOST_STDIO
#if defined(__unix__) || defined(__APPLE__) || defined(_WIN32) || defined(__EMSCRIPTEN__)
#define GFX_ENABLE_HOST_STDIO 1
#else
#define GFX_ENABLE_HOST_STDIO 0
#endif
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Format IDs — match MicroPython framebuf + pydisplay RGB888 extension */
#define GFX_MVLSB    0
#define GFX_RGB565   1
#define GFX_GS4_HMSB 2
#define GFX_MHLSB    3
#define GFX_MHMSB    4
#define GFX_GS2_HMSB 5
#define GFX_GS8      6
#define GFX_RGB888   7

#define GFX_MONO_VLSB GFX_MVLSB
#define GFX_MONO_HLSB GFX_MHLSB
#define GFX_MONO_HMSB GFX_MHMSB

typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} gfx_area_t;

static inline void gfx_area_init(gfx_area_t *a, int32_t x, int32_t y, int32_t w, int32_t h) {
    a->x = x;
    a->y = y;
    a->w = w;
    a->h = h;
}

static inline bool gfx_area_contains_point(const gfx_area_t *a, int32_t px, int32_t py) {
    return a->x <= px && px < a->x + a->w && a->y <= py && py < a->y + a->h;
}

static inline bool gfx_area_contains_area(const gfx_area_t *a, const gfx_area_t *b) {
    return a->x <= b->x && a->y <= b->y
        && a->x + a->w >= b->x + b->w && a->y + a->h >= b->y + b->h;
}

static inline bool gfx_area_intersects(const gfx_area_t *a, const gfx_area_t *b) {
    if (a->x + a->w <= b->x || b->x + b->w <= a->x) {
        return false;
    }
    return !(a->y + a->h <= b->y || b->y + b->h <= a->y);
}

static inline bool gfx_area_touches_or_intersects(const gfx_area_t *a, const gfx_area_t *b) {
    if (a->x + a->w < b->x || b->x + b->w < a->x) {
        return false;
    }
    return !(a->y + a->h < b->y || b->y + b->h < a->y);
}

static inline gfx_area_t gfx_area_shift(const gfx_area_t *a, int32_t dx, int32_t dy) {
    gfx_area_t out = *a;
    out.x += dx;
    out.y += dy;
    return out;
}

static inline gfx_area_t gfx_area_clip(const gfx_area_t *a, const gfx_area_t *b) {
    int32_t x = a->x > b->x ? a->x : b->x;
    int32_t y = a->y > b->y ? a->y : b->y;
    int32_t x2 = (a->x + a->w < b->x + b->w) ? a->x + a->w : b->x + b->w;
    int32_t y2 = (a->y + a->h < b->y + b->h) ? a->y + a->h : b->y + b->h;
    gfx_area_t out;
    gfx_area_init(&out, x, y, x2 - x, y2 - y);
    return out;
}

static inline gfx_area_t gfx_area_union(const gfx_area_t *a, const gfx_area_t *b) {
    int32_t x0 = a->x < b->x ? a->x : b->x;
    int32_t y0 = a->y < b->y ? a->y : b->y;
    int32_t x1 = (a->x + a->w > b->x + b->w) ? a->x + a->w : b->x + b->w;
    int32_t y1 = (a->y + a->h > b->y + b->h) ? a->y + a->h : b->y + b->h;
    gfx_area_t out;
    gfx_area_init(&out, x0, y0, x1 - x0, y1 - y0);
    return out;
}

static inline gfx_area_t gfx_area_offset(const gfx_area_t *a, int32_t d1, int32_t d2, int32_t d3, int32_t d4) {
    gfx_area_t out;
    gfx_area_init(&out, a->x - d1, a->y - d2, a->w + d1 + d3, a->h + d2 + d4);
    return out;
}

static inline gfx_area_t gfx_area_inset(const gfx_area_t *a, int32_t d1, int32_t d2, int32_t d3, int32_t d4) {
    gfx_area_t out;
    gfx_area_init(&out, a->x + d1, a->y + d2, a->w - d1 - d3, a->h - d2 - d4);
    return out;
}

static inline gfx_area_t gfx_area_from_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
    gfx_area_t out;
    gfx_area_init(&out, x, y, w, h);
    return out;
}

/* Draw-target vtable for shapes */
typedef struct gfx_canvas {
    void *ctx;
    int width;
    int height;
    int (*pixel)(void *ctx, int x, int y, int c, int set);
    void (*hline)(void *ctx, int x, int y, int w, int c);
    void (*vline)(void *ctx, int x, int y, int h, int c);
    void (*fill_rect)(void *ctx, int x, int y, int w, int h, int c);
} gfx_canvas_t;

#endif
