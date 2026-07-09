/*
 * Framebuffer pixel format operations.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_FRAMEBUFFER_H
#define GFX_FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>

#include "gfx_core.h"

typedef struct gfx_fb {
    void *buf;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint8_t format;
} gfx_fb_t;

int gfx_fb_validate_buffer(size_t buf_len, int width, int height, int format, int stride);
int gfx_fb_color_depth(int format);

void gfx_fb_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col);
uint32_t gfx_fb_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y);
void gfx_fb_setpixel_checked(const gfx_fb_t *fb, int x, int y, int col, int mask);
void gfx_fb_fill_rect_raw(const gfx_fb_t *fb, int x, int y, int w, int h, uint32_t col);
gfx_area_t gfx_fb_fill_rect(const gfx_fb_t *fb, int x, int y, int w, int h, uint32_t col);
gfx_area_t gfx_fb_fill(const gfx_fb_t *fb, uint32_t col);

void gfx_fb_canvas_init(gfx_canvas_t *canvas, const gfx_fb_t *fb);
void gfx_fb_scroll(gfx_fb_t *fb, int xstep, int ystep);

#endif
