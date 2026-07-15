/*
 * MicroPython ClippedCanvas and ClipContext types.
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "py/runtime.h"
#include "py/binary.h"

#include "gfx_core.h"
#include "gfx_shapes.h"
#include "gfx_draw.h"
#include "gfx_area_mp.h"
#include "gfx_canvas_mp.h"
#include "gfx_clip_mp.h"
#include "gfx_bindings_mp.h"

typedef struct _mp_obj_clipped_canvas_t {
    mp_obj_base_t base;
    mp_obj_t canvas_obj;
    gfx_area_t clip;
    mp_canvas_slot_t parent_slot;
    gfx_clipped_canvas_t clipped;
} mp_obj_clipped_canvas_t;

const mp_obj_type_t mp_type_clipped_canvas;

static mp_obj_clipped_canvas_t *clipped_canvas_from_obj(mp_obj_t obj) {
    if (!mp_obj_is_type(obj, &mp_type_clipped_canvas)) {
        mp_raise_TypeError(MP_ERROR_TEXT("ClippedCanvas required"));
    }
    return MP_OBJ_TO_PTR(obj);
}

static void clipped_canvas_bind_parent(mp_obj_clipped_canvas_t *self) {
    if (!mp_canvas_resolve(self->canvas_obj, &self->parent_slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    gfx_clipped_canvas_init(&self->clipped, &self->parent_slot.canvas, &self->clip);
}

static mp_obj_t clipped_canvas_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args_in) {
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    mp_obj_t native = mp_obj_cast_to_native_base(args_in[1], MP_OBJ_FROM_PTR(&mp_type_area));
    if (native == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("Area required"));
    }
    typedef struct { mp_obj_base_t base; gfx_area_t area; } mp_obj_area_t;
    mp_obj_clipped_canvas_t *o = mp_obj_malloc(mp_obj_clipped_canvas_t, type);
    o->canvas_obj = args_in[0];
    o->clip = ((mp_obj_area_t *)MP_OBJ_TO_PTR(native))->area;
    clipped_canvas_bind_parent(o);
    return MP_OBJ_FROM_PTR(o);
}

static void clipped_canvas_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_clipped_canvas_t *self = clipped_canvas_from_obj(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR_width) {
            dest[0] = mp_obj_new_int(self->parent_slot.canvas.width);
        } else if (attr == MP_QSTR_height) {
            dest[0] = mp_obj_new_int(self->parent_slot.canvas.height);
        } else {
            dest[1] = self->canvas_obj;
            dest[0] = MP_OBJ_NULL;
        }
    }
}

static mp_obj_t clipped_canvas_pixel(size_t n_args, const mp_obj_t *args) {
    mp_obj_clipped_canvas_t *self = clipped_canvas_from_obj(args[0]);
    clipped_canvas_bind_parent(self);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    if (!gfx_area_contains_point(&self->clip, x, y)) {
        return mp_const_none;
    }
    if (n_args == 3) {
        mp_obj_t method[4];
        mp_load_method(self->canvas_obj, MP_QSTR_pixel, method);
        method[2] = args[1];
        method[3] = args[2];
        return mp_call_method_n_kw(2, 0, method);
    }
    gfx_area_t area = gfx_shapes_pixel(&self->clipped.base, x, y, mp_obj_get_int(args[3]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_canvas_pixel_obj, 3, 4, clipped_canvas_pixel);

static mp_obj_t clipped_canvas_fill(mp_obj_t self_in, mp_obj_t col_in) {
    mp_obj_clipped_canvas_t *self = clipped_canvas_from_obj(self_in);
    clipped_canvas_bind_parent(self);
    gfx_area_t area = gfx_shapes_fill_rect(&self->clipped.base, self->clip.x, self->clip.y, self->clip.w, self->clip.h, mp_obj_get_int(col_in));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_2(clipped_canvas_fill_obj, clipped_canvas_fill);

static mp_obj_t clipped_canvas_fill_rect(size_t n_args, const mp_obj_t *args) {
    mp_obj_clipped_canvas_t *self = clipped_canvas_from_obj(args[0]);
    clipped_canvas_bind_parent(self);
    gfx_area_t hit;
    if (!gfx_intersect_rect(mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
            mp_obj_get_int(args[4]), &self->clip, &hit)) {
        return mp_const_none;
    }
    gfx_area_t area = gfx_shapes_fill_rect(&self->clipped.base, hit.x, hit.y, hit.w, hit.h, mp_obj_get_int(args[5]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_canvas_fill_rect_obj, 6, 6, clipped_canvas_fill_rect);

static mp_obj_t clipped_canvas_hline(size_t n_args, const mp_obj_t *args) {
    mp_obj_t fill_args[6] = {
        args[0],
        args[1],
        args[2],
        args[3],
        mp_obj_new_int(1),
        args[4],
    };
    return clipped_canvas_fill_rect(6, fill_args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_canvas_hline_obj, 5, 5, clipped_canvas_hline);

static mp_obj_t clipped_canvas_vline(size_t n_args, const mp_obj_t *args) {
    mp_obj_clipped_canvas_t *self = clipped_canvas_from_obj(args[0]);
    clipped_canvas_bind_parent(self);
    mp_obj_t fill_args[6] = {
        args[0],
        args[1],
        args[2],
        mp_obj_new_int(1),
        args[3],
        args[4],
    };
    return clipped_canvas_fill_rect(6, fill_args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_canvas_vline_obj, 5, 5, clipped_canvas_vline);

static mp_obj_t clipped_canvas_blit_rect(size_t n_args, const mp_obj_t *args) {
    mp_obj_clipped_canvas_t *self = clipped_canvas_from_obj(args[0]);
    clipped_canvas_bind_parent(self);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t w = mp_obj_get_int(args[4]);
    mp_int_t h = mp_obj_get_int(args[5]);
    gfx_area_t hit;
    if (!gfx_intersect_rect(x, y, w, h, &self->clip, &hit)) {
        return mp_const_none;
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    int dx = hit.x - x;
    int dy = hit.y - y;
    const void *blit_buf = bufinfo.buf;
    size_t blit_len = bufinfo.len;
    uint8_t crop_buf[4096];
    if (dx || dy || hit.w != w || hit.h != h) {
        size_t need = (size_t)hit.w * (size_t)hit.h * 2;
        if (need > sizeof(crop_buf)) {
            mp_raise_ValueError(MP_ERROR_TEXT("blit crop too large"));
        }
        gfx_crop_rgb565_buffer(bufinfo.buf, w, dx, dy, hit.w, hit.h, crop_buf);
        blit_buf = crop_buf;
        blit_len = need;
    }
    (void)blit_len;
    gfx_area_t area = gfx_shapes_blit_rect(&self->clipped.base, blit_buf, hit.x, hit.y, hit.w, hit.h, 2);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_canvas_blit_rect_obj, 6, 6, clipped_canvas_blit_rect);

static mp_obj_t clipped_canvas_blit_transparent(size_t n_args, const mp_obj_t *args) {
    mp_obj_clipped_canvas_t *self = clipped_canvas_from_obj(args[0]);
    clipped_canvas_bind_parent(self);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t w = mp_obj_get_int(args[4]);
    mp_int_t h = mp_obj_get_int(args[5]);
    gfx_area_t hit;
    if (!gfx_intersect_rect(x, y, w, h, &self->clip, &hit)) {
        return mp_const_none;
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    int dx = hit.x - x;
    int dy = hit.y - y;
    const void *blit_buf = bufinfo.buf;
    uint8_t crop_buf[4096];
    if (dx || dy || hit.w != w || hit.h != h) {
        size_t need = (size_t)hit.w * (size_t)hit.h * 2;
        if (need > sizeof(crop_buf)) {
            mp_raise_ValueError(MP_ERROR_TEXT("blit crop too large"));
        }
        gfx_crop_rgb565_buffer(bufinfo.buf, w, dx, dy, hit.w, hit.h, crop_buf);
        blit_buf = crop_buf;
    }
    gfx_area_t area = gfx_shapes_blit_transparent(&self->clipped.base, blit_buf, hit.x, hit.y, hit.w, hit.h, mp_obj_get_int(args[6]), 2);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_canvas_blit_transparent_obj, 7, 7, clipped_canvas_blit_transparent);

static const mp_rom_map_elem_t clipped_canvas_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&clipped_canvas_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&clipped_canvas_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&clipped_canvas_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&clipped_canvas_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&clipped_canvas_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_rect), MP_ROM_PTR(&clipped_canvas_blit_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_transparent), MP_ROM_PTR(&clipped_canvas_blit_transparent_obj) },
};
static MP_DEFINE_CONST_DICT(clipped_canvas_locals_dict, clipped_canvas_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_clipped_canvas,
    MP_QSTR_ClippedCanvas,
    MP_TYPE_FLAG_NONE,
    make_new, clipped_canvas_make_new,
    attr, clipped_canvas_attr,
    locals_dict, &clipped_canvas_locals_dict
);

/* ClipContext: context manager returned by Draw.clip() */
typedef struct _mp_obj_clip_ctx_t {
    mp_obj_base_t base;
    mp_obj_t draw_obj;
    gfx_area_t area;
} mp_obj_clip_ctx_t;

const mp_obj_type_t mp_type_clip_ctx;

static mp_obj_t clip_ctx_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args_in) {
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    if (!mp_obj_is_type(args_in[0], &mp_type_draw)) {
        mp_raise_TypeError(MP_ERROR_TEXT("Draw required"));
    }
    mp_obj_t native = mp_obj_cast_to_native_base(args_in[1], MP_OBJ_FROM_PTR(&mp_type_area));
    if (native == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("Area required"));
    }
    typedef struct { mp_obj_base_t base; gfx_area_t area; } mp_obj_area_t;
    mp_obj_clip_ctx_t *o = mp_obj_malloc(mp_obj_clip_ctx_t, type);
    o->draw_obj = args_in[0];
    o->area = ((mp_obj_area_t *)MP_OBJ_TO_PTR(native))->area;
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t clip_ctx_enter(mp_obj_t self_in) {
    mp_obj_clip_ctx_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_draw_t *draw = MP_OBJ_TO_PTR(self->draw_obj);
    gfx_draw_push_clip(&draw->draw, &self->area);
    gfx_area_t eff = gfx_draw_effective_clip(&draw->draw);
    return gfx_area_mp_from_gfx(&eff);
}
static MP_DEFINE_CONST_FUN_OBJ_1(clip_ctx_enter_obj, clip_ctx_enter);

static mp_obj_t clip_ctx_exit(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    mp_obj_clip_ctx_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_draw_t *draw = MP_OBJ_TO_PTR(self->draw_obj);
    gfx_draw_pop_clip(&draw->draw);
    return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clip_ctx_exit_obj, 4, 4, clip_ctx_exit);

static const mp_rom_map_elem_t clip_ctx_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&clip_ctx_enter_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&clip_ctx_exit_obj) },
};
static MP_DEFINE_CONST_DICT(clip_ctx_locals_dict, clip_ctx_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_clip_ctx,
    MP_QSTR_ClipContext,
    MP_TYPE_FLAG_NONE,
    make_new, clip_ctx_make_new,
    locals_dict, &clip_ctx_locals_dict
);

mp_obj_t gfx_mp_draw_clip(mp_obj_t draw_obj, const gfx_area_t *area) {
    mp_obj_clip_ctx_t *ctx = mp_obj_malloc(mp_obj_clip_ctx_t, &mp_type_clip_ctx);
    ctx->draw_obj = draw_obj;
    ctx->area = *area;
    return MP_OBJ_FROM_PTR(ctx);
}
