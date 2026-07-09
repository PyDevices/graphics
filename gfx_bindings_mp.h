/*
 * Extended MicroPython bindings header.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_BINDINGS_MP_H
#define GFX_BINDINGS_MP_H

#include "py/obj.h"
#include "gfx_bmp565.h"

typedef struct _mp_obj_framebuf_t {
    mp_obj_base_t base;
    mp_obj_t buf_obj;
    gfx_fb_t fb;
    gfx_canvas_t canvas;
} mp_obj_framebuf_t;

mp_obj_t framebuf_make_new_helper(size_t n_args, const mp_obj_t *args_in, unsigned int buf_flags, mp_obj_framebuf_t *o);

extern const mp_obj_type_t mp_type_framebuf;
extern const mp_obj_type_t mp_type_draw;
extern const mp_obj_type_t mp_type_font;
extern const mp_obj_type_t mp_type_bmp565;

typedef struct _mp_obj_bmp565_t {
    mp_obj_base_t base;
    gfx_bmp565_t bmp;
    mp_obj_t buf_obj;
} mp_obj_bmp565_t;

#endif
