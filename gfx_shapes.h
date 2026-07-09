/*
 * Shape drawing algorithms via gfx_canvas_t.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_SHAPES_H
#define GFX_SHAPES_H

#include <stddef.h>
#include <stdint.h>

#include "gfx_core.h"
#include "gfx_framebuffer.h"

typedef struct {
    gfx_canvas_t base;
    const gfx_canvas_t *parent;
    gfx_area_t clip;
} gfx_clipped_canvas_t;

void gfx_clipped_canvas_init(gfx_clipped_canvas_t *cc, const gfx_canvas_t *parent, const gfx_area_t *clip);

gfx_area_t gfx_shapes_pixel(const gfx_canvas_t *canvas, int x, int y, int c);
gfx_area_t gfx_shapes_hline(const gfx_canvas_t *canvas, int x, int y, int w, int c);
gfx_area_t gfx_shapes_vline(const gfx_canvas_t *canvas, int x, int y, int h, int c);
gfx_area_t gfx_shapes_fill_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c);
gfx_area_t gfx_shapes_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c, int fill);
gfx_area_t gfx_shapes_line(const gfx_canvas_t *canvas, int x1, int y1, int x2, int y2, int c);
gfx_area_t gfx_shapes_ellipse(const gfx_canvas_t *canvas, int cx, int cy, int xradius, int yradius, int col, int fill, int mask_part);
gfx_area_t gfx_shapes_fill(const gfx_canvas_t *canvas, int c);

int gfx_shapes_poly_int_from_buffer(const void *buf, size_t len, size_t itemsize, const char *fmt, size_t index, int *out);
gfx_area_t gfx_shapes_poly(const gfx_canvas_t *canvas, int x, int y, const void *coords, size_t coords_len, size_t itemsize, const char *fmt, int col, int fill);
gfx_area_t gfx_shapes_blit(const gfx_canvas_t *canvas, const gfx_fb_t *source, int x, int y, int key, const gfx_fb_t *palette);

#endif
