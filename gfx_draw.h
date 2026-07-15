/*
 * Draw type: canvas + clip stack.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_DRAW_H
#define GFX_DRAW_H

#include "gfx_core.h"
#include "gfx_shapes.h"

#define GFX_DRAW_CLIP_STACK_MAX 8

typedef struct gfx_draw {
    gfx_canvas_t canvas;
    gfx_clipped_canvas_t clipped;
    int use_clipped;
    gfx_area_t clip_stack[GFX_DRAW_CLIP_STACK_MAX];
    int clip_depth;
} gfx_draw_t;

void gfx_draw_init(gfx_draw_t *draw, const gfx_canvas_t *canvas);
const gfx_canvas_t *gfx_draw_target(gfx_draw_t *draw);
gfx_area_t gfx_draw_effective_clip(const gfx_draw_t *draw);
void gfx_draw_push_clip(gfx_draw_t *draw, const gfx_area_t *clip);
void gfx_draw_pop_clip(gfx_draw_t *draw);

#endif
