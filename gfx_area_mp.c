/*
 * Native Area type for MicroPython / CircuitPython (graphics_native module).
 * SPDX-License-Identifier: MIT
 */

#include "py/runtime.h"
#include "py/obj.h"
#include "gfx_core.h"

typedef struct _mp_obj_area_t {
    mp_obj_base_t base;
    gfx_area_t area;
} mp_obj_area_t;

const mp_obj_type_t mp_type_area;

mp_obj_t gfx_area_mp_from_gfx(const gfx_area_t *a) {
    mp_obj_area_t *out = mp_obj_malloc(mp_obj_area_t, &mp_type_area);
    out->area = *a;
    return MP_OBJ_FROM_PTR(out);
}

static mp_obj_area_t *area_from_obj(mp_obj_t obj) {
    mp_obj_t native = mp_obj_cast_to_native_base(obj, MP_OBJ_FROM_PTR(&mp_type_area));
    if (native == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("Area required"));
    }
    return MP_OBJ_TO_PTR(native);
}

static gfx_area_t area_parse_args(size_t n_args, const mp_obj_t *args) {
    gfx_area_t a;
    if (n_args == 1 && mp_obj_is_type(args[0], &mp_type_tuple)) {
        size_t len;
        mp_obj_t *items;
        mp_obj_get_array(args[0], &len, &items);
        if (len != 4) {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid arguments"));
        }
        gfx_area_init(&a,
            mp_obj_get_int(items[0]),
            mp_obj_get_int(items[1]),
            mp_obj_get_int(items[2]),
            mp_obj_get_int(items[3]));
        return a;
    }
    if (n_args != 4) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid arguments"));
    }
    gfx_area_init(&a,
        mp_obj_get_int(args[0]),
        mp_obj_get_int(args[1]),
        mp_obj_get_int(args[2]),
        mp_obj_get_int(args[3]));
    return a;
}

static mp_obj_t area_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    (void)type;
    mp_arg_check_num(n_args, n_kw, 1, 4, false);
    mp_obj_area_t *o = mp_obj_malloc(mp_obj_area_t, &mp_type_area);
    o->area = area_parse_args(n_args, args);
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t area_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    mp_obj_area_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return (self->area.w > 0 && self->area.h > 0) ? mp_const_true : mp_const_false;
        default:
            return MP_OBJ_NULL;
    }
}

static mp_obj_t area_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    mp_obj_area_t *lhs = area_from_obj(lhs_in);
    if (op == MP_BINARY_OP_ADD) {
        mp_obj_area_t *rhs = area_from_obj(rhs_in);
        mp_obj_area_t *out = mp_obj_malloc(mp_obj_area_t, &mp_type_area);
        out->area = gfx_area_union(&lhs->area, &rhs->area);
        return MP_OBJ_FROM_PTR(out);
    }
    if (op == MP_BINARY_OP_EQUAL) {
        mp_obj_area_t *rhs = area_from_obj(rhs_in);
        bool eq = lhs->area.x == rhs->area.x && lhs->area.y == rhs->area.y
            && lhs->area.w == rhs->area.w && lhs->area.h == rhs->area.h;
        return eq ? mp_const_true : mp_const_false;
    }
    return MP_OBJ_NULL;
}

static void area_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    mp_obj_area_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL) {
        switch (attr) {
            case MP_QSTR_x: dest[0] = MP_OBJ_NEW_SMALL_INT(self->area.x); break;
            case MP_QSTR_y: dest[0] = MP_OBJ_NEW_SMALL_INT(self->area.y); break;
            case MP_QSTR_w: dest[0] = MP_OBJ_NEW_SMALL_INT(self->area.w); break;
            case MP_QSTR_h: dest[0] = MP_OBJ_NEW_SMALL_INT(self->area.h); break;
            default:
                dest[1] = MP_OBJ_SENTINEL;
                break;
        }
    }
}

static mp_obj_t area_contains(size_t n_args, const mp_obj_t *args) {
    mp_obj_area_t *self = area_from_obj(args[0]);
    if (n_args == 2 && mp_obj_is_type(args[1], &mp_type_tuple)) {
        size_t len;
        mp_obj_t *items;
        mp_obj_get_array(args[1], &len, &items);
        return gfx_area_contains_point(&self->area, mp_obj_get_int(items[0]), mp_obj_get_int(items[1]))
            ? mp_const_true : mp_const_false;
    }
    return gfx_area_contains_point(&self->area, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]))
        ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(area_contains_obj, 2, 3, area_contains);

static mp_obj_t area_contains_area(mp_obj_t self_in, mp_obj_t other_in) {
    mp_obj_area_t *self = area_from_obj(self_in);
    mp_obj_area_t *other = area_from_obj(other_in);
    return gfx_area_contains_area(&self->area, &other->area) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_2(area_contains_area_obj, area_contains_area);

static mp_obj_t area_intersects(mp_obj_t self_in, mp_obj_t other_in) {
    mp_obj_area_t *self = area_from_obj(self_in);
    mp_obj_area_t *other = area_from_obj(other_in);
    return gfx_area_intersects(&self->area, &other->area) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_2(area_intersects_obj, area_intersects);

static mp_obj_t area_touches_or_intersects(mp_obj_t self_in, mp_obj_t other_in) {
    mp_obj_area_t *self = area_from_obj(self_in);
    mp_obj_area_t *other = area_from_obj(other_in);
    return gfx_area_touches_or_intersects(&self->area, &other->area) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_2(area_touches_or_intersects_obj, area_touches_or_intersects);

static mp_obj_t area_shift(size_t n_args, const mp_obj_t *args) {
    mp_obj_area_t *self = area_from_obj(args[0]);
    mp_int_t dx = n_args > 1 ? mp_obj_get_int(args[1]) : 0;
    mp_int_t dy = n_args > 2 ? mp_obj_get_int(args[2]) : 0;
    mp_obj_area_t *out = mp_obj_malloc(mp_obj_area_t, &mp_type_area);
    out->area = gfx_area_shift(&self->area, dx, dy);
    return MP_OBJ_FROM_PTR(out);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(area_shift_obj, 1, 3, area_shift);

static mp_obj_t area_clip(mp_obj_t self_in, mp_obj_t other_in) {
    mp_obj_area_t *self = area_from_obj(self_in);
    mp_obj_area_t *other = area_from_obj(other_in);
    mp_obj_area_t *out = mp_obj_malloc(mp_obj_area_t, &mp_type_area);
    out->area = gfx_area_clip(&self->area, &other->area);
    return MP_OBJ_FROM_PTR(out);
}
static MP_DEFINE_CONST_FUN_OBJ_2(area_clip_obj, area_clip);

static mp_obj_t area_offset(size_t n_args, const mp_obj_t *args) {
    mp_obj_area_t *self = area_from_obj(args[0]);
    mp_int_t d1 = mp_obj_get_int(args[1]);
    mp_int_t d2 = n_args > 2 ? mp_obj_get_int(args[2]) : d1;
    mp_int_t d3 = n_args > 3 ? mp_obj_get_int(args[3]) : d1;
    mp_int_t d4 = n_args > 4 ? mp_obj_get_int(args[4]) : d2;
    mp_obj_area_t *out = mp_obj_malloc(mp_obj_area_t, &mp_type_area);
    out->area = gfx_area_offset(&self->area, d1, d2, d3, d4);
    return MP_OBJ_FROM_PTR(out);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(area_offset_obj, 2, 5, area_offset);

static mp_obj_t area_inset(size_t n_args, const mp_obj_t *args) {
    mp_obj_area_t *self = area_from_obj(args[0]);
    mp_int_t d1 = mp_obj_get_int(args[1]);
    mp_int_t d2 = n_args > 2 ? mp_obj_get_int(args[2]) : d1;
    mp_int_t d3 = n_args > 3 ? mp_obj_get_int(args[3]) : d1;
    mp_int_t d4 = n_args > 4 ? mp_obj_get_int(args[4]) : d2;
    mp_obj_area_t *out = mp_obj_malloc(mp_obj_area_t, &mp_type_area);
    out->area = gfx_area_inset(&self->area, d1, d2, d3, d4);
    return MP_OBJ_FROM_PTR(out);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(area_inset_obj, 2, 5, area_inset);

static mp_obj_t area_getiter(mp_obj_t self_in, mp_obj_iter_buf_t *iter_buf) {
    (void)iter_buf;
    mp_obj_area_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t items[4] = {
        MP_OBJ_NEW_SMALL_INT(self->area.x),
        MP_OBJ_NEW_SMALL_INT(self->area.y),
        MP_OBJ_NEW_SMALL_INT(self->area.w),
        MP_OBJ_NEW_SMALL_INT(self->area.h),
    };
    return mp_obj_tuple_getiter(mp_obj_new_tuple(4, items), iter_buf);
}

static void area_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_area_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Area(%d, %d, %d, %d)",
        (int)self->area.x, (int)self->area.y, (int)self->area.w, (int)self->area.h);
}

static const mp_rom_map_elem_t area_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_contains), MP_ROM_PTR(&area_contains_obj) },
    { MP_ROM_QSTR(MP_QSTR_contains_area), MP_ROM_PTR(&area_contains_area_obj) },
    { MP_ROM_QSTR(MP_QSTR_intersects), MP_ROM_PTR(&area_intersects_obj) },
    { MP_ROM_QSTR(MP_QSTR_touches_or_intersects), MP_ROM_PTR(&area_touches_or_intersects_obj) },
    { MP_ROM_QSTR(MP_QSTR_shift), MP_ROM_PTR(&area_shift_obj) },
    { MP_ROM_QSTR(MP_QSTR_clip), MP_ROM_PTR(&area_clip_obj) },
    { MP_ROM_QSTR(MP_QSTR_offset), MP_ROM_PTR(&area_offset_obj) },
    { MP_ROM_QSTR(MP_QSTR_inset), MP_ROM_PTR(&area_inset_obj) },
};
static MP_DEFINE_CONST_DICT(area_locals_dict, area_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_area,
    MP_QSTR_Area,
    MP_TYPE_FLAG_NONE,
    make_new, area_make_new,
    unary_op, area_unary_op,
    binary_op, area_binary_op,
    attr, area_attr,
    print, area_print,
    iter, area_getiter,
    locals_dict, &area_locals_dict
);
