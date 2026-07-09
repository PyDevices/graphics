/*
 * Font and text rendering.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "gfx_core.h"
#include "gfx_shapes.h"

gfx_area_t gfx_font_text8(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col);

#endif
