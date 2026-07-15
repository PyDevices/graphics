/*
 * Extended MicroPython bindings: Draw, Font, BMP565, module-level API.
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "py/runtime.h"
#include "py/binary.h"

#include "gfx_core.h"
#include "gfx_framebuffer.h"
#include "gfx_shapes.h"
#include "gfx_draw.h"
#include "gfx_font.h"
#include "gfx_bmp565.h"
#include "gfx_files.h"
#include "gfx_capabilities.h"
#include "gfx_area_mp.h"
#include "gfx_bindings_mp.h"
#include "gfx_canvas_mp.h"

extern const mp_obj_type_t mp_type_framebuf;
extern const mp_obj_type_t mp_type_area;

static bool mp_get_framebuf(mp_obj_t obj, mp_obj_framebuf_t *out) {
    mp_obj_t native = mp_obj_cast_to_native_base(obj, MP_OBJ_FROM_PTR(&mp_type_framebuf));
    if (native == MP_OBJ_NULL) {
        return false;
    }
    *out = *(mp_obj_framebuf_t *)MP_OBJ_TO_PTR(native);
    return true;
}

typedef struct _mp_obj_draw_t {
    mp_obj_base_t base;
    mp_obj_t canvas_obj;
    gfx_draw_t draw;
    mp_py_canvas_ctx_t py_ctx;
} mp_obj_draw_t;

const mp_obj_type_t mp_type_draw;

static void draw_bind_canvas(mp_obj_draw_t *self, const mp_canvas_slot_t *slot) {
    if (slot->kind == MP_CANVAS_PY_OBJ) {
        self->py_ctx = slot->u.py;
        self->draw.canvas = slot->canvas;
        self->draw.canvas.ctx = &self->py_ctx;
    } else {
        self->draw.canvas = slot->canvas;
    }
}

static mp_obj_t draw_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(args[0], &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("FrameBuffer required"));
    }
    mp_obj_draw_t *o = mp_obj_malloc(mp_obj_draw_t, &mp_type_draw);
    o->canvas_obj = args[0];
    draw_bind_canvas(o, &slot);
    gfx_draw_init(&o->draw, &o->draw.canvas);
    return MP_OBJ_FROM_PTR(o);
}

static const gfx_canvas_t *draw_target(mp_obj_draw_t *self) {
    mp_canvas_slot_t slot;
    if (mp_canvas_resolve(self->canvas_obj, &slot)) {
        draw_bind_canvas(self, &slot);
    }
    return gfx_draw_target(&self->draw);
}

static mp_obj_t draw_fill(mp_obj_t self_in, mp_obj_t col_in) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(self_in);
    gfx_area_t area = gfx_shapes_fill(draw_target(self), mp_obj_get_int(col_in));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_2(draw_fill_obj, draw_fill);

static mp_obj_t draw_fill_rect(size_t n_args, const mp_obj_t *args) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(args[0]);
    gfx_area_t area = gfx_shapes_fill_rect(draw_target(self), mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_fill_rect_obj, 6, 6, draw_fill_rect);

static mp_obj_t draw_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_x, ARG_y, ARG_w, ARG_h, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_rect(draw_target(self), parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_rect_obj, 6, draw_rect);

static mp_obj_t draw_round_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_x, ARG_y, ARG_w, ARG_h, ARG_r, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_round_rect(draw_target(self), parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_r].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_round_rect_obj, 7, draw_round_rect);

static mp_obj_t draw_line(size_t n_args, const mp_obj_t *args) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(args[0]);
    gfx_area_t area = gfx_shapes_line(draw_target(self), mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_line_obj, 6, 6, draw_line);

static mp_obj_t draw_hline(size_t n_args, const mp_obj_t *args) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(args[0]);
    gfx_area_t area = gfx_shapes_hline(draw_target(self), mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_hline_obj, 5, 5, draw_hline);

static mp_obj_t draw_vline(size_t n_args, const mp_obj_t *args) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(args[0]);
    gfx_area_t area = gfx_shapes_vline(draw_target(self), mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_vline_obj, 5, 5, draw_vline);

static mp_obj_t draw_pixel(size_t n_args, const mp_obj_t *args) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(args[0]);
    gfx_area_t area = gfx_shapes_pixel(draw_target(self), mp_obj_get_int(args[1]),
        mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_pixel_obj, 4, 4, draw_pixel);

static mp_obj_t draw_circle(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_x, ARG_y, ARG_r, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_circle(draw_target(self), parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_r].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_circle_obj, 5, draw_circle);

static mp_obj_t draw_ellipse(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_x, ARG_y, ARG_r1, ARG_r2, ARG_c, ARG_fill_pos, ARG_mask_pos, ARG_f, ARG_fill, ARG_m };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0x0f} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_m, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x0f} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    mp_int_t mask = (n_args > 7) ? parsed[ARG_mask_pos].u_int : parsed[ARG_m].u_int;
    gfx_area_t area = gfx_shapes_ellipse(draw_target(self), parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_r1].u_int, parsed[ARG_r2].u_int, parsed[ARG_c].u_int, fill, mask);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_ellipse_obj, 6, draw_ellipse);

static mp_obj_t draw_arc(size_t n_args, const mp_obj_t *args) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(args[0]);
    gfx_area_t area = gfx_shapes_arc(draw_target(self), mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), (float)mp_obj_get_float(args[4]), (float)mp_obj_get_float(args[5]),
        mp_obj_get_int(args[6]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_arc_obj, 7, 7, draw_arc);

static mp_obj_t draw_triangle(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_x0, ARG_y0, ARG_x1, ARG_y1, ARG_x2, ARG_y2, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_triangle(draw_target(self),
        parsed[ARG_x0].u_int, parsed[ARG_y0].u_int, parsed[ARG_x1].u_int, parsed[ARG_y1].u_int,
        parsed[ARG_x2].u_int, parsed[ARG_y2].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_triangle_obj, 8, draw_triangle);

static mp_obj_t draw_gradient_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_x, ARG_y, ARG_w, ARG_h, ARG_c1, ARG_c2_pos, ARG_vert_pos, ARG_c2, ARG_vertical };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_, MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_c2, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_vertical, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    mp_obj_t c2_obj = (n_args > 6) ? parsed[ARG_c2_pos].u_obj : parsed[ARG_c2].u_obj;
    int c2 = (c2_obj == mp_const_none) ? parsed[ARG_c1].u_int : mp_obj_get_int(c2_obj);
    int vertical = (n_args > 7) ? parsed[ARG_vert_pos].u_bool : parsed[ARG_vertical].u_bool;
    gfx_area_t area = gfx_shapes_gradient_rect(draw_target(self), parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_c1].u_int, c2, vertical);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_gradient_rect_obj, 6, draw_gradient_rect);

#if MICROPY_PY_ARRAY
static mp_obj_t draw_poly(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_x, ARG_y, ARG_coords, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(parsed[ARG_coords].u_obj, &bufinfo, MP_BUFFER_READ);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    char fmt[2] = { (char)bufinfo.typecode, '\0' };
    gfx_area_t area = gfx_shapes_poly(draw_target(self), parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        bufinfo.buf, bufinfo.len, mp_binary_get_size('@', bufinfo.typecode, NULL), fmt,
        parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_poly_obj, 5, draw_poly);
#endif

static mp_obj_t draw_polygon(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_points, ARG_x, ARG_y, ARG_color, ARG_angle_pos, ARG_cx_pos, ARG_cy_pos, ARG_angle, ARG_center_x, ARG_center_y };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(0)} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_angle, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(0)} },
        { MP_QSTR_center_x, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_center_y, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    size_t len;
    mp_obj_t *items;
    mp_obj_get_array(parsed[ARG_points].u_obj, &len, &items);
    if (len < 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("Polygon must have at least 3 points"));
    }
    int points[128];
    if (len > 64) {
        mp_raise_ValueError(NULL);
    }
    for (size_t i = 0; i < len; i++) {
        size_t plen;
        mp_obj_t *pitems;
        mp_obj_get_array(items[i], &plen, &pitems);
        if (plen != 2) {
            mp_raise_ValueError(NULL);
        }
        points[i * 2] = mp_obj_get_int(pitems[0]);
        points[i * 2 + 1] = mp_obj_get_int(pitems[1]);
    }
    mp_obj_t angle_obj = (n_args > 5) ? parsed[ARG_angle_pos].u_obj : parsed[ARG_angle].u_obj;
    float angle = (float)mp_obj_get_float(angle_obj);
    int cx = (n_args > 6) ? parsed[ARG_cx_pos].u_int : parsed[ARG_center_x].u_int;
    int cy = (n_args > 7) ? parsed[ARG_cy_pos].u_int : parsed[ARG_center_y].u_int;
    gfx_area_t area = gfx_shapes_polygon(draw_target(self), points, len, parsed[ARG_x].u_int,
        parsed[ARG_y].u_int, parsed[ARG_color].u_int, angle, cx, cy);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_polygon_obj, 5, draw_polygon);

static void get_readonly_framebuffer_draw(mp_obj_t arg, mp_obj_framebuf_t *rofb) {
    mp_obj_t fb = mp_obj_cast_to_native_base(arg, MP_OBJ_FROM_PTR(&mp_type_framebuf));
    if (fb != MP_OBJ_NULL) {
        *rofb = *(mp_obj_framebuf_t *)MP_OBJ_TO_PTR(fb);
    } else {
        size_t len;
        mp_obj_t *items;
        mp_obj_get_array(arg, &len, &items);
        if (len < 4 || len > 5) {
            mp_raise_ValueError(NULL);
        }
        framebuf_make_new_helper(len, items, MP_BUFFER_READ, rofb);
    }
}

static mp_obj_t draw_blit(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_source, ARG_x, ARG_y, ARG_key_pos, ARG_palette_pos, ARG_key, ARG_palette };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_key, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_palette, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    mp_obj_framebuf_t source;
    get_readonly_framebuffer_draw(parsed[ARG_source].u_obj, &source);
    mp_int_t key = (n_args > 4) ? parsed[ARG_key_pos].u_int : parsed[ARG_key].u_int;
    mp_obj_t pal_obj = (n_args > 5) ? parsed[ARG_palette_pos].u_obj : parsed[ARG_palette].u_obj;
    const gfx_fb_t *pal = NULL;
    mp_obj_framebuf_t palette;
    if (pal_obj != mp_const_none) {
        get_readonly_framebuffer_draw(pal_obj, &palette);
        pal = &palette.fb;
    }
    gfx_area_t area = gfx_shapes_blit(draw_target(self), &source.fb, parsed[ARG_x].u_int, parsed[ARG_y].u_int, key, pal);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_blit_obj, 4, draw_blit);

static mp_obj_t draw_blit_rect(size_t n_args, const mp_obj_t *args) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    gfx_area_t area = gfx_shapes_blit_rect(draw_target(self), bufinfo.buf, mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]), 2);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_blit_rect_obj, 6, 6, draw_blit_rect);

static mp_obj_t draw_blit_transparent(size_t n_args, const mp_obj_t *args) {
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    gfx_area_t area = gfx_shapes_blit_transparent(draw_target(self), bufinfo.buf, mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]),
        mp_obj_get_int(args[6]), 2);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_blit_transparent_obj, 7, 7, draw_blit_transparent);

static mp_obj_t draw_text_height(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args, int height) {
    enum { ARG_self, ARG_s, ARG_x, ARG_y, ARG_c, ARG_scale, ARG_inverted, ARG_font_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_scale, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_inverted, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_font_data, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    gfx_font_t font;
    if (parsed[ARG_font_data].u_obj != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(parsed[ARG_font_data].u_obj, &bufinfo, MP_BUFFER_READ);
        gfx_font_init_from_data(&font, bufinfo.buf, bufinfo.len, height);
    } else {
        gfx_font_init_default(&font, height);
    }
    gfx_area_t area = gfx_font_text(draw_target(self), &font, mp_obj_str_get_str(parsed[ARG_s].u_obj),
        parsed[ARG_x].u_int, parsed[ARG_y].u_int, parsed[ARG_c].u_int,
        parsed[ARG_scale].u_int, parsed[ARG_inverted].u_bool);
    if (font.owns_data) {
        gfx_font_deinit(&font);
    }
    return gfx_area_mp_from_gfx(&area);
}

static mp_obj_t draw_text(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_s, ARG_x, ARG_y, ARG_c, ARG_scale, ARG_inverted, ARG_font_data, ARG_height };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_scale, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_inverted, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_font_data, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_height, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_draw_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    mp_int_t height = parsed[ARG_height].u_int;
    gfx_font_t font;
    if (parsed[ARG_font_data].u_obj != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(parsed[ARG_font_data].u_obj, &bufinfo, MP_BUFFER_READ);
        gfx_font_init_from_data(&font, bufinfo.buf, bufinfo.len, height);
    } else {
        gfx_font_init_default(&font, height);
    }
    gfx_area_t area = gfx_font_text(draw_target(self), &font, mp_obj_str_get_str(parsed[ARG_s].u_obj),
        parsed[ARG_x].u_int, parsed[ARG_y].u_int, parsed[ARG_c].u_int,
        parsed[ARG_scale].u_int, parsed[ARG_inverted].u_bool);
    if (font.owns_data) {
        gfx_font_deinit(&font);
    }
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_text_obj, 4, draw_text);

static mp_obj_t draw_text8(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return draw_text_height(n_args, args, kw_args, 8);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_text8_obj, 4, draw_text8);

static mp_obj_t draw_text14(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return draw_text_height(n_args, args, kw_args, 14);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_text14_obj, 4, draw_text14);

static mp_obj_t draw_text16(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return draw_text_height(n_args, args, kw_args, 16);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(draw_text16_obj, 4, draw_text16);

/* ClipContext: with draw.clip(x,y,w,h) / draw.clip(Area) — matches graphics._clip */
typedef struct _mp_obj_clip_ctx_t {
    mp_obj_base_t base;
    mp_obj_t draw_obj;
    gfx_area_t area;
} mp_obj_clip_ctx_t;

const mp_obj_type_t mp_type_clip_ctx;

static gfx_area_t clip_ctx_parse_area(mp_obj_t area_in) {
    mp_obj_t native = mp_obj_cast_to_native_base(area_in, MP_OBJ_FROM_PTR(&mp_type_area));
    if (native == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("Area required"));
    }
    typedef struct { mp_obj_base_t base; gfx_area_t area; } mp_obj_area_t;
    return ((mp_obj_area_t *)MP_OBJ_TO_PTR(native))->area;
}

static mp_obj_t clip_ctx_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    (void)type;
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    if (!mp_obj_is_type(args[0], &mp_type_draw)) {
        mp_raise_TypeError(MP_ERROR_TEXT("Draw required"));
    }
    mp_obj_clip_ctx_t *ctx = mp_obj_malloc(mp_obj_clip_ctx_t, &mp_type_clip_ctx);
    ctx->draw_obj = args[0];
    ctx->area = clip_ctx_parse_area(args[1]);
    return MP_OBJ_FROM_PTR(ctx);
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

static mp_obj_t draw_clip(size_t n_args, const mp_obj_t *args) {
    gfx_area_t area;
    if (n_args == 2) {
        area = clip_ctx_parse_area(args[1]);
    } else if (n_args == 5) {
        gfx_area_init(&area, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
            mp_obj_get_int(args[3]), mp_obj_get_int(args[4]));
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("clip() requires x, y, w, h or an Area"));
    }
    mp_obj_clip_ctx_t *ctx = mp_obj_malloc(mp_obj_clip_ctx_t, &mp_type_clip_ctx);
    ctx->draw_obj = args[0];
    ctx->area = area;
    return MP_OBJ_FROM_PTR(ctx);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_clip_obj, 2, 5, draw_clip);

/* ClippedCanvas: proxy that restricts drawing to a clip Area — matches graphics._clip */
typedef struct _mp_obj_clipped_canvas_t {
    mp_obj_base_t base;
    mp_obj_t canvas_obj;
    mp_obj_t clip_obj;
    gfx_area_t clip;
} mp_obj_clipped_canvas_t;

const mp_obj_type_t mp_type_clipped_canvas;

static bool clipped_intersect(const gfx_area_t *clip, int x, int y, int w, int h, gfx_area_t *out) {
    if (w <= 0 || h <= 0) {
        return false;
    }
    gfx_area_t rect = gfx_area_from_rect(x, y, w, h);
    *out = gfx_area_clip(&rect, clip);
    return out->w > 0 && out->h > 0;
}

static const gfx_canvas_t *clipped_bind(mp_obj_clipped_canvas_t *self, mp_canvas_slot_t *slot, gfx_clipped_canvas_t *cc) {
    if (!mp_canvas_resolve(self->canvas_obj, slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    gfx_clipped_canvas_init(cc, &slot->canvas, &self->clip);
    return &cc->base;
}

static mp_obj_t clipped_canvas_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    (void)type;
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    mp_obj_clipped_canvas_t *o = mp_obj_malloc(mp_obj_clipped_canvas_t, &mp_type_clipped_canvas);
    o->canvas_obj = args[0];
    o->clip_obj = args[1];
    o->clip = clip_ctx_parse_area(args[1]);
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t clipped_pixel(size_t n_args, const mp_obj_t *args) {
    mp_obj_clipped_canvas_t *self = MP_OBJ_TO_PTR(args[0]);
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    if (!gfx_area_contains_point(&self->clip, x, y)) {
        return mp_const_none;
    }
    mp_canvas_slot_t slot;
    gfx_clipped_canvas_t cc;
    const gfx_canvas_t *canvas = clipped_bind(self, &slot, &cc);
    if (n_args == 3) {
        return mp_obj_new_int(canvas->pixel(canvas->ctx, x, y, 0, 0));
    }
    gfx_area_t area = gfx_shapes_pixel(canvas, x, y, mp_obj_get_int(args[3]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_pixel_obj, 3, 4, clipped_pixel);

static mp_obj_t clipped_fill_rect(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    mp_obj_clipped_canvas_t *self = MP_OBJ_TO_PTR(args[0]);
    gfx_area_t hit;
    if (!clipped_intersect(&self->clip, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
            mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), &hit)) {
        return mp_const_none;
    }
    mp_canvas_slot_t slot;
    gfx_clipped_canvas_t cc;
    const gfx_canvas_t *canvas = clipped_bind(self, &slot, &cc);
    gfx_shapes_fill_rect(canvas, hit.x, hit.y, hit.w, hit.h, mp_obj_get_int(args[5]));
    return gfx_area_mp_from_gfx(&hit);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_fill_rect_obj, 6, 6, clipped_fill_rect);

static mp_obj_t clipped_fill(mp_obj_t self_in, mp_obj_t c_in) {
    mp_obj_clipped_canvas_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t args[] = {
        self_in,
        mp_obj_new_int(self->clip.x),
        mp_obj_new_int(self->clip.y),
        mp_obj_new_int(self->clip.w),
        mp_obj_new_int(self->clip.h),
        c_in,
    };
    return clipped_fill_rect(6, args);
}
static MP_DEFINE_CONST_FUN_OBJ_2(clipped_fill_obj, clipped_fill);

static mp_obj_t clipped_hline(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    mp_obj_t call[] = {
        args[0], args[1], args[2], args[3], mp_obj_new_int(1), args[4],
    };
    return clipped_fill_rect(6, call);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_hline_obj, 5, 5, clipped_hline);

static mp_obj_t clipped_vline(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    mp_obj_t call[] = {
        args[0], args[1], args[2], mp_obj_new_int(1), args[3], args[4],
    };
    return clipped_fill_rect(6, call);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_vline_obj, 5, 5, clipped_vline);

static mp_obj_t clipped_blit_rect(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    mp_obj_clipped_canvas_t *self = MP_OBJ_TO_PTR(args[0]);
    int x = mp_obj_get_int(args[2]);
    int y = mp_obj_get_int(args[3]);
    int w = mp_obj_get_int(args[4]);
    int h = mp_obj_get_int(args[5]);
    gfx_area_t hit;
    if (!clipped_intersect(&self->clip, x, y, w, h, &hit)) {
        return mp_const_none;
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    mp_canvas_slot_t slot;
    gfx_clipped_canvas_t cc;
    const gfx_canvas_t *canvas = clipped_bind(self, &slot, &cc);
    /* Crop when the destination rect is partially outside the clip (RGB565). */
    int dx = hit.x - x;
    int dy = hit.y - y;
    if (dx || dy || hit.w != w || hit.h != h) {
        size_t row_bytes = (size_t)hit.w * 2;
        size_t out_len = row_bytes * (size_t)hit.h;
        uint8_t *out = m_new(uint8_t, out_len);
        const uint8_t *src = (const uint8_t *)bufinfo.buf;
        for (int row = 0; row < hit.h; row++) {
            size_t src_start = ((size_t)(dy + row) * (size_t)w + (size_t)dx) * 2;
            memcpy(out + (size_t)row * row_bytes, src + src_start, row_bytes);
        }
        gfx_shapes_blit_rect(canvas, out, hit.x, hit.y, hit.w, hit.h, 2);
        m_del(uint8_t, out, out_len);
    } else {
        gfx_shapes_blit_rect(canvas, bufinfo.buf, hit.x, hit.y, hit.w, hit.h, 2);
    }
    return gfx_area_mp_from_gfx(&hit);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_blit_rect_obj, 6, 6, clipped_blit_rect);

static mp_obj_t clipped_blit_transparent(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    mp_obj_clipped_canvas_t *self = MP_OBJ_TO_PTR(args[0]);
    int x = mp_obj_get_int(args[2]);
    int y = mp_obj_get_int(args[3]);
    int w = mp_obj_get_int(args[4]);
    int h = mp_obj_get_int(args[5]);
    gfx_area_t hit;
    if (!clipped_intersect(&self->clip, x, y, w, h, &hit)) {
        return mp_const_none;
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    mp_canvas_slot_t slot;
    gfx_clipped_canvas_t cc;
    const gfx_canvas_t *canvas = clipped_bind(self, &slot, &cc);
    int dx = hit.x - x;
    int dy = hit.y - y;
    int key = mp_obj_get_int(args[6]);
    if (dx || dy || hit.w != w || hit.h != h) {
        size_t row_bytes = (size_t)hit.w * 2;
        size_t out_len = row_bytes * (size_t)hit.h;
        uint8_t *out = m_new(uint8_t, out_len);
        const uint8_t *src = (const uint8_t *)bufinfo.buf;
        for (int row = 0; row < hit.h; row++) {
            size_t src_start = ((size_t)(dy + row) * (size_t)w + (size_t)dx) * 2;
            memcpy(out + (size_t)row * row_bytes, src + src_start, row_bytes);
        }
        gfx_shapes_blit_transparent(canvas, out, hit.x, hit.y, hit.w, hit.h, key, 2);
        m_del(uint8_t, out, out_len);
    } else {
        gfx_shapes_blit_transparent(canvas, bufinfo.buf, hit.x, hit.y, hit.w, hit.h, key, 2);
    }
    return gfx_area_mp_from_gfx(&hit);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(clipped_blit_transparent_obj, 7, 7, clipped_blit_transparent);

static void clipped_canvas_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_clipped_canvas_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        switch (attr) {
            case MP_QSTR_width:
            case MP_QSTR_height:
                dest[0] = mp_load_attr(self->canvas_obj, attr);
                return;
            case MP_QSTR_pixel:
            case MP_QSTR_fill:
            case MP_QSTR_fill_rect:
            case MP_QSTR_hline:
            case MP_QSTR_vline:
            case MP_QSTR_blit_rect:
            case MP_QSTR_blit_transparent:
                /* Defer to locals_dict (attr runs first). */
                dest[1] = MP_OBJ_SENTINEL;
                return;
            default:
                /* Forward other attributes / methods to the underlying canvas. */
                mp_load_method_maybe(self->canvas_obj, attr, dest);
                return;
        }
    } else {
        mp_store_attr(self->canvas_obj, attr, dest[1]);
        dest[0] = MP_OBJ_NULL;
    }
}

static const mp_rom_map_elem_t clipped_canvas_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&clipped_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&clipped_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&clipped_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&clipped_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&clipped_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_rect), MP_ROM_PTR(&clipped_blit_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_transparent), MP_ROM_PTR(&clipped_blit_transparent_obj) },
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

static const mp_rom_map_elem_t draw_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&draw_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&draw_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&draw_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&draw_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_round_rect), MP_ROM_PTR(&draw_round_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&draw_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&draw_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&draw_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&draw_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_ellipse), MP_ROM_PTR(&draw_ellipse_obj) },
    { MP_ROM_QSTR(MP_QSTR_arc), MP_ROM_PTR(&draw_arc_obj) },
    { MP_ROM_QSTR(MP_QSTR_triangle), MP_ROM_PTR(&draw_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_gradient_rect), MP_ROM_PTR(&draw_gradient_rect_obj) },
#if MICROPY_PY_ARRAY
    { MP_ROM_QSTR(MP_QSTR_poly), MP_ROM_PTR(&draw_poly_obj) },
#endif
    { MP_ROM_QSTR(MP_QSTR_polygon), MP_ROM_PTR(&draw_polygon_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&draw_blit_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_rect), MP_ROM_PTR(&draw_blit_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_transparent), MP_ROM_PTR(&draw_blit_transparent_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&draw_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_text8), MP_ROM_PTR(&draw_text8_obj) },
    { MP_ROM_QSTR(MP_QSTR_text14), MP_ROM_PTR(&draw_text14_obj) },
    { MP_ROM_QSTR(MP_QSTR_text16), MP_ROM_PTR(&draw_text16_obj) },
    { MP_ROM_QSTR(MP_QSTR_clip), MP_ROM_PTR(&draw_clip_obj) },
};
static MP_DEFINE_CONST_DICT(draw_locals_dict, draw_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_draw,
    MP_QSTR_Draw,
    MP_TYPE_FLAG_NONE,
    make_new, draw_make_new,
    locals_dict, &draw_locals_dict
);

typedef struct _mp_obj_font_t {
    mp_obj_base_t base;
    gfx_font_t font;
    mp_obj_t data_obj;
    mp_obj_t path_obj;
} mp_obj_font_t;

const mp_obj_type_t mp_type_font;

static int parse_font_height_from_name(const char *path, int *height) {
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    const char *x = strrchr(base, 'x');
    if (!x) {
        return -1;
    }
    *height = atoi(x + 1);
    return 0;
}

static mp_obj_t font_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args_in) {
    enum { ARG_font_data, ARG_height };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_font_data, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_height, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args_in, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);

    mp_obj_font_t *o = mp_obj_malloc(mp_obj_font_t, &mp_type_font);
    o->data_obj = mp_const_none;
    o->path_obj = mp_const_none;
    int height = parsed[ARG_height].u_int;
    mp_obj_t font_arg = parsed[ARG_font_data].u_obj;

    if (font_arg != mp_const_none) {
        if (mp_obj_is_str(font_arg)) {
            const char *path = mp_obj_str_get_str(font_arg);
            if (height == 0 && parse_font_height_from_name(path, &height) < 0) {
                mp_raise_ValueError(MP_ERROR_TEXT("Invalid font"));
            }
            if (height == 0) {
                height = 8;
            }
            FILE *f = fopen(path, "rb");
            if (!f) {
                mp_raise_ValueError(MP_ERROR_TEXT("Font not found"));
            }
            fseek(f, 0, SEEK_END);
            long flen = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t *data = m_new(uint8_t, (size_t)flen);
            if (fread(data, 1, (size_t)flen, f) != (size_t)flen) {
                fclose(f);
                m_del(uint8_t, data, (size_t)flen);
                mp_raise_ValueError(MP_ERROR_TEXT("Font read failed"));
            }
            fclose(f);
            o->font.data = data;
            o->font.data_len = (size_t)flen;
            o->font.height = height;
            o->font.width = 8;
            o->font.owns_data = 1;
            o->path_obj = font_arg;
        } else {
            mp_buffer_info_t bufinfo;
            mp_get_buffer_raise(font_arg, &bufinfo, MP_BUFFER_READ);
            o->data_obj = font_arg;
            if (height == 0) {
                /* Match Python: derive height from a full 256-glyph memoryview. */
                height = bufinfo.len ? (int)(bufinfo.len / 256) : 8;
                if (height == 0) {
                    height = 8;
                }
            }
            gfx_font_init_from_data(&o->font, bufinfo.buf, bufinfo.len, height);
        }
    } else {
        if (height == 0) {
            height = 8;
        }
        gfx_font_init_default(&o->font, height);
    }
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t font_text(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_canvas, ARG_s, ARG_x, ARG_y, ARG_color, ARG_scale, ARG_inverted };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_scale, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_inverted, MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_font_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    gfx_area_t area = gfx_font_text(&slot.canvas, &self->font, mp_obj_str_get_str(parsed[ARG_s].u_obj),
        parsed[ARG_x].u_int, parsed[ARG_y].u_int, parsed[ARG_color].u_int,
        parsed[ARG_scale].u_int, parsed[ARG_inverted].u_bool);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(font_text_obj, 6, font_text);

static mp_obj_t font_draw_char(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    /* Font.draw_char(char, x, y, canvas, color, scale=1, inverted=False) */
    enum { ARG_self, ARG_char, ARG_x, ARG_y, ARG_canvas, ARG_color, ARG_scale, ARG_inverted };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_scale, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_inverted, MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_font_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    const char *ch = mp_obj_str_get_str(parsed[ARG_char].u_obj);
    gfx_area_t area = gfx_font_draw_char(&slot.canvas, &self->font, (unsigned char)ch[0],
        parsed[ARG_x].u_int, parsed[ARG_y].u_int, parsed[ARG_color].u_int,
        parsed[ARG_scale].u_int, parsed[ARG_inverted].u_bool);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(font_draw_char_obj, 6, font_draw_char);

static mp_obj_t font_text_width(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_text, ARG_scale };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_scale, MP_ARG_INT, {.u_int = 1} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_font_t *self = MP_OBJ_TO_PTR(parsed[ARG_self].u_obj);
    return mp_obj_new_int(gfx_font_text_width(&self->font, mp_obj_str_get_str(parsed[ARG_text].u_obj),
        parsed[ARG_scale].u_int));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(font_text_width_obj, 2, font_text_width);

static mp_obj_t font_deinit(mp_obj_t self_in) {
    mp_obj_font_t *self = MP_OBJ_TO_PTR(self_in);
    gfx_font_deinit(&self->font);
    self->data_obj = mp_const_none;
    self->path_obj = mp_const_none;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(font_deinit_obj, font_deinit);

static mp_obj_t font_export(mp_obj_t self_in, mp_obj_t filename_in) {
    mp_obj_font_t *self = MP_OBJ_TO_PTR(self_in);
    if (gfx_font_export(&self->font, mp_obj_str_get_str(filename_in)) < 0) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Font data not cached, cannot export"));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(font_export_obj, font_export);

static void font_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_font_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        switch (attr) {
            case MP_QSTR_width:
                dest[0] = mp_obj_new_int(self->font.width);
                break;
            case MP_QSTR_height:
                dest[0] = mp_obj_new_int(self->font.height);
                break;
            case MP_QSTR_font_name:
                if (self->path_obj != mp_const_none) {
                    dest[0] = self->path_obj;
                } else if (self->data_obj != mp_const_none) {
                    dest[0] = mp_obj_new_str("memoryview", 10);
                } else {
                    dest[0] = mp_obj_new_str("default", 7);
                }
                break;
            default:
                dest[1] = MP_OBJ_SENTINEL;
                break;
        }
    }
}

static const mp_rom_map_elem_t font_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&font_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_char), MP_ROM_PTR(&font_draw_char_obj) },
    { MP_ROM_QSTR(MP_QSTR_text_width), MP_ROM_PTR(&font_text_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&font_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_export), MP_ROM_PTR(&font_export_obj) },
};
static MP_DEFINE_CONST_DICT(font_locals_dict, font_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_font,
    MP_QSTR_Font,
    MP_TYPE_FLAG_NONE,
    make_new, font_make_new,
    attr, font_attr,
    locals_dict, &font_locals_dict
);

const mp_obj_type_t mp_type_bmp565;

static mp_obj_t bmp565_get_region(mp_obj_bmp565_t *self, mp_int_t x0, mp_int_t x1, mp_int_t y0, mp_int_t y1) {
    size_t len = 0;
    size_t cap = (size_t)(x1 - x0) * (size_t)(y1 - y0) * GFX_BMP565_BYTES_PER_PIXEL;
    uint8_t *data = m_new(uint8_t, cap);
    if (gfx_bmp565_read_region(&self->bmp, x0, x1, y0, y1, data, cap, &len) < 0) {
        m_del(uint8_t, data, cap);
        mp_raise_ValueError(MP_ERROR_TEXT("BMP read failed"));
    }
    mp_obj_t out = mp_obj_new_bytearray(len, data);
    m_del(uint8_t, data, cap);
    return out;
}

static mp_obj_t bmp565_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value) {
    if (value != MP_OBJ_SENTINEL) {
        if (value == MP_OBJ_NULL) {
            mp_raise_TypeError(MP_ERROR_TEXT("can't delete from BMP565"));
        }
        mp_raise_TypeError(MP_ERROR_TEXT("can't assign to BMP565"));
    }
    mp_obj_bmp565_t *self = MP_OBJ_TO_PTR(self_in);
    if (mp_obj_is_type(index, &mp_type_tuple)) {
        size_t len;
        mp_obj_t *items;
        mp_obj_get_array(index, &len, &items);
        if (len != 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid key"));
        }
        if (mp_obj_is_int(items[0]) && mp_obj_is_int(items[1])) {
            mp_int_t x = mp_obj_get_int(items[0]);
            mp_int_t y = mp_obj_get_int(items[1]);
            uint8_t out[2];
            size_t out_len = 0;
            int pos = y * self->bmp.width + x;
            if (gfx_bmp565_read_bytes(&self->bmp, pos, pos + 1, out, sizeof(out), &out_len) < 0) {
                mp_raise_ValueError(MP_ERROR_TEXT("BMP read failed"));
            }
            return mp_obj_new_int((mp_int_t)(out[0] | (out[1] << 8)));
        }
        mp_bound_slice_t xslice;
        mp_bound_slice_t yslice;
        mp_obj_slice_indices(items[0], self->bmp.width, &xslice);
        mp_obj_slice_indices(items[1], self->bmp.height, &yslice);
        return bmp565_get_region(self, xslice.start, xslice.stop, yslice.start, yslice.stop);
    }
    if (mp_obj_is_type(index, &mp_type_slice)) {
        mp_bound_slice_t yslice;
        mp_obj_slice_indices(index, self->bmp.height, &yslice);
        return bmp565_get_region(self, 0, self->bmp.width, yslice.start, yslice.stop);
    }
    if (mp_obj_is_int(index)) {
        mp_int_t key = mp_obj_get_int(index);
        uint8_t out[2];
        size_t out_len = 0;
        if (gfx_bmp565_read_bytes(&self->bmp, key, key + 1, out, sizeof(out), &out_len) < 0) {
            mp_raise_ValueError(MP_ERROR_TEXT("BMP read failed"));
        }
        return mp_obj_new_int((mp_int_t)(out[0] | (out[1] << 8)));
    }
    mp_raise_ValueError(MP_ERROR_TEXT("Invalid key"));
}

static mp_obj_t bmp565_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 4, 4, false);
    mp_obj_bmp565_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t x = mp_obj_get_int(args[0]);
    mp_int_t y = mp_obj_get_int(args[1]);
    mp_int_t w = mp_obj_get_int(args[2]);
    mp_int_t h = mp_obj_get_int(args[3]);
    return bmp565_get_region(self, x, x + w, y, y + h);
}

static mp_obj_t bmp565_deinit(mp_obj_t self_in) {
    mp_obj_bmp565_t *self = MP_OBJ_TO_PTR(self_in);
    gfx_bmp565_deinit(&self->bmp);
    self->buf_obj = mp_const_none;
    self->filename_obj = mp_const_none;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(bmp565_deinit_obj, bmp565_deinit);

static mp_obj_t bmp565_save(size_t n_args, const mp_obj_t *args) {
    mp_obj_bmp565_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *path = NULL;
    if (n_args >= 2 && args[1] != mp_const_none) {
        path = mp_obj_str_get_str(args[1]);
    } else if (self->filename_obj != mp_const_none) {
        path = mp_obj_str_get_str(self->filename_obj);
    }
    char out_path[512];
    if (gfx_bmp565_save_versioned(&self->bmp, path, out_path, sizeof(out_path)) < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("BMP save failed"));
    }
    return mp_obj_new_str(out_path, strlen(out_path));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(bmp565_save_obj, 1, 2, bmp565_save);

static mp_obj_t bmp565_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args_in) {
    /* Mirror Python BMP565.__init__(filename=None, source=None, streamed=False,
     * mirrored=False, width=None, height=None). A non-str filename is also
     * accepted as a buffer source for convenience. */
    enum { ARG_filename, ARG_source, ARG_streamed, ARG_mirrored, ARG_width, ARG_height };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_filename, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_source, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_streamed, MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_mirrored, MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_width, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_height, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args_in, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);

    mp_obj_t filename = mp_const_none;
    mp_obj_t source = parsed[ARG_source].u_obj;
    if (parsed[ARG_filename].u_obj != mp_const_none) {
        if (mp_obj_is_str(parsed[ARG_filename].u_obj)) {
            filename = parsed[ARG_filename].u_obj;
        } else if (source == mp_const_none) {
            source = parsed[ARG_filename].u_obj;
        }
    }
    mp_int_t width = parsed[ARG_width].u_int;
    mp_int_t height = parsed[ARG_height].u_int;

    mp_obj_bmp565_t *o = mp_obj_malloc(mp_obj_bmp565_t, &mp_type_bmp565);
    o->buf_obj = mp_const_none;
    o->filename_obj = filename;

    if (source != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(source, &bufinfo, MP_BUFFER_READ);
        o->buf_obj = source;
        gfx_bmp565_init_from_buffer(&o->bmp, bufinfo.buf, bufinfo.len, width, height);
    } else if (filename != mp_const_none) {
        const char *path = mp_obj_str_get_str(filename);
        if (parsed[ARG_streamed].u_bool) {
            if (gfx_bmp565_open_stream(path, &o->bmp) < 0) {
                mp_raise_ValueError(MP_ERROR_TEXT("BMP load failed"));
            }
            o->bmp.mirrored = parsed[ARG_mirrored].u_bool;
        } else {
            if (gfx_bmp565_load_from_file(path, &o->bmp) < 0) {
                mp_raise_ValueError(MP_ERROR_TEXT("BMP load failed"));
            }
            o->buf_obj = mp_obj_new_bytearray(o->bmp.buffer_len, o->bmp.buffer);
        }
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid arguments"));
    }
    return MP_OBJ_FROM_PTR(o);
}

static void bmp565_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_bmp565_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        switch (attr) {
            case MP_QSTR_width:
                dest[0] = mp_obj_new_int(self->bmp.width);
                break;
            case MP_QSTR_height:
                dest[0] = mp_obj_new_int(self->bmp.height);
                break;
            case MP_QSTR_bpp:
            case MP_QSTR_BPP:
                dest[0] = mp_obj_new_int(attr == MP_QSTR_bpp ? GFX_BMP565_BPP : GFX_BMP565_BYTES_PER_PIXEL);
                break;
            case MP_QSTR_buffer:
                dest[0] = self->buf_obj;
                break;
            default:
                dest[1] = MP_OBJ_SENTINEL;
                break;
        }
    }
}

static const mp_rom_map_elem_t bmp565_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&bmp565_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_save), MP_ROM_PTR(&bmp565_save_obj) },
};
static MP_DEFINE_CONST_DICT(bmp565_locals_dict, bmp565_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_bmp565,
    MP_QSTR_BMP565,
    MP_TYPE_FLAG_NONE,
    make_new, bmp565_make_new,
    attr, bmp565_attr,
    subscr, bmp565_subscr,
    call, bmp565_call,
    locals_dict, &bmp565_locals_dict
);

#define MOD_SHAPE_5(fn) \
static mp_obj_t mod_##fn(size_t n_args, const mp_obj_t *args) { \
    mp_canvas_slot_t slot; \
    if (!mp_canvas_resolve(args[0], &slot)) mp_raise_TypeError(MP_ERROR_TEXT("canvas required")); \
    gfx_area_t area = gfx_shapes_##fn(&slot.canvas, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), \
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4])); \
    return gfx_area_mp_from_gfx(&area); \
} \
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_##fn##_obj, 5, 5, mod_##fn);

static mp_obj_t mod_fill_rect(size_t n_args, const mp_obj_t *args) {
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(args[0], &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    const gfx_canvas_t *canvas = &slot.canvas;
    gfx_area_t area = gfx_shapes_fill_rect(canvas, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_fill_rect_obj, 6, 6, mod_fill_rect);

MOD_SHAPE_5(hline)
MOD_SHAPE_5(vline)

static mp_obj_t mod_line(size_t n_args, const mp_obj_t *args) {
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(args[0], &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    const gfx_canvas_t *canvas = &slot.canvas;
    gfx_area_t area = gfx_shapes_line(canvas, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_line_obj, 6, 6, mod_line);

static void get_readonly_framebuffer(mp_obj_t arg, mp_obj_framebuf_t *rofb) {
    mp_obj_t fb = mp_obj_cast_to_native_base(arg, MP_OBJ_FROM_PTR(&mp_type_framebuf));
    if (fb != MP_OBJ_NULL) {
        *rofb = *(mp_obj_framebuf_t *)MP_OBJ_TO_PTR(fb);
    } else {
        size_t len;
        mp_obj_t *items;
        mp_obj_get_array(arg, &len, &items);
        if (len < 4 || len > 5) {
            mp_raise_ValueError(NULL);
        }
        framebuf_make_new_helper(len, items, MP_BUFFER_READ, rofb);
    }
}

static mp_obj_t mod_ellipse(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_x, ARG_y, ARG_rx, ARG_ry, ARG_c, ARG_fill_pos, ARG_mask_pos, ARG_f, ARG_fill, ARG_m };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0x0f} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_m, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x0f} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    mp_int_t mask = (n_args > 7) ? parsed[ARG_mask_pos].u_int : parsed[ARG_m].u_int;
    gfx_area_t area = gfx_shapes_ellipse(&slot.canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_rx].u_int, parsed[ARG_ry].u_int, parsed[ARG_c].u_int, fill, mask);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_ellipse_obj, 6, mod_ellipse);

static mp_obj_t mod_arc(size_t n_args, const mp_obj_t *args) {
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(args[0], &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    const gfx_canvas_t *canvas = &slot.canvas;
    gfx_area_t area = gfx_shapes_arc(canvas, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), (float)mp_obj_get_float(args[4]), (float)mp_obj_get_float(args[5]), mp_obj_get_int(args[6]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_arc_obj, 7, 7, mod_arc);

static mp_obj_t mod_triangle(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_x0, ARG_y0, ARG_x1, ARG_y1, ARG_x2, ARG_y2, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_triangle(&slot.canvas,
        parsed[ARG_x0].u_int, parsed[ARG_y0].u_int, parsed[ARG_x1].u_int, parsed[ARG_y1].u_int,
        parsed[ARG_x2].u_int, parsed[ARG_y2].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_triangle_obj, 8, mod_triangle);

static mp_obj_t mod_gradient_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_x, ARG_y, ARG_w, ARG_h, ARG_c1, ARG_c2_pos, ARG_vert_pos, ARG_c2, ARG_vertical };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_, MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_c2, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_vertical, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    mp_obj_t c2_obj = (n_args > 6) ? parsed[ARG_c2_pos].u_obj : parsed[ARG_c2].u_obj;
    int c2 = (c2_obj == mp_const_none) ? parsed[ARG_c1].u_int : mp_obj_get_int(c2_obj);
    int vertical = (n_args > 7) ? parsed[ARG_vert_pos].u_bool : parsed[ARG_vertical].u_bool;
    gfx_area_t area = gfx_shapes_gradient_rect(&slot.canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_c1].u_int, c2, vertical);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_gradient_rect_obj, 6, mod_gradient_rect);

#if MICROPY_PY_ARRAY
static mp_obj_t mod_poly(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_x, ARG_y, ARG_coords, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(parsed[ARG_coords].u_obj, &bufinfo, MP_BUFFER_READ);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    char fmt[2] = { (char)bufinfo.typecode, '\0' };
    gfx_area_t area = gfx_shapes_poly(&slot.canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        bufinfo.buf, bufinfo.len, mp_binary_get_size('@', bufinfo.typecode, NULL), fmt,
        parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_poly_obj, 5, mod_poly);
#endif

static mp_obj_t mod_blit(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_dest, ARG_source, ARG_x, ARG_y, ARG_key_pos, ARG_palette_pos, ARG_key, ARG_palette };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_key, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_palette, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_framebuf_t dest;
    if (!mp_get_framebuf(parsed[ARG_dest].u_obj, &dest)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    mp_obj_framebuf_t source;
    get_readonly_framebuffer(parsed[ARG_source].u_obj, &source);
    mp_int_t key = (n_args > 4) ? parsed[ARG_key_pos].u_int : parsed[ARG_key].u_int;
    mp_obj_t pal_obj = (n_args > 5) ? parsed[ARG_palette_pos].u_obj : parsed[ARG_palette].u_obj;
    const gfx_fb_t *pal = NULL;
    mp_obj_framebuf_t palette;
    if (pal_obj != mp_const_none) {
        get_readonly_framebuffer(pal_obj, &palette);
        pal = &palette.fb;
    }
    gfx_area_t area = gfx_shapes_blit(&dest.canvas, &source.fb, parsed[ARG_x].u_int, parsed[ARG_y].u_int, key, pal);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_blit_obj, 4, mod_blit);

static mp_obj_t mod_blit_rect(size_t n_args, const mp_obj_t *args) {
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(args[0], &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    const gfx_canvas_t *canvas = &slot.canvas;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    gfx_area_t area = gfx_shapes_blit_rect(canvas, bufinfo.buf, mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]), 2);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_blit_rect_obj, 6, 6, mod_blit_rect);

static mp_obj_t mod_blit_transparent(size_t n_args, const mp_obj_t *args) {
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(args[0], &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    const gfx_canvas_t *canvas = &slot.canvas;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    gfx_area_t area = gfx_shapes_blit_transparent(canvas, bufinfo.buf, mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]), mp_obj_get_int(args[4]), mp_obj_get_int(args[5]),
        mp_obj_get_int(args[6]), 2);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_blit_transparent_obj, 7, 7, mod_blit_transparent);

static mp_obj_t mod_polygon(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_points, ARG_x, ARG_y, ARG_color, ARG_angle_pos, ARG_cx_pos, ARG_cy_pos, ARG_angle, ARG_center_x, ARG_center_y };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(0)} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_angle, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(0)} },
        { MP_QSTR_center_x, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_center_y, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    size_t len;
    mp_obj_t *items;
    mp_obj_get_array(parsed[ARG_points].u_obj, &len, &items);
    if (len < 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("Polygon must have at least 3 points"));
    }
    int points[128];
    if (len > 64) {
        mp_raise_ValueError(NULL);
    }
    for (size_t i = 0; i < len; i++) {
        size_t plen;
        mp_obj_t *pitems;
        mp_obj_get_array(items[i], &plen, &pitems);
        if (plen != 2) {
            mp_raise_ValueError(NULL);
        }
        points[i * 2] = mp_obj_get_int(pitems[0]);
        points[i * 2 + 1] = mp_obj_get_int(pitems[1]);
    }
    mp_obj_t angle_obj = (n_args > 5) ? parsed[ARG_angle_pos].u_obj : parsed[ARG_angle].u_obj;
    float angle = (float)mp_obj_get_float(angle_obj);
    int cx = (n_args > 6) ? parsed[ARG_cx_pos].u_int : parsed[ARG_center_x].u_int;
    int cy = (n_args > 7) ? parsed[ARG_cy_pos].u_int : parsed[ARG_center_y].u_int;
    gfx_area_t area = gfx_shapes_polygon(&slot.canvas, points, len, parsed[ARG_x].u_int,
        parsed[ARG_y].u_int, parsed[ARG_color].u_int, angle, cx, cy);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_polygon_obj, 5, mod_polygon);

static mp_obj_t mod_fill(mp_obj_t target, mp_obj_t col_in) {
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(target, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    const gfx_canvas_t *canvas = &slot.canvas;
    gfx_area_t area = gfx_shapes_fill(canvas, mp_obj_get_int(col_in));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mod_fill_obj, mod_fill);

static mp_obj_t mod_pixel(size_t n_args, const mp_obj_t *args) {
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(args[0], &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    const gfx_canvas_t *canvas = &slot.canvas;
    gfx_area_t area = gfx_shapes_pixel(canvas, mp_obj_get_int(args[1]),
        mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_pixel_obj, 4, 4, mod_pixel);

static mp_obj_t mod_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_x, ARG_y, ARG_w, ARG_h, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_rect(&slot.canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_rect_obj, 6, mod_rect);

static mp_obj_t mod_round_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_x, ARG_y, ARG_w, ARG_h, ARG_r, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_round_rect(&slot.canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_r].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_round_rect_obj, 7, mod_round_rect);

static mp_obj_t mod_circle(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_x, ARG_y, ARG_r, ARG_c, ARG_fill_pos, ARG_f, ARG_fill };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_f, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_fill, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_circle(&slot.canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_r].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_circle_obj, 5, mod_circle);

static mp_obj_t mod_text_height(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args, int height) {
    enum { ARG_canvas, ARG_s, ARG_x, ARG_y, ARG_c, ARG_scale, ARG_inverted, ARG_font_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_scale, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_inverted, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_font_data, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    gfx_font_t font;
    if (parsed[ARG_font_data].u_obj != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(parsed[ARG_font_data].u_obj, &bufinfo, MP_BUFFER_READ);
        gfx_font_init_from_data(&font, bufinfo.buf, bufinfo.len, height);
    } else {
        gfx_font_init_default(&font, height);
    }
    gfx_area_t area = gfx_font_text(&slot.canvas, &font, mp_obj_str_get_str(parsed[ARG_s].u_obj),
        parsed[ARG_x].u_int, parsed[ARG_y].u_int, parsed[ARG_c].u_int,
        parsed[ARG_scale].u_int, parsed[ARG_inverted].u_bool);
    if (font.owns_data) {
        gfx_font_deinit(&font);
    }
    return gfx_area_mp_from_gfx(&area);
}

static mp_obj_t mod_text8(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return mod_text_height(n_args, args, kw_args, 8);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_text8_obj, 4, mod_text8);

static mp_obj_t mod_text14(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return mod_text_height(n_args, args, kw_args, 14);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_text14_obj, 4, mod_text14);

static mp_obj_t mod_text16(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return mod_text_height(n_args, args, kw_args, 16);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_text16_obj, 4, mod_text16);

static mp_obj_t mod_text(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_canvas, ARG_s, ARG_x, ARG_y, ARG_c, ARG_scale, ARG_inverted, ARG_font_data, ARG_height };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_scale, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_inverted, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_font_data, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_height, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_int_t height = parsed[ARG_height].u_int;
    mp_canvas_slot_t slot;
    if (!mp_canvas_resolve(parsed[ARG_canvas].u_obj, &slot)) {
        mp_raise_TypeError(MP_ERROR_TEXT("canvas required"));
    }
    gfx_font_t font;
    if (parsed[ARG_font_data].u_obj != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(parsed[ARG_font_data].u_obj, &bufinfo, MP_BUFFER_READ);
        gfx_font_init_from_data(&font, bufinfo.buf, bufinfo.len, height);
    } else {
        gfx_font_init_default(&font, height);
    }
    gfx_area_t area = gfx_font_text(&slot.canvas, &font, mp_obj_str_get_str(parsed[ARG_s].u_obj),
        parsed[ARG_x].u_int, parsed[ARG_y].u_int, parsed[ARG_c].u_int,
        parsed[ARG_scale].u_int, parsed[ARG_inverted].u_bool);
    if (font.owns_data) {
        gfx_font_deinit(&font);
    }
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mod_text_obj, 4, mod_text);

static mp_obj_t mod_load_image(mp_obj_t path_in) {
    gfx_image_fb_t img;
    if (gfx_files_load_image(mp_obj_str_get_str(path_in), &img) < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("Unsupported image"));
    }
    mp_obj_t buf = mp_obj_new_bytearray(img.buffer_len, img.buffer);
    mp_obj_t tuple[4] = { buf, mp_obj_new_int(img.width), mp_obj_new_int(img.height), mp_obj_new_int(img.format) };
    mp_obj_t fb = framebuf_make_new_helper(4, tuple, MP_BUFFER_WRITE, NULL);
    if (img.owns_buffer) {
        free(img.buffer);
    }
    return fb;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mod_load_image_obj, mod_load_image);

static mp_obj_t mod_bmp_to_framebuffer(mp_obj_t path_in) {
    gfx_image_fb_t img;
    if (gfx_files_bmp_to_framebuffer(mp_obj_str_get_str(path_in), &img) < 0) {
        mp_raise_ValueError(NULL);
    }
    mp_obj_t buf = mp_obj_new_bytearray(img.buffer_len, img.buffer);
    mp_obj_t tuple[4] = { buf, mp_obj_new_int(img.width), mp_obj_new_int(img.height), mp_obj_new_int(GFX_RGB565) };
    mp_obj_t fb = framebuf_make_new_helper(4, tuple, MP_BUFFER_WRITE, NULL);
    free(img.buffer);
    return fb;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mod_bmp_to_framebuffer_obj, mod_bmp_to_framebuffer);

static mp_obj_t mod_pbm_to_framebuffer(mp_obj_t path_in) {
    gfx_image_fb_t img;
    if (gfx_files_pbm_to_framebuffer(mp_obj_str_get_str(path_in), &img) < 0) {
        mp_raise_ValueError(NULL);
    }
    mp_obj_t buf = mp_obj_new_bytearray(img.buffer_len, img.buffer);
    mp_obj_t tuple[4] = { buf, mp_obj_new_int(img.width), mp_obj_new_int(img.height), mp_obj_new_int(GFX_MHLSB) };
    mp_obj_t fb = framebuf_make_new_helper(4, tuple, MP_BUFFER_WRITE, NULL);
    free(img.buffer);
    return fb;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mod_pbm_to_framebuffer_obj, mod_pbm_to_framebuffer);

static mp_obj_t mod_pgm_to_framebuffer(mp_obj_t path_in) {
    gfx_image_fb_t img;
    if (gfx_files_pgm_to_framebuffer(mp_obj_str_get_str(path_in), &img) < 0) {
        mp_raise_ValueError(NULL);
    }
    mp_obj_t buf = mp_obj_new_bytearray(img.buffer_len, img.buffer);
    mp_obj_t tuple[4] = { buf, mp_obj_new_int(img.width), mp_obj_new_int(img.height), mp_obj_new_int(img.format) };
    mp_obj_t fb = framebuf_make_new_helper(4, tuple, MP_BUFFER_WRITE, NULL);
    free(img.buffer);
    return fb;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mod_pgm_to_framebuffer_obj, mod_pgm_to_framebuffer);

static mp_obj_t mod_save_image(size_t n_args, const mp_obj_t *args) {
    mp_obj_framebuf_t fb;
    if (!mp_get_framebuf(args[0], &fb)) {
        mp_raise_TypeError(MP_ERROR_TEXT("FrameBuffer required"));
    }
    const char *path = n_args >= 2 ? mp_obj_str_get_str(args[1]) : "screenshot";
    char out_path[256];
    if (gfx_files_save_image(&fb.fb, path, out_path, sizeof(out_path)) < 0) {
        mp_raise_ValueError(NULL);
    }
    return mp_obj_new_str(out_path, strlen(out_path));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_save_image_obj, 1, 2, mod_save_image);

static mp_obj_t mod_framebuf_backend(void) {
    return mp_obj_new_str(gfx_framebuf_backend(), strlen(gfx_framebuf_backend()));
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_framebuf_backend_obj, mod_framebuf_backend);

static mp_obj_t mod_implementation(void) {
    return mp_obj_new_str(gfx_implementation(), strlen(gfx_implementation()));
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_implementation_obj, mod_implementation);

static mp_obj_t mod_capabilities(void) {
    mp_obj_t caps = mp_obj_new_dict(0);
    mp_obj_dict_store(caps, MP_OBJ_NEW_QSTR(MP_QSTR_implementation), mod_implementation());
    mp_obj_dict_store(caps, MP_OBJ_NEW_QSTR(MP_QSTR_framebuf), mod_framebuf_backend());
    mp_obj_t formats = mp_obj_new_list(0, NULL);
    const char *names[] = {"MONO_VLSB", "MONO_HLSB", "MONO_HMSB", "RGB565", "GS2_HMSB", "GS4_HMSB", "GS8", "RGB888"};
    for (int i = 0; i < 8; i++) {
        mp_obj_list_append(formats, mp_obj_new_str(names[i], strlen(names[i])));
    }
    mp_obj_dict_store(caps, MP_OBJ_NEW_QSTR(MP_QSTR_formats), formats);
    return caps;
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_capabilities_obj, mod_capabilities);

static const mp_rom_map_elem_t graphics_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_graphics) },
    { MP_ROM_QSTR(MP_QSTR_FrameBuffer), MP_ROM_PTR(&mp_type_framebuf) },
    { MP_ROM_QSTR(MP_QSTR_Area), MP_ROM_PTR(&mp_type_area) },
    { MP_ROM_QSTR(MP_QSTR_Draw), MP_ROM_PTR(&mp_type_draw) },
    { MP_ROM_QSTR(MP_QSTR_ClipContext), MP_ROM_PTR(&mp_type_clip_ctx) },
    { MP_ROM_QSTR(MP_QSTR_ClippedCanvas), MP_ROM_PTR(&mp_type_clipped_canvas) },
    { MP_ROM_QSTR(MP_QSTR_Font), MP_ROM_PTR(&mp_type_font) },
    { MP_ROM_QSTR(MP_QSTR_BMP565), MP_ROM_PTR(&mp_type_bmp565) },
    { MP_ROM_QSTR(MP_QSTR_MONO_VLSB), MP_ROM_INT(GFX_MVLSB) },
    { MP_ROM_QSTR(MP_QSTR_RGB565), MP_ROM_INT(GFX_RGB565) },
    { MP_ROM_QSTR(MP_QSTR_GS2_HMSB), MP_ROM_INT(GFX_GS2_HMSB) },
    { MP_ROM_QSTR(MP_QSTR_GS4_HMSB), MP_ROM_INT(GFX_GS4_HMSB) },
    { MP_ROM_QSTR(MP_QSTR_GS8), MP_ROM_INT(GFX_GS8) },
    { MP_ROM_QSTR(MP_QSTR_MONO_HLSB), MP_ROM_INT(GFX_MHLSB) },
    { MP_ROM_QSTR(MP_QSTR_MONO_HMSB), MP_ROM_INT(GFX_MHMSB) },
    { MP_ROM_QSTR(MP_QSTR_RGB888), MP_ROM_INT(GFX_RGB888) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&mod_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&mod_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&mod_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&mod_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_round_rect), MP_ROM_PTR(&mod_round_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&mod_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&mod_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&mod_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&mod_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_ellipse), MP_ROM_PTR(&mod_ellipse_obj) },
    { MP_ROM_QSTR(MP_QSTR_arc), MP_ROM_PTR(&mod_arc_obj) },
    { MP_ROM_QSTR(MP_QSTR_triangle), MP_ROM_PTR(&mod_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_gradient_rect), MP_ROM_PTR(&mod_gradient_rect_obj) },
#if MICROPY_PY_ARRAY
    { MP_ROM_QSTR(MP_QSTR_poly), MP_ROM_PTR(&mod_poly_obj) },
#endif
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&mod_blit_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_rect), MP_ROM_PTR(&mod_blit_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_transparent), MP_ROM_PTR(&mod_blit_transparent_obj) },
    { MP_ROM_QSTR(MP_QSTR_polygon), MP_ROM_PTR(&mod_polygon_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&mod_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_text8), MP_ROM_PTR(&mod_text8_obj) },
    { MP_ROM_QSTR(MP_QSTR_text14), MP_ROM_PTR(&mod_text14_obj) },
    { MP_ROM_QSTR(MP_QSTR_text16), MP_ROM_PTR(&mod_text16_obj) },
    { MP_ROM_QSTR(MP_QSTR_load_image), MP_ROM_PTR(&mod_load_image_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_image), MP_ROM_PTR(&mod_save_image_obj) },
    { MP_ROM_QSTR(MP_QSTR_bmp_to_framebuffer), MP_ROM_PTR(&mod_bmp_to_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_pbm_to_framebuffer), MP_ROM_PTR(&mod_pbm_to_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_pgm_to_framebuffer), MP_ROM_PTR(&mod_pgm_to_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_framebuf_backend), MP_ROM_PTR(&mod_framebuf_backend_obj) },
    { MP_ROM_QSTR(MP_QSTR_implementation), MP_ROM_PTR(&mod_implementation_obj) },
    { MP_ROM_QSTR(MP_QSTR_capabilities), MP_ROM_PTR(&mod_capabilities_obj) },
};

MP_DEFINE_CONST_DICT(graphics_module_globals, graphics_module_globals_table);

const mp_obj_module_t mp_module_graphics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&graphics_module_globals,
};

#if !CIRCUITPY
MP_REGISTER_MODULE(MP_QSTR_graphics, mp_module_graphics);
#endif
