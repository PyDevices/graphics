/*
 * Font and text rendering.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_FONT_H
#define GFX_FONT_H

#include <stddef.h>
#include <stdint.h>

#include "gfx_core.h"
#include "gfx_shapes.h"

typedef struct gfx_font {
    const uint8_t *data;
    size_t data_len;
    int height;
    int width;
    int owns_data;
} gfx_font_t;

void gfx_font_init_default(gfx_font_t *font, int height);
void gfx_font_init_from_data(gfx_font_t *font, const uint8_t *data, size_t len, int height);
void gfx_font_deinit(gfx_font_t *font);
int gfx_font_export(const gfx_font_t *font, const char *filename);

gfx_area_t gfx_font_text(const gfx_canvas_t *canvas, const gfx_font_t *font, const char *str, int x0, int y0, int col, int scale, int inverted);
gfx_area_t gfx_font_draw_char(const gfx_canvas_t *canvas, const gfx_font_t *font, unsigned char ch, int x, int y, int color, int scale, int inverted);
int gfx_font_text_width(const gfx_font_t *font, const char *str, int scale);
gfx_area_t gfx_font_text8(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col);
gfx_area_t gfx_font_text14(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col);
gfx_area_t gfx_font_text16(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col);

#endif
