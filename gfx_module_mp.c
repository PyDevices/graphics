/*
 * graphics — MicroPython / CircuitPython module.
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "py/runtime.h"
#include "py/binary.h"

#include "gfx_core.h"
#include "gfx_framebuffer.h"
#include "gfx_shapes.h"
#include "gfx_font.h"
#include "gfx_capabilities.h"
#include "gfx_area_mp.h"
#include "gfx_bindings_mp.h"
#include "gfx_files.h"

const mp_obj_type_t mp_type_framebuf;

static mp_obj_framebuf_t *framebuf_from_obj(mp_obj_t obj) {
    mp_obj_t native = mp_obj_cast_to_native_base(obj, MP_OBJ_FROM_PTR(&mp_type_framebuf));
    if (native == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("FrameBuffer required"));
    }
    return MP_OBJ_TO_PTR(native);
}

mp_obj_t framebuf_make_new_helper(size_t n_args, const mp_obj_t *args_in, unsigned int buf_flags, mp_obj_framebuf_t *o) {
    mp_int_t width = mp_obj_get_int(args_in[1]);
    mp_int_t height = mp_obj_get_int(args_in[2]);
    mp_int_t format = mp_obj_get_int(args_in[3]);
    mp_int_t stride = n_args >= 5 ? mp_obj_get_int(args_in[4]) : width;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args_in[0], &bufinfo, buf_flags);

    if (gfx_fb_validate_buffer(bufinfo.len, width, height, format, stride) < 0) {
        mp_raise_ValueError(NULL);
    }

    if (o == NULL) {
        o = mp_obj_malloc(mp_obj_framebuf_t, &mp_type_framebuf);
    }
    o->buf_obj = args_in[0];
    o->fb.buf = bufinfo.buf;
    o->fb.width = width;
    o->fb.height = height;
    o->fb.format = format;
    o->fb.stride = stride;
    gfx_fb_canvas_init(&o->canvas, &o->fb);
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t framebuf_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args_in) {
    mp_arg_check_num(n_args, n_kw, 4, 5, false);
    mp_obj_framebuf_t *o = mp_obj_malloc(mp_obj_framebuf_t, type);
    return framebuf_make_new_helper(n_args, args_in, MP_BUFFER_WRITE, o);
}

static mp_obj_t framebuf_from_file_fun(mp_obj_t path_in) {
    gfx_image_fb_t img;
    if (gfx_files_load_image(mp_obj_str_get_str(path_in), &img) < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("Unsupported image"));
    }
    mp_obj_t buf = mp_obj_new_bytearray(img.buffer_len, img.buffer);
    mp_obj_t tuple[4] = {
        buf,
        mp_obj_new_int(img.width),
        mp_obj_new_int(img.height),
        mp_obj_new_int(img.format),
    };
    mp_obj_t fb = framebuf_make_new_helper(4, tuple, MP_BUFFER_WRITE, NULL);
    if (img.owns_buffer) {
        free(img.buffer);
    }
    return fb;
}
static MP_DEFINE_CONST_FUN_OBJ_1(framebuf_from_file_fun_obj, framebuf_from_file_fun);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(framebuf_from_file_obj, MP_ROM_PTR(&framebuf_from_file_fun_obj));

static void framebuf_args(const mp_obj_t *args_in, mp_int_t *args_out, int n) {
    for (int i = 0; i < n; ++i) {
        args_out[i] = mp_obj_get_int(args_in[i + 1]);
    }
}

static mp_int_t framebuf_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    mp_obj_framebuf_t *self = framebuf_from_obj(self_in);
    return mp_get_buffer(self->buf_obj, bufinfo, flags) ? 0 : 1;
}

static mp_obj_t framebuf_fill(mp_obj_t self_in, mp_obj_t col_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(self_in);
    gfx_area_t area = gfx_shapes_fill(&self->canvas, mp_obj_get_int(col_in));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_2(framebuf_fill_obj, framebuf_fill);

static mp_obj_t framebuf_fill_rect(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t args[5];
    framebuf_args(args_in, args, 5);
    gfx_area_t area = gfx_shapes_fill_rect(&self->canvas, args[0], args[1], args[2], args[3], args[4]);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_fill_rect_obj, 6, 6, framebuf_fill_rect);

static mp_obj_t framebuf_pixel(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t x = mp_obj_get_int(args_in[1]);
    mp_int_t y = mp_obj_get_int(args_in[2]);
    if (0 <= x && x < self->fb.width && 0 <= y && y < self->fb.height) {
        if (n_args == 3) {
            return MP_OBJ_NEW_SMALL_INT(gfx_fb_getpixel(&self->fb, x, y));
        }
        gfx_area_t area = gfx_shapes_pixel(&self->canvas, x, y, mp_obj_get_int(args_in[3]));
        return gfx_area_mp_from_gfx(&area);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_pixel_obj, 3, 4, framebuf_pixel);

static mp_obj_t framebuf_hline(size_t n_args, const mp_obj_t *args_in) {
    (void)n_args;
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t args[4];
    framebuf_args(args_in, args, 4);
    gfx_area_t area = gfx_shapes_hline(&self->canvas, args[0], args[1], args[2], args[3]);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_hline_obj, 5, 5, framebuf_hline);

static mp_obj_t framebuf_vline(size_t n_args, const mp_obj_t *args_in) {
    (void)n_args;
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t args[4];
    framebuf_args(args_in, args, 4);
    gfx_area_t area = gfx_shapes_vline(&self->canvas, args[0], args[1], args[2], args[3]);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_vline_obj, 5, 5, framebuf_vline);

static mp_obj_t framebuf_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_rect(&self->canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_rect_obj, 6, framebuf_rect);

static mp_obj_t framebuf_round_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_round_rect(&self->canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_r].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_round_rect_obj, 7, framebuf_round_rect);

static mp_obj_t framebuf_circle(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_circle(&self->canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_r].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_circle_obj, 5, framebuf_circle);

static mp_obj_t framebuf_line(size_t n_args, const mp_obj_t *args_in) {
    (void)n_args;
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t args[5];
    framebuf_args(args_in, args, 5);
    gfx_area_t area = gfx_shapes_line(&self->canvas, args[0], args[1], args[2], args[3], args[4]);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_line_obj, 6, 6, framebuf_line);

static mp_obj_t framebuf_ellipse(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_x, ARG_y, ARG_rx, ARG_ry, ARG_c, ARG_fill_pos, ARG_mask_pos, ARG_f, ARG_fill, ARG_m };
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    mp_int_t mask = (n_args > 7) ? parsed[ARG_mask_pos].u_int : parsed[ARG_m].u_int;
    gfx_area_t area = gfx_shapes_ellipse(&self->canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_rx].u_int, parsed[ARG_ry].u_int, parsed[ARG_c].u_int, fill, mask);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_ellipse_obj, 6, framebuf_ellipse);

#if MICROPY_PY_ARRAY
static mp_obj_t framebuf_poly(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(parsed[ARG_coords].u_obj, &bufinfo, MP_BUFFER_READ);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    size_t itemsize = mp_binary_get_size('@', bufinfo.typecode, NULL);
    char fmt[2] = { (char)bufinfo.typecode, '\0' };
    gfx_area_t area = gfx_shapes_poly(&self->canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        bufinfo.buf, bufinfo.len, itemsize, fmt, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_poly_obj, 5, framebuf_poly);
#endif

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

static mp_obj_t framebuf_blit(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
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
    gfx_area_t area = gfx_shapes_blit(&self->canvas, &source.fb, parsed[ARG_x].u_int, parsed[ARG_y].u_int, key, pal);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_blit_obj, 4, framebuf_blit);

static mp_obj_t framebuf_blit_rect(size_t n_args, const mp_obj_t *args_in) {
    (void)n_args;
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args_in[1], &bufinfo, MP_BUFFER_READ);
    gfx_area_t area = gfx_shapes_blit_rect(&self->canvas, bufinfo.buf, mp_obj_get_int(args_in[2]),
        mp_obj_get_int(args_in[3]), mp_obj_get_int(args_in[4]), mp_obj_get_int(args_in[5]), 2);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_blit_rect_obj, 6, 6, framebuf_blit_rect);

static mp_obj_t framebuf_blit_transparent(size_t n_args, const mp_obj_t *args_in) {
    (void)n_args;
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args_in[1], &bufinfo, MP_BUFFER_READ);
    gfx_area_t area = gfx_shapes_blit_transparent(&self->canvas, bufinfo.buf, mp_obj_get_int(args_in[2]),
        mp_obj_get_int(args_in[3]), mp_obj_get_int(args_in[4]), mp_obj_get_int(args_in[5]),
        mp_obj_get_int(args_in[6]), 2);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_blit_transparent_obj, 7, 7, framebuf_blit_transparent);

static mp_obj_t framebuf_arc(size_t n_args, const mp_obj_t *args_in) {
    (void)n_args;
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    gfx_area_t area = gfx_shapes_arc(&self->canvas, mp_obj_get_int(args_in[1]), mp_obj_get_int(args_in[2]),
        mp_obj_get_int(args_in[3]), (float)mp_obj_get_float(args_in[4]), (float)mp_obj_get_float(args_in[5]),
        mp_obj_get_int(args_in[6]));
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_arc_obj, 7, 7, framebuf_arc);

static mp_obj_t framebuf_triangle(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    bool fill = parsed[ARG_fill_pos].u_int || parsed[ARG_f].u_bool || parsed[ARG_fill].u_bool;
    gfx_area_t area = gfx_shapes_triangle(&self->canvas,
        parsed[ARG_x0].u_int, parsed[ARG_y0].u_int, parsed[ARG_x1].u_int, parsed[ARG_y1].u_int,
        parsed[ARG_x2].u_int, parsed[ARG_y2].u_int, parsed[ARG_c].u_int, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_triangle_obj, 8, framebuf_triangle);

static mp_obj_t framebuf_gradient_rect(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    mp_obj_t c2_obj = (n_args > 6) ? parsed[ARG_c2_pos].u_obj : parsed[ARG_c2].u_obj;
    int c2 = (c2_obj == mp_const_none) ? parsed[ARG_c1].u_int : mp_obj_get_int(c2_obj);
    int vertical = (n_args > 7) ? parsed[ARG_vert_pos].u_bool : parsed[ARG_vertical].u_bool;
    gfx_area_t area = gfx_shapes_gradient_rect(&self->canvas, parsed[ARG_x].u_int, parsed[ARG_y].u_int,
        parsed[ARG_w].u_int, parsed[ARG_h].u_int, parsed[ARG_c1].u_int, c2, vertical);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_gradient_rect_obj, 6, framebuf_gradient_rect);

static mp_obj_t framebuf_polygon(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
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
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
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
    gfx_area_t area = gfx_shapes_polygon(&self->canvas, points, len, parsed[ARG_x].u_int,
        parsed[ARG_y].u_int, parsed[ARG_color].u_int, angle, cx, cy);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_polygon_obj, 5, framebuf_polygon);

/* Shared textN helper: height fixed; accepts c=/scale=/inverted=/font_data=. */
static mp_obj_t framebuf_text_height(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args, int height) {
    enum { ARG_self, ARG_s, ARG_x, ARG_y, ARG_c, ARG_scale, ARG_inverted, ARG_font_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_INT, {.u_int = 1} },
        /* Positional scale/inverted/font_data match Python FrameBuffer.textN */
        { MP_QSTR_scale, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_inverted, MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_font_data, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    gfx_font_t font;
    if (parsed[ARG_font_data].u_obj != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(parsed[ARG_font_data].u_obj, &bufinfo, MP_BUFFER_READ);
        gfx_font_init_from_data(&font, bufinfo.buf, bufinfo.len, height);
    } else {
        gfx_font_init_default(&font, height);
    }
    gfx_area_t area = gfx_font_text(&self->canvas, &font, mp_obj_str_get_str(parsed[ARG_s].u_obj),
        parsed[ARG_x].u_int, parsed[ARG_y].u_int, parsed[ARG_c].u_int,
        parsed[ARG_scale].u_int, parsed[ARG_inverted].u_bool);
    if (font.owns_data) {
        gfx_font_deinit(&font);
    }
    return gfx_area_mp_from_gfx(&area);
}

static mp_obj_t framebuf_text(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_self, ARG_s, ARG_x, ARG_y, ARG_c, ARG_scale, ARG_inverted, ARG_font_data, ARG_height };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_c, MP_ARG_INT, {.u_int = 1} },
        /* Positional optional args match Python FrameBuffer.text */
        { MP_QSTR_scale, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_inverted, MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_font_data, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_height, MP_ARG_INT, {.u_int = 8} },
    };
    mp_arg_val_t parsed[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed);
    mp_obj_framebuf_t *self = framebuf_from_obj(parsed[ARG_self].u_obj);
    mp_int_t height = parsed[ARG_height].u_int;
    gfx_font_t font;
    if (parsed[ARG_font_data].u_obj != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(parsed[ARG_font_data].u_obj, &bufinfo, MP_BUFFER_READ);
        gfx_font_init_from_data(&font, bufinfo.buf, bufinfo.len, height);
    } else {
        gfx_font_init_default(&font, height);
    }
    gfx_area_t area = gfx_font_text(&self->canvas, &font, mp_obj_str_get_str(parsed[ARG_s].u_obj),
        parsed[ARG_x].u_int, parsed[ARG_y].u_int, parsed[ARG_c].u_int,
        parsed[ARG_scale].u_int, parsed[ARG_inverted].u_bool);
    if (font.owns_data) {
        gfx_font_deinit(&font);
    }
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_text_obj, 4, framebuf_text);

static mp_obj_t framebuf_text8(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return framebuf_text_height(n_args, args, kw_args, 8);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_text8_obj, 4, framebuf_text8);

static mp_obj_t framebuf_text14(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return framebuf_text_height(n_args, args, kw_args, 14);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_text14_obj, 4, framebuf_text14);

static mp_obj_t framebuf_text16(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return framebuf_text_height(n_args, args, kw_args, 16);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(framebuf_text16_obj, 4, framebuf_text16);

static mp_obj_t framebuf_scroll(mp_obj_t self_in, mp_obj_t xstep_in, mp_obj_t ystep_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(self_in);
    gfx_fb_scroll(&self->fb, mp_obj_get_int(xstep_in), mp_obj_get_int(ystep_in));
    gfx_area_t area = { 0, 0, self->fb.width, self->fb.height };
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_3(framebuf_scroll_obj, framebuf_scroll);

static mp_obj_t framebuf_save(size_t n_args, const mp_obj_t *args) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args[0]);
    const char *path = n_args >= 2 ? mp_obj_str_get_str(args[1]) : "screenshot";
    char out_path[256];
    if (gfx_files_save_image(&self->fb, path, out_path, sizeof(out_path)) < 0) {
        mp_raise_ValueError(NULL);
    }
    return mp_obj_new_str(out_path, strlen(out_path));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_save_obj, 1, 2, framebuf_save);

static void framebuf_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_framebuf_t *self = framebuf_from_obj(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        switch (attr) {
            case MP_QSTR_buffer:
                dest[0] = self->buf_obj;
                break;
            case MP_QSTR_width:
                dest[0] = mp_obj_new_int(self->fb.width);
                break;
            case MP_QSTR_height:
                dest[0] = mp_obj_new_int(self->fb.height);
                break;
            case MP_QSTR_format:
                dest[0] = mp_obj_new_int(self->fb.format);
                break;
            case MP_QSTR_color_depth:
                dest[0] = mp_obj_new_int(gfx_fb_color_depth(self->fb.format));
                break;
            default:
                dest[1] = MP_OBJ_SENTINEL;
                break;
        }
    }
}

static const mp_rom_map_elem_t framebuf_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_from_file), MP_ROM_PTR(&framebuf_from_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&framebuf_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&framebuf_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&framebuf_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&framebuf_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&framebuf_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&framebuf_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_round_rect), MP_ROM_PTR(&framebuf_round_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&framebuf_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&framebuf_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_ellipse), MP_ROM_PTR(&framebuf_ellipse_obj) },
    { MP_ROM_QSTR(MP_QSTR_arc), MP_ROM_PTR(&framebuf_arc_obj) },
    { MP_ROM_QSTR(MP_QSTR_triangle), MP_ROM_PTR(&framebuf_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_gradient_rect), MP_ROM_PTR(&framebuf_gradient_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_polygon), MP_ROM_PTR(&framebuf_polygon_obj) },
#if MICROPY_PY_ARRAY
    { MP_ROM_QSTR(MP_QSTR_poly), MP_ROM_PTR(&framebuf_poly_obj) },
#endif
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&framebuf_blit_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_rect), MP_ROM_PTR(&framebuf_blit_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_transparent), MP_ROM_PTR(&framebuf_blit_transparent_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&framebuf_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_text8), MP_ROM_PTR(&framebuf_text8_obj) },
    { MP_ROM_QSTR(MP_QSTR_text14), MP_ROM_PTR(&framebuf_text14_obj) },
    { MP_ROM_QSTR(MP_QSTR_text16), MP_ROM_PTR(&framebuf_text16_obj) },
    { MP_ROM_QSTR(MP_QSTR_scroll), MP_ROM_PTR(&framebuf_scroll_obj) },
    { MP_ROM_QSTR(MP_QSTR_save), MP_ROM_PTR(&framebuf_save_obj) },
};
static MP_DEFINE_CONST_DICT(framebuf_locals_dict, framebuf_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_framebuf,
    MP_QSTR_FrameBuffer,
    MP_TYPE_FLAG_NONE,
    make_new, framebuf_make_new,
    attr, framebuf_attr,
    buffer, framebuf_get_buffer,
    locals_dict, &framebuf_locals_dict
);
