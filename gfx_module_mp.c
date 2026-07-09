/*
 * graphics — MicroPython / CircuitPython module.
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
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

static mp_obj_t framebuf_rect(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t args[5];
    framebuf_args(args_in, args, 5);
    bool fill = n_args > 6 && mp_obj_is_true(args_in[6]);
    gfx_area_t area = gfx_shapes_rect(&self->canvas, args[0], args[1], args[2], args[3], args[4], fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_rect_obj, 6, 7, framebuf_rect);

static mp_obj_t framebuf_round_rect(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    bool fill = n_args > 7 && mp_obj_is_true(args_in[7]);
    gfx_area_t area = gfx_shapes_round_rect(&self->canvas, mp_obj_get_int(args_in[1]), mp_obj_get_int(args_in[2]),
        mp_obj_get_int(args_in[3]), mp_obj_get_int(args_in[4]), mp_obj_get_int(args_in[5]), mp_obj_get_int(args_in[6]), fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_round_rect_obj, 7, 8, framebuf_round_rect);

static mp_obj_t framebuf_line(size_t n_args, const mp_obj_t *args_in) {
    (void)n_args;
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t args[5];
    framebuf_args(args_in, args, 5);
    gfx_area_t area = gfx_shapes_line(&self->canvas, args[0], args[1], args[2], args[3], args[4]);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_line_obj, 6, 6, framebuf_line);

static mp_obj_t framebuf_ellipse(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t args[5];
    framebuf_args(args_in, args, 5);
    mp_int_t fill = n_args > 6 && mp_obj_is_true(args_in[6]);
    mp_int_t mask = n_args > 7 ? mp_obj_get_int(args_in[7]) : 0x0f;
    gfx_area_t area = gfx_shapes_ellipse(&self->canvas, args[0], args[1], args[2], args[3], args[4], fill, mask);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_ellipse_obj, 6, 8, framebuf_ellipse);

#if MICROPY_PY_ARRAY
static mp_obj_t framebuf_poly(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_int_t x = mp_obj_get_int(args_in[1]);
    mp_int_t y = mp_obj_get_int(args_in[2]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args_in[3], &bufinfo, MP_BUFFER_READ);
    mp_int_t col = mp_obj_get_int(args_in[4]);
    bool fill = n_args > 5 && mp_obj_is_true(args_in[5]);
    size_t itemsize = mp_binary_get_size('@', bufinfo.typecode, NULL);
    char fmt[2] = { (char)bufinfo.typecode, '\0' };
    gfx_area_t area = gfx_shapes_poly(
        &self->canvas, x, y, bufinfo.buf, bufinfo.len, itemsize, fmt, col, fill);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_poly_obj, 5, 6, framebuf_poly);
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

static mp_obj_t framebuf_blit(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    mp_obj_framebuf_t source;
    get_readonly_framebuffer(args_in[1], &source);
    mp_int_t x = mp_obj_get_int(args_in[2]);
    mp_int_t y = mp_obj_get_int(args_in[3]);
    mp_int_t key = n_args > 4 ? mp_obj_get_int(args_in[4]) : -1;
    mp_obj_framebuf_t palette;
    palette.fb.buf = NULL;
    const gfx_fb_t *pal = NULL;
    if (n_args > 5 && args_in[5] != mp_const_none) {
        get_readonly_framebuffer(args_in[5], &palette);
        pal = &palette.fb;
    }
    gfx_area_t area = gfx_shapes_blit(&self->canvas, &source.fb, x, y, key, pal);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_blit_obj, 4, 6, framebuf_blit);

static mp_obj_t framebuf_text(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    const char *str = mp_obj_str_get_str(args_in[1]);
    mp_int_t x0 = mp_obj_get_int(args_in[2]);
    mp_int_t y0 = mp_obj_get_int(args_in[3]);
    mp_int_t col = n_args >= 5 ? mp_obj_get_int(args_in[4]) : 1;
    gfx_area_t area = gfx_font_text8(&self->canvas, str, x0, y0, col);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_text_obj, 4, 5, framebuf_text);

static mp_obj_t framebuf_text16(size_t n_args, const mp_obj_t *args_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(args_in[0]);
    const char *str = mp_obj_str_get_str(args_in[1]);
    mp_int_t x0 = mp_obj_get_int(args_in[2]);
    mp_int_t y0 = mp_obj_get_int(args_in[3]);
    mp_int_t col = n_args >= 5 ? mp_obj_get_int(args_in[4]) : 1;
    gfx_area_t area = gfx_font_text16(&self->canvas, str, x0, y0, col);
    return gfx_area_mp_from_gfx(&area);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(framebuf_text16_obj, 4, 5, framebuf_text16);

static mp_obj_t framebuf_scroll(mp_obj_t self_in, mp_obj_t xstep_in, mp_obj_t ystep_in) {
    mp_obj_framebuf_t *self = framebuf_from_obj(self_in);
    gfx_fb_scroll(&self->fb, mp_obj_get_int(xstep_in), mp_obj_get_int(ystep_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(framebuf_scroll_obj, framebuf_scroll);

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
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&framebuf_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&framebuf_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&framebuf_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&framebuf_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&framebuf_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&framebuf_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_round_rect), MP_ROM_PTR(&framebuf_round_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&framebuf_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_ellipse), MP_ROM_PTR(&framebuf_ellipse_obj) },
#if MICROPY_PY_ARRAY
    { MP_ROM_QSTR(MP_QSTR_poly), MP_ROM_PTR(&framebuf_poly_obj) },
#endif
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&framebuf_blit_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&framebuf_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_text16), MP_ROM_PTR(&framebuf_text16_obj) },
    { MP_ROM_QSTR(MP_QSTR_scroll), MP_ROM_PTR(&framebuf_scroll_obj) },
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
