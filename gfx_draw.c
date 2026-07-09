/*
 * Draw type: canvas + clip stack.
 * SPDX-License-Identifier: MIT
 */

#include "gfx_draw.h"

void gfx_draw_init(gfx_draw_t *draw, const gfx_canvas_t *canvas) {
    draw->canvas = *canvas;
    draw->clip_depth = 0;
    draw->use_clipped = 0;
}

static gfx_area_t effective_clip(const gfx_draw_t *draw) {
    if (draw->clip_depth <= 0) {
        return gfx_area_from_rect(0, 0, draw->canvas.width, draw->canvas.height);
    }
    gfx_area_t clip = draw->clip_stack[0];
    for (int i = 1; i < draw->clip_depth; i++) {
        clip = gfx_area_clip(&clip, &draw->clip_stack[i]);
    }
    gfx_area_t bounds = gfx_area_from_rect(0, 0, draw->canvas.width, draw->canvas.height);
    return gfx_area_clip(&clip, &bounds);
}

const gfx_canvas_t *gfx_draw_target(gfx_draw_t *draw) {
    if (draw->clip_depth <= 0) {
        draw->use_clipped = 0;
        return &draw->canvas;
    }
    gfx_area_t clip = effective_clip(draw);
    if (clip.w <= 0 || clip.h <= 0) {
        draw->use_clipped = 0;
        return &draw->canvas;
    }
    gfx_clipped_canvas_init(&draw->clipped, &draw->canvas, &clip);
    draw->use_clipped = 1;
    return &draw->clipped.base;
}

void gfx_draw_push_clip(gfx_draw_t *draw, const gfx_area_t *clip) {
    if (draw->clip_depth < GFX_DRAW_CLIP_STACK_MAX) {
        draw->clip_stack[draw->clip_depth++] = *clip;
    }
}

void gfx_draw_pop_clip(gfx_draw_t *draw) {
    if (draw->clip_depth > 0) {
        draw->clip_depth--;
    }
}
