/*
 * Extended MicroPython bindings header.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_BINDINGS_MP_H
#define GFX_BINDINGS_MP_H

#include "py/obj.h"
#include "gfx_bmp565.h"
#include "gfx_draw.h"
#include "gfx_framebuffer.h"

typedef struct _mp_py_canvas_ctx_t {
    mp_obj_t obj;
} mp_py_canvas_ctx_t;

typedef struct _mp_obj_framebuf_t {
    mp_obj_base_t base;
    mp_obj_t buf_obj;
    gfx_fb_t fb;
    gfx_canvas_t canvas;
} mp_obj_framebuf_t;

typedef struct _mp_obj_draw_t {
    mp_obj_base_t base;
    mp_obj_t canvas_obj;
    gfx_draw_t draw;
    mp_py_canvas_ctx_t py_ctx;
} mp_obj_draw_t;

typedef enum {
    MP_CANVAS_NATIVE_FB = 0,
    MP_CANVAS_PY_OBJ,
    MP_CANVAS_BMP565_BUF,
} mp_canvas_kind_t;

typedef struct _mp_canvas_slot_t {
    mp_canvas_kind_t kind;
    union {
        mp_obj_framebuf_t fb;
        mp_py_canvas_ctx_t py;
        struct {
            gfx_fb_t gfx_fb;
        } bmp;
    } u;
    gfx_canvas_t canvas;
} mp_canvas_slot_t;

mp_obj_t framebuf_make_new_helper(size_t n_args, const mp_obj_t *args_in, unsigned int buf_flags, mp_obj_framebuf_t *o);

extern const mp_obj_type_t mp_type_framebuf;
extern const mp_obj_type_t mp_type_draw;
extern const mp_obj_type_t mp_type_font;
extern const mp_obj_type_t mp_type_bmp565;

mp_obj_t gfx_mp_draw_clip(mp_obj_t draw_obj, const gfx_area_t *area);

typedef struct _mp_obj_bmp565_t {
    mp_obj_base_t base;
    gfx_bmp565_t bmp;
    mp_obj_t buf_obj;
    mp_obj_t filename_obj;
} mp_obj_bmp565_t;

#endif
