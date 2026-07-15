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
gfx_area_t gfx_shapes_ellipse(const gfx_canvas_t *canvas, int cx, int cy, int xradius, int yradius, int col, int fill, int mask_part, int w, int h);
int gfx_intersect_rect(int x, int y, int w, int h, const gfx_area_t *clip, gfx_area_t *out);
void gfx_crop_rgb565_buffer(const void *src, int src_w, int src_x, int src_y, int crop_w, int crop_h, void *dst);
gfx_area_t gfx_shapes_fill(const gfx_canvas_t *canvas, int c);

int gfx_shapes_poly_int_from_buffer(const void *buf, size_t len, size_t itemsize, const char *fmt, size_t index, int *out);
gfx_area_t gfx_shapes_poly(const gfx_canvas_t *canvas, int x, int y, const void *coords, size_t coords_len, size_t itemsize, const char *fmt, int col, int fill);
gfx_area_t gfx_shapes_blit(const gfx_canvas_t *canvas, const gfx_fb_t *source, int x, int y, int key, const gfx_fb_t *palette);
gfx_area_t gfx_shapes_circle(const gfx_canvas_t *canvas, int x0, int y0, int r, int c, int fill);
gfx_area_t gfx_shapes_round_rect(const gfx_canvas_t *canvas, int x0, int y0, int w, int h, int r, int c, int fill);
gfx_area_t gfx_shapes_triangle(const gfx_canvas_t *canvas, int x0, int y0, int x1, int y1, int x2, int y2, int c, int fill);
gfx_area_t gfx_shapes_arc(const gfx_canvas_t *canvas, int x, int y, int r, float a0, float a1, int c);
gfx_area_t gfx_shapes_gradient_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c1, int c2, int vertical);
gfx_area_t gfx_shapes_blit_rect(const gfx_canvas_t *canvas, const void *buf, int x, int y, int w, int h, int bpp);
gfx_area_t gfx_shapes_blit_transparent(const gfx_canvas_t *canvas, const void *buf, int x, int y, int w, int h, int key, int bpp);
gfx_area_t gfx_shapes_polygon(const gfx_canvas_t *canvas, const int *points, size_t n_points, int x, int y, int color, float angle, int center_x, int center_y);

#endif
