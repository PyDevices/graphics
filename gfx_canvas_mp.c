/*
 * MicroPython canvas resolution (native FrameBuffer + Python duck typing).
 * SPDX-License-Identifier: MIT
 */

#include "gfx_canvas_mp.h"
#include "gfx_clip_mp.h"

#include "py/runtime.h"
#include "py/misc.h"

extern const mp_obj_type_t mp_type_framebuf;
extern const mp_obj_type_t mp_type_bmp565;

static bool mp_get_native_framebuf(mp_obj_t obj, mp_obj_framebuf_t *out) {
    mp_obj_t native = mp_obj_cast_to_native_base(obj, MP_OBJ_FROM_PTR(&mp_type_framebuf));
    if (native == MP_OBJ_NULL) {
        return false;
    }
    *out = *(mp_obj_framebuf_t *)MP_OBJ_TO_PTR(native);
    return true;
}

static bool mp_obj_has_method(mp_obj_t obj, qstr name) {
    mp_obj_t dest[2];
    mp_load_method_maybe(obj, name, dest);
    return dest[0] != MP_OBJ_NULL;
}

static mp_int_t mp_obj_attr_int(mp_obj_t obj, qstr name) {
    return mp_obj_get_int(mp_load_attr(obj, name));
}

static void py_canvas_call_method(mp_obj_t obj, qstr name, size_t n_args, mp_obj_t *args) {
    mp_obj_t method[10];
    mp_load_method(obj, name, method);
    for (size_t i = 0; i < n_args; i++) {
        method[2 + i] = args[i];
    }
    mp_call_method_n_kw(n_args, 0, method);
}

static int py_canvas_pixel(void *ctx, int x, int y, int c, int set) {
    mp_py_canvas_ctx_t *pc = (mp_py_canvas_ctx_t *)ctx;
    if (!set) {
        return 0;
    }
    mp_obj_t args[3] = {
        mp_obj_new_int(x),
        mp_obj_new_int(y),
        mp_obj_new_int(c),
    };
    py_canvas_call_method(pc->obj, MP_QSTR_pixel, 3, args);
    return 0;
}

static void py_canvas_fill_rect(void *ctx, int x, int y, int w, int h, int c) {
    mp_py_canvas_ctx_t *pc = (mp_py_canvas_ctx_t *)ctx;
    mp_obj_t args[5] = {
        mp_obj_new_int(x),
        mp_obj_new_int(y),
        mp_obj_new_int(w),
        mp_obj_new_int(h),
        mp_obj_new_int(c),
    };
    if (mp_obj_has_method(pc->obj, MP_QSTR_fill_rect)) {
        py_canvas_call_method(pc->obj, MP_QSTR_fill_rect, 5, args);
        return;
    }
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            py_canvas_pixel(pc, i, j, c, 1);
        }
    }
}

static void py_canvas_hline(void *ctx, int x, int y, int w, int c) {
    mp_py_canvas_ctx_t *pc = (mp_py_canvas_ctx_t *)ctx;
    if (mp_obj_has_method(pc->obj, MP_QSTR_hline)) {
        mp_obj_t args[4] = {
            mp_obj_new_int(x),
            mp_obj_new_int(y),
            mp_obj_new_int(w),
            mp_obj_new_int(c),
        };
        py_canvas_call_method(pc->obj, MP_QSTR_hline, 4, args);
        return;
    }
    py_canvas_fill_rect(ctx, x, y, w, 1, c);
}

static void py_canvas_vline(void *ctx, int x, int y, int h, int c) {
    mp_py_canvas_ctx_t *pc = (mp_py_canvas_ctx_t *)ctx;
    if (mp_obj_has_method(pc->obj, MP_QSTR_vline)) {
        mp_obj_t args[4] = {
            mp_obj_new_int(x),
            mp_obj_new_int(y),
            mp_obj_new_int(h),
            mp_obj_new_int(c),
        };
        py_canvas_call_method(pc->obj, MP_QSTR_vline, 4, args);
        return;
    }
    py_canvas_fill_rect(ctx, x, y, 1, h, c);
}

static bool mp_init_py_canvas(mp_obj_t target, mp_canvas_slot_t *slot) {
    if (!mp_obj_has_method(target, MP_QSTR_fill_rect) && !mp_obj_has_method(target, MP_QSTR_pixel)) {
        return false;
    }
    mp_int_t width = mp_obj_attr_int(target, MP_QSTR_width);
    mp_int_t height = mp_obj_attr_int(target, MP_QSTR_height);
    slot->kind = MP_CANVAS_PY_OBJ;
    slot->u.py.obj = target;
    slot->canvas.ctx = &slot->u.py;
    slot->canvas.width = width;
    slot->canvas.height = height;
    slot->canvas.pixel = py_canvas_pixel;
    slot->canvas.hline = py_canvas_hline;
    slot->canvas.vline = py_canvas_vline;
    slot->canvas.fill_rect = py_canvas_fill_rect;
    return true;
}

bool mp_canvas_resolve(mp_obj_t target, mp_canvas_slot_t *slot) {
    if (mp_obj_is_type(target, &mp_type_clipped_canvas)) {
        typedef struct _mp_obj_clipped_canvas_t {
            mp_obj_base_t base;
            mp_obj_t canvas_obj;
            gfx_area_t clip;
            mp_canvas_slot_t parent_slot;
            gfx_clipped_canvas_t clipped;
        } mp_obj_clipped_canvas_t;
        mp_obj_clipped_canvas_t *cc = MP_OBJ_TO_PTR(target);
        if (!mp_canvas_resolve(cc->canvas_obj, &cc->parent_slot)) {
            return false;
        }
        gfx_clipped_canvas_init(&cc->clipped, &cc->parent_slot.canvas, &cc->clip);
        slot->kind = MP_CANVAS_PY_OBJ;
        slot->u.py.obj = target;
        slot->canvas = cc->clipped.base;
        return true;
    }
    if (mp_get_native_framebuf(target, &slot->u.fb)) {
        slot->kind = MP_CANVAS_NATIVE_FB;
        slot->canvas = slot->u.fb.canvas;
        return true;
    }
    if (mp_obj_is_type(target, &mp_type_bmp565)) {
        mp_obj_bmp565_t *bmp_obj = MP_OBJ_TO_PTR(target);
        if (!bmp_obj->bmp.streamed && bmp_obj->bmp.buffer) {
            slot->kind = MP_CANVAS_BMP565_BUF;
            slot->u.bmp.gfx_fb.buf = bmp_obj->bmp.buffer;
            slot->u.bmp.gfx_fb.width = (uint16_t)bmp_obj->bmp.width;
            slot->u.bmp.gfx_fb.height = (uint16_t)bmp_obj->bmp.height;
            slot->u.bmp.gfx_fb.stride = (uint16_t)bmp_obj->bmp.width;
            slot->u.bmp.gfx_fb.format = GFX_RGB565;
            gfx_fb_canvas_init(&slot->canvas, &slot->u.bmp.gfx_fb);
            return true;
        }
    }
    return mp_init_py_canvas(target, slot);
}
