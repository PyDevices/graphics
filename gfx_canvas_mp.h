/*
 * MicroPython canvas resolution (native FrameBuffer + Python duck typing).
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_CANVAS_MP_H
#define GFX_CANVAS_MP_H

#include "py/obj.h"
#include "gfx_core.h"
#include "gfx_framebuffer.h"
#include "gfx_bindings_mp.h"

typedef struct _mp_py_canvas_ctx_t {
    mp_obj_t obj;
} mp_py_canvas_ctx_t;

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

bool mp_canvas_resolve(mp_obj_t target, mp_canvas_slot_t *slot);

#endif
