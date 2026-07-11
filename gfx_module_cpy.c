/*
 * graphics — CPython extension module.
 *
 * Full-parity binding surface matching the MicroPython/CircuitPython bindings
 * (gfx_module_mp.c / gfx_bindings_mp.c) and the pure-Python reference under
 * pydisplay src/lib/graphics. All shape/text/blit primitives share the same C
 * core (gfx_shapes/gfx_font/gfx_files/gfx_bmp565) as the MP bindings.
 *
 * SPDX-License-Identifier: MIT
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx_core.h"
#include "gfx_framebuffer.h"
#include "gfx_shapes.h"
#include "gfx_draw.h"
#include "gfx_font.h"
#include "gfx_capabilities.h"
#include "gfx_bmp565.h"
#include "gfx_files.h"

static PyTypeObject GfxAreaType;
static PyTypeObject GfxFrameBufferType;
static PyTypeObject GfxDrawType;
static PyTypeObject GfxFontType;
static PyTypeObject GfxBmp565Type;

/* ------------------------------------------------------------------------- */
/* Object structs (declared early so the canvas resolver can see them)       */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    PyObject *buf_obj;
    gfx_fb_t fb;
    gfx_canvas_t canvas;
} GfxFrameBufferObject;

typedef struct {
    PyObject_HEAD
    gfx_bmp565_t bmp;
    PyObject *buf_obj;
    PyObject *filename_obj;
    Py_buffer src_view;
    int has_src_view;
} GfxBmp565Object;

/* ------------------------------------------------------------------------- */
/* Area                                                                      */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    gfx_area_t area;
} GfxAreaObject;

static GfxAreaObject *area_check(PyObject *obj) {
    if (!PyObject_TypeCheck(obj, &GfxAreaType)) {
        PyErr_SetString(PyExc_TypeError, "Area required");
        return NULL;
    }
    return (GfxAreaObject *)obj;
}

static PyObject *area_from_gfx(const gfx_area_t *a) {
    GfxAreaObject *o = PyObject_New(GfxAreaObject, &GfxAreaType);
    if (!o) {
        return NULL;
    }
    o->area = *a;
    return (PyObject *)o;
}

/* Build an Area result, but surface any pending Python error first (a Python
 * duck-typed canvas callback may have raised during the C drawing loop). */
#define RETURN_AREA(a) do { if (PyErr_Occurred()) { return NULL; } return area_from_gfx(&(a)); } while (0)

static int area_parse_sequence(PyObject *seq, gfx_area_t *out) {
    PyObject *fast = PySequence_Fast(seq, "expected sequence");
    if (!fast) {
        return -1;
    }
    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast);
    if (len != 4) {
        Py_DECREF(fast);
        PyErr_SetString(PyExc_ValueError, "Invalid arguments");
        return -1;
    }
    PyObject **items = PySequence_Fast_ITEMS(fast);
    for (int i = 0; i < 4; ++i) {
        long v = PyLong_AsLong(items[i]);
        if (v == -1 && PyErr_Occurred()) {
            Py_DECREF(fast);
            return -1;
        }
        ((int32_t *)&out->x)[i] = (int32_t)v;
    }
    Py_DECREF(fast);
    return 0;
}

static PyObject *area_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    (void)kwds;
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    gfx_area_t a;
    if (n == 1) {
        if (area_parse_sequence(PyTuple_GET_ITEM(args, 0), &a) < 0) {
            return NULL;
        }
    } else if (n == 4) {
        for (int i = 0; i < 4; ++i) {
            long v = PyLong_AsLong(PyTuple_GET_ITEM(args, i));
            if (v == -1 && PyErr_Occurred()) {
                return NULL;
            }
            ((int32_t *)&a.x)[i] = (int32_t)v;
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "Invalid arguments");
        return NULL;
    }
    GfxAreaObject *o = (GfxAreaObject *)type->tp_alloc(type, 0);
    if (!o) {
        return NULL;
    }
    o->area = a;
    return (PyObject *)o;
}

static void area_dealloc(GfxAreaObject *self) {
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *area_repr(GfxAreaObject *self) {
    return PyUnicode_FromFormat(
        "Area(%d, %d, %d, %d)",
        (int)self->area.x, (int)self->area.y, (int)self->area.w, (int)self->area.h);
}

static PyObject *area_str(GfxAreaObject *self) {
    return area_repr(self);
}

static PyObject *area_iter(GfxAreaObject *self) {
    PyObject *items[4] = {
        PyLong_FromLong(self->area.x),
        PyLong_FromLong(self->area.y),
        PyLong_FromLong(self->area.w),
        PyLong_FromLong(self->area.h),
    };
    for (int i = 0; i < 4; ++i) {
        if (!items[i]) {
            for (int j = 0; j < i; ++j) {
                Py_DECREF(items[j]);
            }
            return NULL;
        }
    }
    PyObject *t = PyTuple_Pack(4, items[0], items[1], items[2], items[3]);
    for (int i = 0; i < 4; ++i) {
        Py_DECREF(items[i]);
    }
    if (!t) {
        return NULL;
    }
    PyObject *it = PyObject_GetIter(t);
    Py_DECREF(t);
    return it;
}

static PyObject *area_richcompare(PyObject *self, PyObject *other, int op) {
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (!PyObject_TypeCheck(other, &GfxAreaType)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    GfxAreaObject *a = (GfxAreaObject *)self;
    GfxAreaObject *b = (GfxAreaObject *)other;
    bool eq = a->area.x == b->area.x && a->area.y == b->area.y
        && a->area.w == b->area.w && a->area.h == b->area.h;
    if (op == Py_NE) {
        eq = !eq;
    }
    return PyBool_FromLong(eq);
}

static PyObject *area_add(PyObject *lhs, PyObject *rhs) {
    GfxAreaObject *a = area_check(lhs);
    GfxAreaObject *b = area_check(rhs);
    if (!a || !b) {
        return NULL;
    }
    gfx_area_t result = gfx_area_union(&a->area, &b->area);
    return area_from_gfx(&result);
}

static PyObject *area_bool(GfxAreaObject *self) {
    return PyBool_FromLong(self->area.w > 0 && self->area.h > 0);
}

static PyObject *area_get_x(GfxAreaObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->area.x);
}
static PyObject *area_get_y(GfxAreaObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->area.y);
}
static PyObject *area_get_w(GfxAreaObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->area.w);
}
static PyObject *area_get_h(GfxAreaObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->area.h);
}

static PyGetSetDef area_getset[] = {
    {"x", (getter)area_get_x, NULL, NULL},
    {"y", (getter)area_get_y, NULL, NULL},
    {"w", (getter)area_get_w, NULL, NULL},
    {"h", (getter)area_get_h, NULL, NULL},
    {NULL},
};

static PyObject *area_contains(PyObject *self, PyObject *args) {
    GfxAreaObject *o = area_check(self);
    if (!o) {
        return NULL;
    }
    PyObject *arg1;
    PyObject *arg2 = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &arg1, &arg2)) {
        return NULL;
    }
    int32_t px, py;
    if (arg2 == NULL && PyTuple_Check(arg1)) {
        PyObject *fast = PySequence_Fast(arg1, "expected tuple");
        if (!fast) {
            return NULL;
        }
        if (PySequence_Fast_GET_SIZE(fast) != 2) {
            Py_DECREF(fast);
            PyErr_SetString(PyExc_ValueError, "Invalid arguments");
            return NULL;
        }
        PyObject **items = PySequence_Fast_ITEMS(fast);
        long x = PyLong_AsLong(items[0]);
        long y = PyLong_AsLong(items[1]);
        Py_DECREF(fast);
        if ((x == -1 || y == -1) && PyErr_Occurred()) {
            return NULL;
        }
        px = (int32_t)x;
        py = (int32_t)y;
    } else if (arg2 != NULL) {
        long x = PyLong_AsLong(arg1);
        long y = PyLong_AsLong(arg2);
        if ((x == -1 || y == -1) && PyErr_Occurred()) {
            return NULL;
        }
        px = (int32_t)x;
        py = (int32_t)y;
    } else {
        PyErr_SetString(PyExc_TypeError, "Invalid arguments");
        return NULL;
    }
    return PyBool_FromLong(gfx_area_contains_point(&o->area, px, py));
}

static PyObject *area_contains_area(PyObject *self, PyObject *other) {
    GfxAreaObject *a = area_check(self);
    GfxAreaObject *b = area_check(other);
    if (!a || !b) {
        return NULL;
    }
    return PyBool_FromLong(gfx_area_contains_area(&a->area, &b->area));
}

static PyObject *area_intersects(PyObject *self, PyObject *other) {
    GfxAreaObject *a = area_check(self);
    GfxAreaObject *b = area_check(other);
    if (!a || !b) {
        return NULL;
    }
    return PyBool_FromLong(gfx_area_intersects(&a->area, &b->area));
}

static PyObject *area_touches_or_intersects(PyObject *self, PyObject *other) {
    GfxAreaObject *a = area_check(self);
    GfxAreaObject *b = area_check(other);
    if (!a || !b) {
        return NULL;
    }
    return PyBool_FromLong(gfx_area_touches_or_intersects(&a->area, &b->area));
}

static PyObject *area_shift(PyObject *self, PyObject *args) {
    GfxAreaObject *o = area_check(self);
    if (!o) {
        return NULL;
    }
    int dx = 0;
    int dy = 0;
    if (!PyArg_ParseTuple(args, "|ii", &dx, &dy)) {
        return NULL;
    }
    gfx_area_t result = gfx_area_shift(&o->area, dx, dy);
    return area_from_gfx(&result);
}

static PyObject *area_clip(PyObject *self, PyObject *other) {
    GfxAreaObject *a = area_check(self);
    GfxAreaObject *b = area_check(other);
    if (!a || !b) {
        return NULL;
    }
    gfx_area_t result = gfx_area_clip(&a->area, &b->area);
    return area_from_gfx(&result);
}

static PyObject *area_offset(PyObject *self, PyObject *args) {
    GfxAreaObject *o = area_check(self);
    if (!o) {
        return NULL;
    }
    int d1, d2, d3, d4;
    if (!PyArg_ParseTuple(args, "i|iii", &d1, &d2, &d3, &d4)) {
        return NULL;
    }
    if (PyTuple_GET_SIZE(args) < 2) {
        d2 = d1;
    }
    if (PyTuple_GET_SIZE(args) < 3) {
        d3 = d1;
    }
    if (PyTuple_GET_SIZE(args) < 4) {
        d4 = d2;
    }
    gfx_area_t result = gfx_area_offset(&o->area, d1, d2, d3, d4);
    return area_from_gfx(&result);
}

static PyObject *area_inset(PyObject *self, PyObject *args) {
    GfxAreaObject *o = area_check(self);
    if (!o) {
        return NULL;
    }
    int d1, d2, d3, d4;
    if (!PyArg_ParseTuple(args, "i|iii", &d1, &d2, &d3, &d4)) {
        return NULL;
    }
    if (PyTuple_GET_SIZE(args) < 2) {
        d2 = d1;
    }
    if (PyTuple_GET_SIZE(args) < 3) {
        d3 = d1;
    }
    if (PyTuple_GET_SIZE(args) < 4) {
        d4 = d2;
    }
    gfx_area_t result = gfx_area_inset(&o->area, d1, d2, d3, d4);
    return area_from_gfx(&result);
}

static PyMethodDef area_methods[] = {
    {"contains", area_contains, METH_VARARGS, NULL},
    {"contains_area", area_contains_area, METH_O, NULL},
    {"intersects", area_intersects, METH_O, NULL},
    {"touches_or_intersects", area_touches_or_intersects, METH_O, NULL},
    {"shift", area_shift, METH_VARARGS, NULL},
    {"clip", area_clip, METH_O, NULL},
    {"offset", area_offset, METH_VARARGS, NULL},
    {"inset", area_inset, METH_VARARGS, NULL},
    {NULL},
};

static PyNumberMethods area_as_number = {
    .nb_add = area_add,
    .nb_bool = (inquiry)area_bool,
};

static PyTypeObject GfxAreaType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "graphics.Area",
    .tp_basicsize = sizeof(GfxAreaObject),
    .tp_dealloc = (destructor)area_dealloc,
    .tp_repr = (reprfunc)area_repr,
    .tp_str = (reprfunc)area_str,
    .tp_as_number = &area_as_number,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = area_methods,
    .tp_getset = area_getset,
    .tp_new = area_new,
    .tp_iter = (getiterfunc)area_iter,
    .tp_richcompare = area_richcompare,
    .tp_hash = PyObject_HashNotImplemented,
};

/* ------------------------------------------------------------------------- */
/* Duck-typed canvas resolution (native FrameBuffer, BMP565 buffer, Python)  */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject *obj;
} cpy_py_canvas_ctx_t;

typedef struct {
    gfx_fb_t fb;
    cpy_py_canvas_ctx_t py;
    gfx_canvas_t canvas;
} cpy_canvas_slot_t;

static int cpy_py_has_method(PyObject *o, const char *name) {
    PyObject *a = PyObject_GetAttrString(o, name);
    if (!a) {
        PyErr_Clear();
        return 0;
    }
    int r = PyCallable_Check(a);
    Py_DECREF(a);
    return r;
}

static int cpy_py_attr_int(PyObject *o, const char *name, int *out) {
    PyObject *a = PyObject_GetAttrString(o, name);
    if (!a) {
        return -1;
    }
    long v = PyLong_AsLong(a);
    Py_DECREF(a);
    if (v == -1 && PyErr_Occurred()) {
        return -1;
    }
    *out = (int)v;
    return 0;
}

static int py_canvas_pixel(void *ctx, int x, int y, int c, int set) {
    cpy_py_canvas_ctx_t *pc = (cpy_py_canvas_ctx_t *)ctx;
    if (!set || PyErr_Occurred()) {
        return 0;
    }
    PyObject *r = PyObject_CallMethod(pc->obj, "pixel", "iii", x, y, c);
    Py_XDECREF(r);
    return 0;
}

static void py_canvas_fill_rect(void *ctx, int x, int y, int w, int h, int c) {
    cpy_py_canvas_ctx_t *pc = (cpy_py_canvas_ctx_t *)ctx;
    if (PyErr_Occurred()) {
        return;
    }
    if (cpy_py_has_method(pc->obj, "fill_rect")) {
        PyObject *r = PyObject_CallMethod(pc->obj, "fill_rect", "iiiii", x, y, w, h, c);
        Py_XDECREF(r);
        return;
    }
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            py_canvas_pixel(ctx, i, j, c, 1);
        }
    }
}

static void py_canvas_hline(void *ctx, int x, int y, int w, int c) {
    cpy_py_canvas_ctx_t *pc = (cpy_py_canvas_ctx_t *)ctx;
    if (PyErr_Occurred()) {
        return;
    }
    if (cpy_py_has_method(pc->obj, "hline")) {
        PyObject *r = PyObject_CallMethod(pc->obj, "hline", "iiii", x, y, w, c);
        Py_XDECREF(r);
        return;
    }
    py_canvas_fill_rect(ctx, x, y, w, 1, c);
}

static void py_canvas_vline(void *ctx, int x, int y, int h, int c) {
    cpy_py_canvas_ctx_t *pc = (cpy_py_canvas_ctx_t *)ctx;
    if (PyErr_Occurred()) {
        return;
    }
    if (cpy_py_has_method(pc->obj, "vline")) {
        PyObject *r = PyObject_CallMethod(pc->obj, "vline", "iiii", x, y, h, c);
        Py_XDECREF(r);
        return;
    }
    py_canvas_fill_rect(ctx, x, y, 1, h, c);
}

/* Resolve a drawing target into a gfx_canvas_t. slot must outlive the draw
 * call (its py ctx is referenced by canvas.ctx). Returns 0 on success. */
static int cpy_canvas_resolve(PyObject *target, cpy_canvas_slot_t *slot) {
    if (PyObject_TypeCheck(target, &GfxFrameBufferType)) {
        GfxFrameBufferObject *fb = (GfxFrameBufferObject *)target;
        slot->canvas = fb->canvas;
        return 0;
    }
    if (PyObject_TypeCheck(target, &GfxBmp565Type)) {
        GfxBmp565Object *b = (GfxBmp565Object *)target;
        if (!b->bmp.streamed && b->bmp.buffer) {
            slot->fb.buf = b->bmp.buffer;
            slot->fb.width = (uint16_t)b->bmp.width;
            slot->fb.height = (uint16_t)b->bmp.height;
            slot->fb.stride = (uint16_t)b->bmp.width;
            slot->fb.format = GFX_RGB565;
            gfx_fb_canvas_init(&slot->canvas, &slot->fb);
            return 0;
        }
    }
    if (cpy_py_has_method(target, "fill_rect") || cpy_py_has_method(target, "pixel")) {
        int w, h;
        if (cpy_py_attr_int(target, "width", &w) < 0 || cpy_py_attr_int(target, "height", &h) < 0) {
            return -1;
        }
        slot->py.obj = target;
        slot->canvas.ctx = &slot->py;
        slot->canvas.width = w;
        slot->canvas.height = h;
        slot->canvas.pixel = py_canvas_pixel;
        slot->canvas.hline = py_canvas_hline;
        slot->canvas.vline = py_canvas_vline;
        slot->canvas.fill_rect = py_canvas_fill_rect;
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "FrameBuffer or canvas required");
    return -1;
}

/* Read-only framebuffer source for blit (native FrameBuffer or (buf,w,h,fmt[,stride])). */
static int get_readonly_framebuffer(PyObject *arg, gfx_fb_t *fb_out, Py_buffer *view, int *have_view) {
    *have_view = 0;
    if (PyObject_TypeCheck(arg, &GfxFrameBufferType)) {
        *fb_out = ((GfxFrameBufferObject *)arg)->fb;
        return 0;
    }
    PyObject *fast = PySequence_Fast(arg, "expected FrameBuffer or sequence");
    if (!fast) {
        return -1;
    }
    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast);
    if (len < 4 || len > 5) {
        Py_DECREF(fast);
        PyErr_SetString(PyExc_ValueError, "invalid framebuffer parameters");
        return -1;
    }
    PyObject **items = PySequence_Fast_ITEMS(fast);
    if (PyObject_GetBuffer(items[0], view, PyBUF_SIMPLE) < 0) {
        Py_DECREF(fast);
        return -1;
    }
    long width = PyLong_AsLong(items[1]);
    long height = PyLong_AsLong(items[2]);
    long format = PyLong_AsLong(items[3]);
    long stride = len >= 5 ? PyLong_AsLong(items[4]) : width;
    Py_DECREF(fast);
    if ((width == -1 || height == -1 || format == -1 || stride == -1) && PyErr_Occurred()) {
        PyBuffer_Release(view);
        return -1;
    }
    if (gfx_fb_validate_buffer(view->len, (int)width, (int)height, (int)format, (int)stride) < 0) {
        PyBuffer_Release(view);
        PyErr_SetString(PyExc_ValueError, "invalid framebuffer parameters");
        return -1;
    }
    fb_out->buf = view->buf;
    fb_out->width = (uint16_t)width;
    fb_out->height = (uint16_t)height;
    fb_out->stride = (uint16_t)stride;
    fb_out->format = (uint8_t)format;
    *have_view = 1;
    return 0;
}

/* Parse a sequence of (x, y) points into a flat int array (max 64 points). */
static int parse_points(PyObject *seq, int points[128], size_t *out_len) {
    PyObject *fast = PySequence_Fast(seq, "expected sequence of points");
    if (!fast) {
        return -1;
    }
    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast);
    if (len < 3) {
        Py_DECREF(fast);
        PyErr_SetString(PyExc_ValueError, "Polygon must have at least 3 points");
        return -1;
    }
    if (len > 64) {
        Py_DECREF(fast);
        PyErr_SetString(PyExc_ValueError, "Polygon must have at most 64 points");
        return -1;
    }
    PyObject **items = PySequence_Fast_ITEMS(fast);
    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *pf = PySequence_Fast(items[i], "expected sequence of (x, y)");
        if (!pf) {
            Py_DECREF(fast);
            return -1;
        }
        if (PySequence_Fast_GET_SIZE(pf) != 2) {
            Py_DECREF(pf);
            Py_DECREF(fast);
            PyErr_SetString(PyExc_ValueError, "expected sequence of (x, y) points");
            return -1;
        }
        PyObject **pi = PySequence_Fast_ITEMS(pf);
        points[i * 2] = (int)PyLong_AsLong(pi[0]);
        points[i * 2 + 1] = (int)PyLong_AsLong(pi[1]);
        Py_DECREF(pf);
        if (PyErr_Occurred()) {
            Py_DECREF(fast);
            return -1;
        }
    }
    *out_len = (size_t)len;
    Py_DECREF(fast);
    return 0;
}

/* Draw text with the given canvas; font_data (buffer) optional, else default. */
static PyObject *do_text(const gfx_canvas_t *canvas, const char *s, int x, int y,
                         int c, int scale, int inverted, PyObject *font_data, int height) {
    gfx_font_t font;
    Py_buffer view;
    int have_view = 0;
    if (font_data && font_data != Py_None) {
        if (PyObject_GetBuffer(font_data, &view, PyBUF_SIMPLE) < 0) {
            return NULL;
        }
        gfx_font_init_from_data(&font, view.buf, (size_t)view.len, height);
        have_view = 1;
    } else {
        gfx_font_init_default(&font, height);
    }
    gfx_area_t area = gfx_font_text(canvas, &font, s, x, y, c, scale, inverted);
    if (font.owns_data) {
        gfx_font_deinit(&font);
    }
    if (have_view) {
        PyBuffer_Release(&view);
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    return area_from_gfx(&area);
}

/* ------------------------------------------------------------------------- */
/* FrameBuffer                                                               */
/* ------------------------------------------------------------------------- */

static int framebuffer_init_from_buffer(PyObject *buf_obj, int width, int height, int format, int stride, GfxFrameBufferObject *o) {
    Py_buffer view;
    if (PyObject_GetBuffer(buf_obj, &view, PyBUF_WRITABLE) < 0) {
        return -1;
    }
    if (gfx_fb_validate_buffer(view.len, width, height, format, stride) < 0) {
        PyBuffer_Release(&view);
        PyErr_SetString(PyExc_ValueError, "invalid framebuffer parameters");
        return -1;
    }
    Py_XDECREF(o->buf_obj);
    o->buf_obj = buf_obj;
    Py_INCREF(buf_obj);
    o->fb.buf = view.buf;
    o->fb.width = (uint16_t)width;
    o->fb.height = (uint16_t)height;
    o->fb.stride = (uint16_t)stride;
    o->fb.format = (uint8_t)format;
    PyBuffer_Release(&view);
    gfx_fb_canvas_init(&o->canvas, &o->fb);
    return 0;
}

static PyObject *framebuffer_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    (void)args;
    (void)kwds;
    return type->tp_alloc(type, 0);
}

static void framebuffer_dealloc(GfxFrameBufferObject *self) {
    Py_XDECREF(self->buf_obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int framebuffer_init(PyObject *obj, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"buffer", "width", "height", "format", "stride", NULL};
    PyObject *buf;
    int width, height, format, stride = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiii|i", kwlist, &buf, &width, &height, &format, &stride)) {
        return -1;
    }
    if (stride < 0) {
        stride = width;
    }
    return framebuffer_init_from_buffer(buf, width, height, format, stride, (GfxFrameBufferObject *)obj);
}

static PyObject *framebuffer_from_image(gfx_image_fb_t *img) {
    int width = img->width;
    int height = img->height;
    int format = img->format;
    PyObject *buf = PyByteArray_FromStringAndSize((const char *)img->buffer, (Py_ssize_t)img->buffer_len);
    gfx_image_fb_free(img);
    if (!buf) {
        return NULL;
    }
    GfxFrameBufferObject *fb = PyObject_New(GfxFrameBufferObject, &GfxFrameBufferType);
    if (!fb) {
        Py_DECREF(buf);
        return NULL;
    }
    fb->buf_obj = NULL;
    if (framebuffer_init_from_buffer(buf, width, height, format, width, fb) < 0) {
        Py_DECREF(fb);
        Py_DECREF(buf);
        return NULL;
    }
    Py_DECREF(buf);
    return (PyObject *)fb;
}

static PyObject *framebuffer_fill(GfxFrameBufferObject *self, PyObject *args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_fill(&self->canvas, col);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_fill_rect(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, w, h, col;
    if (!PyArg_ParseTuple(args, "iiiii", &x, &y, &w, &h, &col)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_fill_rect(&self->canvas, x, y, w, h, col);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_pixel(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, col = 0;
    if (!PyArg_ParseTuple(args, "ii|i", &x, &y, &col)) {
        return NULL;
    }
    if (PyTuple_GET_SIZE(args) < 3) {
        if (0 <= x && x < self->fb.width && 0 <= y && y < self->fb.height) {
            return PyLong_FromUnsignedLong(gfx_fb_getpixel(&self->fb, (unsigned int)x, (unsigned int)y));
        }
        Py_RETURN_NONE;
    }
    if (0 <= x && x < self->fb.width && 0 <= y && y < self->fb.height) {
        gfx_area_t result = gfx_shapes_pixel(&self->canvas, x, y, col);
        return area_from_gfx(&result);
    }
    Py_RETURN_NONE;
}

static PyObject *framebuffer_hline(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, w, col;
    if (!PyArg_ParseTuple(args, "iiii", &x, &y, &w, &col)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_hline(&self->canvas, x, y, w, col);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_vline(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, h, col;
    if (!PyArg_ParseTuple(args, "iiii", &x, &y, &h, &col)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_vline(&self->canvas, x, y, h, col);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_rect(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "w", "h", "c", "f", "fill", NULL};
    int x, y, w, h, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiii|pp", kwlist, &x, &y, &w, &h, &col, &f, &fill)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_rect(&self->canvas, x, y, w, h, col, f || fill);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_round_rect(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "w", "h", "r", "c", "f", "fill", NULL};
    int x, y, w, h, r, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiiii|pp", kwlist, &x, &y, &w, &h, &r, &col, &f, &fill)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_round_rect(&self->canvas, x, y, w, h, r, col, f || fill);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_circle(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "r", "c", "f", "fill", NULL};
    int x, y, r, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiii|pp", kwlist, &x, &y, &r, &col, &f, &fill)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_circle(&self->canvas, x, y, r, col, f || fill);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_line(GfxFrameBufferObject *self, PyObject *args) {
    int x1, y1, x2, y2, col;
    if (!PyArg_ParseTuple(args, "iiiii", &x1, &y1, &x2, &y2, &col)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_line(&self->canvas, x1, y1, x2, y2, col);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_ellipse(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "r1", "r2", "c", "f", "m", "fill", "w", "h", NULL};
    int cx, cy, rx, ry, col, f = 0, m = 0x0f, fill = 0;
    PyObject *w_ignored = NULL, *h_ignored = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiii|pipOO", kwlist,
            &cx, &cy, &rx, &ry, &col, &f, &m, &fill, &w_ignored, &h_ignored)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_ellipse(&self->canvas, cx, cy, rx, ry, col, f || fill, m);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_poly(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "coords", "c", "f", "fill", NULL};
    int x, y, col, f = 0, fill = 0;
    PyObject *coords;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiOi|pp", kwlist, &x, &y, &coords, &col, &f, &fill)) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(coords, &view, PyBUF_FORMAT) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_poly(
        &self->canvas, x, y, view.buf, (size_t)view.len, (size_t)view.itemsize, view.format, col, f || fill);
    PyBuffer_Release(&view);
    return area_from_gfx(&area);
}

static PyObject *framebuffer_arc(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, r, col;
    double a0, a1;
    if (!PyArg_ParseTuple(args, "iiiddi", &x, &y, &r, &a0, &a1, &col)) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_arc(&self->canvas, x, y, r, (float)a0, (float)a1, col);
    return area_from_gfx(&area);
}

static PyObject *framebuffer_triangle(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x0", "y0", "x1", "y1", "x2", "y2", "c", "f", "fill", NULL};
    int x0, y0, x1, y1, x2, y2, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiiiii|pp", kwlist,
            &x0, &y0, &x1, &y1, &x2, &y2, &col, &f, &fill)) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_triangle(&self->canvas, x0, y0, x1, y1, x2, y2, col, f || fill);
    return area_from_gfx(&area);
}

static PyObject *framebuffer_gradient_rect(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "w", "h", "c1", "c2", "vertical", NULL};
    int x, y, w, h, c1, vertical = 1;
    PyObject *c2_obj = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiii|Op", kwlist, &x, &y, &w, &h, &c1, &c2_obj, &vertical)) {
        return NULL;
    }
    int c2 = c1;
    if (c2_obj && c2_obj != Py_None) {
        c2 = (int)PyLong_AsLong(c2_obj);
        if (c2 == -1 && PyErr_Occurred()) {
            return NULL;
        }
    }
    gfx_area_t area = gfx_shapes_gradient_rect(&self->canvas, x, y, w, h, c1, c2, vertical);
    return area_from_gfx(&area);
}

static PyObject *framebuffer_polygon(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"points", "x", "y", "c", "angle", "center_x", "center_y", NULL};
    PyObject *points_seq;
    int x, y, col, cx = 0, cy = 0;
    double angle = 0.0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiii|dii", kwlist,
            &points_seq, &x, &y, &col, &angle, &cx, &cy)) {
        return NULL;
    }
    int points[128];
    size_t len;
    if (parse_points(points_seq, points, &len) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_polygon(&self->canvas, points, len, x, y, col, (float)angle, cx, cy);
    return area_from_gfx(&area);
}

static PyObject *framebuffer_blit(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"source", "x", "y", "key", "palette", NULL};
    PyObject *source;
    int x, y, key = -1;
    PyObject *palette = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oii|iO", kwlist, &source, &x, &y, &key, &palette)) {
        return NULL;
    }
    gfx_fb_t source_fb;
    Py_buffer sview;
    int shave = 0;
    if (get_readonly_framebuffer(source, &source_fb, &sview, &shave) < 0) {
        return NULL;
    }
    const gfx_fb_t *pal = NULL;
    gfx_fb_t palette_fb;
    Py_buffer pview;
    int phave = 0;
    if (palette && palette != Py_None) {
        if (get_readonly_framebuffer(palette, &palette_fb, &pview, &phave) < 0) {
            if (shave) {
                PyBuffer_Release(&sview);
            }
            return NULL;
        }
        pal = &palette_fb;
    }
    gfx_area_t area = gfx_shapes_blit(&self->canvas, &source_fb, x, y, key, pal);
    if (shave) {
        PyBuffer_Release(&sview);
    }
    if (phave) {
        PyBuffer_Release(&pview);
    }
    return area_from_gfx(&area);
}

static PyObject *framebuffer_blit_rect(GfxFrameBufferObject *self, PyObject *args) {
    PyObject *buf;
    int x, y, w, h;
    if (!PyArg_ParseTuple(args, "Oiiii", &buf, &x, &y, &w, &h)) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(buf, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit_rect(&self->canvas, view.buf, x, y, w, h, 2);
    PyBuffer_Release(&view);
    return area_from_gfx(&area);
}

static PyObject *framebuffer_blit_transparent(GfxFrameBufferObject *self, PyObject *args) {
    PyObject *buf;
    int x, y, w, h, key;
    if (!PyArg_ParseTuple(args, "Oiiiii", &buf, &x, &y, &w, &h, &key)) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(buf, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit_transparent(&self->canvas, view.buf, x, y, w, h, key, 2);
    PyBuffer_Release(&view);
    return area_from_gfx(&area);
}

static PyObject *framebuffer_text_common(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds, int fixed_height) {
    static char *kwlist_h[] = {"s", "x", "y", "c", "scale", "inverted", "font_data", "height", NULL};
    static char *kwlist[] = {"s", "x", "y", "c", "scale", "inverted", "font_data", NULL};
    const char *s;
    int x, y, c = 1, scale = 1, inverted = 0, height = 8;
    PyObject *font_data = NULL;
    if (fixed_height < 0) {
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "sii|iipOi", kwlist_h,
                &s, &x, &y, &c, &scale, &inverted, &font_data, &height)) {
            return NULL;
        }
    } else {
        height = fixed_height;
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "sii|iipO", kwlist,
                &s, &x, &y, &c, &scale, &inverted, &font_data)) {
            return NULL;
        }
    }
    return do_text(&self->canvas, s, x, y, c, scale, inverted, font_data, height);
}

static PyObject *framebuffer_text(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    return framebuffer_text_common(self, args, kwds, -1);
}
static PyObject *framebuffer_text8(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    return framebuffer_text_common(self, args, kwds, 8);
}
static PyObject *framebuffer_text14(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    return framebuffer_text_common(self, args, kwds, 14);
}
static PyObject *framebuffer_text16(GfxFrameBufferObject *self, PyObject *args, PyObject *kwds) {
    return framebuffer_text_common(self, args, kwds, 16);
}

static PyObject *framebuffer_scroll(GfxFrameBufferObject *self, PyObject *args) {
    int xstep, ystep;
    if (!PyArg_ParseTuple(args, "ii", &xstep, &ystep)) {
        return NULL;
    }
    gfx_fb_scroll(&self->fb, xstep, ystep);
    gfx_area_t area = {0, 0, self->fb.width, self->fb.height};
    return area_from_gfx(&area);
}

static PyObject *framebuffer_save(GfxFrameBufferObject *self, PyObject *args) {
    const char *path = "screenshot";
    if (!PyArg_ParseTuple(args, "|s", &path)) {
        return NULL;
    }
    char out_path[512];
    if (gfx_files_save_image(&self->fb, path, out_path, sizeof(out_path)) < 0) {
        PyErr_SetString(PyExc_ValueError, "save failed");
        return NULL;
    }
    return PyUnicode_FromString(out_path);
}

static PyObject *framebuffer_from_file(PyObject *cls, PyObject *args) {
    (void)cls;
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }
    gfx_image_fb_t img = {0};
    if (gfx_files_load_image(path, &img) < 0) {
        PyErr_SetString(PyExc_ValueError, "Unsupported image");
        return NULL;
    }
    return framebuffer_from_image(&img);
}

static PyObject *framebuffer_get_width(GfxFrameBufferObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->fb.width);
}
static PyObject *framebuffer_get_height(GfxFrameBufferObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->fb.height);
}
static PyObject *framebuffer_get_buffer(GfxFrameBufferObject *self, void *closure) {
    (void)closure;
    Py_INCREF(self->buf_obj);
    return self->buf_obj;
}
static PyObject *framebuffer_get_format(GfxFrameBufferObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->fb.format);
}
static PyObject *framebuffer_get_color_depth(GfxFrameBufferObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(gfx_fb_color_depth(self->fb.format));
}

static PyGetSetDef framebuffer_getset[] = {
    {"width", (getter)framebuffer_get_width, NULL, NULL},
    {"height", (getter)framebuffer_get_height, NULL, NULL},
    {"buffer", (getter)framebuffer_get_buffer, NULL, NULL},
    {"format", (getter)framebuffer_get_format, NULL, NULL},
    {"color_depth", (getter)framebuffer_get_color_depth, NULL, NULL},
    {NULL},
};

static PyMethodDef framebuffer_methods[] = {
    {"from_file", (PyCFunction)framebuffer_from_file, METH_VARARGS | METH_STATIC, NULL},
    {"fill", (PyCFunction)framebuffer_fill, METH_VARARGS, NULL},
    {"fill_rect", (PyCFunction)framebuffer_fill_rect, METH_VARARGS, NULL},
    {"pixel", (PyCFunction)framebuffer_pixel, METH_VARARGS, NULL},
    {"hline", (PyCFunction)framebuffer_hline, METH_VARARGS, NULL},
    {"vline", (PyCFunction)framebuffer_vline, METH_VARARGS, NULL},
    {"rect", (PyCFunction)framebuffer_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"round_rect", (PyCFunction)framebuffer_round_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"circle", (PyCFunction)framebuffer_circle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"line", (PyCFunction)framebuffer_line, METH_VARARGS, NULL},
    {"ellipse", (PyCFunction)framebuffer_ellipse, METH_VARARGS | METH_KEYWORDS, NULL},
    {"poly", (PyCFunction)framebuffer_poly, METH_VARARGS | METH_KEYWORDS, NULL},
    {"arc", (PyCFunction)framebuffer_arc, METH_VARARGS, NULL},
    {"triangle", (PyCFunction)framebuffer_triangle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"gradient_rect", (PyCFunction)framebuffer_gradient_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"polygon", (PyCFunction)framebuffer_polygon, METH_VARARGS | METH_KEYWORDS, NULL},
    {"blit", (PyCFunction)framebuffer_blit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"blit_rect", (PyCFunction)framebuffer_blit_rect, METH_VARARGS, NULL},
    {"blit_transparent", (PyCFunction)framebuffer_blit_transparent, METH_VARARGS, NULL},
    {"text", (PyCFunction)framebuffer_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text8", (PyCFunction)framebuffer_text8, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text14", (PyCFunction)framebuffer_text14, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text16", (PyCFunction)framebuffer_text16, METH_VARARGS | METH_KEYWORDS, NULL},
    {"scroll", (PyCFunction)framebuffer_scroll, METH_VARARGS, NULL},
    {"save", (PyCFunction)framebuffer_save, METH_VARARGS, NULL},
    {NULL},
};

static PyTypeObject GfxFrameBufferType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "graphics.FrameBuffer",
    .tp_basicsize = sizeof(GfxFrameBufferObject),
    .tp_dealloc = (destructor)framebuffer_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = framebuffer_methods,
    .tp_getset = framebuffer_getset,
    .tp_init = (initproc)framebuffer_init,
    .tp_new = framebuffer_new,
};

/* ------------------------------------------------------------------------- */
/* Module-level capabilities                                                 */
/* ------------------------------------------------------------------------- */

static PyObject *mod_framebuf_backend(PyObject *self, PyObject *args) {
    (void)self;
    (void)args;
    return PyUnicode_FromString(gfx_framebuf_backend());
}

static PyObject *mod_implementation(PyObject *self, PyObject *args) {
    (void)self;
    (void)args;
    return PyUnicode_FromString(gfx_implementation());
}

static PyObject *mod_capabilities(PyObject *self, PyObject *args) {
    (void)self;
    (void)args;
    PyObject *caps = PyDict_New();
    if (!caps) {
        return NULL;
    }
    PyObject *formats = PyList_New(8);
    if (!formats) {
        Py_DECREF(caps);
        return NULL;
    }
    const char *names[] = {"MONO_VLSB", "MONO_HLSB", "MONO_HMSB", "RGB565", "GS2_HMSB", "GS4_HMSB", "GS8", "RGB888"};
    for (int i = 0; i < 8; i++) {
        PyList_SET_ITEM(formats, i, PyUnicode_FromString(names[i]));
    }
    PyDict_SetItemString(caps, "implementation", PyUnicode_FromString(gfx_implementation()));
    PyDict_SetItemString(caps, "framebuf", PyUnicode_FromString(gfx_framebuf_backend()));
    PyDict_SetItemString(caps, "formats", formats);
    Py_DECREF(formats);
    return caps;
}

/* ------------------------------------------------------------------------- */
/* Module-level drawing (duck-typed canvas)                                  */
/* ------------------------------------------------------------------------- */

static PyObject *mod_fill(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int col;
    if (!PyArg_ParseTuple(args, "Oi", &target, &col)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_fill(&slot.canvas, col);
    RETURN_AREA(area);
}

static PyObject *mod_fill_rect(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, w, h, c;
    if (!PyArg_ParseTuple(args, "Oiiiii", &target, &x, &y, &w, &h, &c)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_fill_rect(&slot.canvas, x, y, w, h, c);
    RETURN_AREA(area);
}

static PyObject *mod_pixel(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, c;
    if (!PyArg_ParseTuple(args, "Oiii", &target, &x, &y, &c)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_pixel(&slot.canvas, x, y, c);
    RETURN_AREA(area);
}

static PyObject *mod_hline(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, w, c;
    if (!PyArg_ParseTuple(args, "Oiiii", &target, &x, &y, &w, &c)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_hline(&slot.canvas, x, y, w, c);
    RETURN_AREA(area);
}

static PyObject *mod_vline(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, h, c;
    if (!PyArg_ParseTuple(args, "Oiiii", &target, &x, &y, &h, &c)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_vline(&slot.canvas, x, y, h, c);
    RETURN_AREA(area);
}

static PyObject *mod_line(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x1, y1, x2, y2, col;
    if (!PyArg_ParseTuple(args, "Oiiiii", &target, &x1, &y1, &x2, &y2, &col)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_line(&slot.canvas, x1, y1, x2, y2, col);
    RETURN_AREA(area);
}

static PyObject *mod_rect(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "x", "y", "w", "h", "c", "f", "fill", NULL};
    PyObject *target;
    int x, y, w, h, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiiiii|pp", kwlist, &target, &x, &y, &w, &h, &col, &f, &fill)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_rect(&slot.canvas, x, y, w, h, col, f || fill);
    RETURN_AREA(area);
}

static PyObject *mod_round_rect(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "x", "y", "w", "h", "r", "c", "f", "fill", NULL};
    PyObject *target;
    int x, y, w, h, r, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiiiiii|pp", kwlist, &target, &x, &y, &w, &h, &r, &col, &f, &fill)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_round_rect(&slot.canvas, x, y, w, h, r, col, f || fill);
    RETURN_AREA(area);
}

static PyObject *mod_circle(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "x", "y", "r", "c", "f", "fill", NULL};
    PyObject *target;
    int x, y, r, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiiii|pp", kwlist, &target, &x, &y, &r, &col, &f, &fill)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_circle(&slot.canvas, x, y, r, col, f || fill);
    RETURN_AREA(area);
}

static PyObject *mod_ellipse(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "x", "y", "r1", "r2", "c", "f", "m", "fill", "w", "h", NULL};
    PyObject *target;
    int cx, cy, rx, ry, col, f = 0, m = 0x0f, fill = 0;
    PyObject *w_ignored = NULL, *h_ignored = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiiiii|pipOO", kwlist,
            &target, &cx, &cy, &rx, &ry, &col, &f, &m, &fill, &w_ignored, &h_ignored)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_ellipse(&slot.canvas, cx, cy, rx, ry, col, f || fill, m);
    RETURN_AREA(area);
}

static PyObject *mod_arc(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, r, col;
    double a0, a1;
    if (!PyArg_ParseTuple(args, "Oiiiddi", &target, &x, &y, &r, &a0, &a1, &col)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_arc(&slot.canvas, x, y, r, (float)a0, (float)a1, col);
    RETURN_AREA(area);
}

static PyObject *mod_triangle(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "x0", "y0", "x1", "y1", "x2", "y2", "c", "f", "fill", NULL};
    PyObject *target;
    int x0, y0, x1, y1, x2, y2, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiiiiiii|pp", kwlist,
            &target, &x0, &y0, &x1, &y1, &x2, &y2, &col, &f, &fill)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_triangle(&slot.canvas, x0, y0, x1, y1, x2, y2, col, f || fill);
    RETURN_AREA(area);
}

static PyObject *mod_gradient_rect(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "x", "y", "w", "h", "c1", "c2", "vertical", NULL};
    PyObject *target;
    int x, y, w, h, c1, vertical = 1;
    PyObject *c2_obj = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiiiii|Op", kwlist,
            &target, &x, &y, &w, &h, &c1, &c2_obj, &vertical)) {
        return NULL;
    }
    int c2 = c1;
    if (c2_obj && c2_obj != Py_None) {
        c2 = (int)PyLong_AsLong(c2_obj);
        if (c2 == -1 && PyErr_Occurred()) {
            return NULL;
        }
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_gradient_rect(&slot.canvas, x, y, w, h, c1, c2, vertical);
    RETURN_AREA(area);
}

static PyObject *mod_poly(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "x", "y", "coords", "c", "f", "fill", NULL};
    PyObject *target, *coords;
    int x, y, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OiiOi|pp", kwlist, &target, &x, &y, &coords, &col, &f, &fill)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(coords, &view, PyBUF_FORMAT) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_poly(
        &slot.canvas, x, y, view.buf, (size_t)view.len, (size_t)view.itemsize, view.format, col, f || fill);
    PyBuffer_Release(&view);
    RETURN_AREA(area);
}

static PyObject *mod_polygon(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "points", "x", "y", "c", "angle", "center_x", "center_y", NULL};
    PyObject *target, *points_seq;
    int x, y, col, cx = 0, cy = 0;
    double angle = 0.0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOiii|dii", kwlist,
            &target, &points_seq, &x, &y, &col, &angle, &cx, &cy)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    int points[128];
    size_t len;
    if (parse_points(points_seq, points, &len) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_polygon(&slot.canvas, points, len, x, y, col, (float)angle, cx, cy);
    RETURN_AREA(area);
}

static PyObject *mod_blit(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    static char *kwlist[] = {"canvas", "source", "x", "y", "key", "palette", NULL};
    PyObject *target, *source;
    int x, y, key = -1;
    PyObject *palette = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOii|iO", kwlist, &target, &source, &x, &y, &key, &palette)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    gfx_fb_t source_fb;
    Py_buffer sview;
    int shave = 0;
    if (get_readonly_framebuffer(source, &source_fb, &sview, &shave) < 0) {
        return NULL;
    }
    const gfx_fb_t *pal = NULL;
    gfx_fb_t palette_fb;
    Py_buffer pview;
    int phave = 0;
    if (palette && palette != Py_None) {
        if (get_readonly_framebuffer(palette, &palette_fb, &pview, &phave) < 0) {
            if (shave) {
                PyBuffer_Release(&sview);
            }
            return NULL;
        }
        pal = &palette_fb;
    }
    gfx_area_t area = gfx_shapes_blit(&slot.canvas, &source_fb, x, y, key, pal);
    if (shave) {
        PyBuffer_Release(&sview);
    }
    if (phave) {
        PyBuffer_Release(&pview);
    }
    RETURN_AREA(area);
}

static PyObject *mod_blit_rect(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target, *buf;
    int x, y, w, h;
    if (!PyArg_ParseTuple(args, "OOiiii", &target, &buf, &x, &y, &w, &h)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(buf, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit_rect(&slot.canvas, view.buf, x, y, w, h, 2);
    PyBuffer_Release(&view);
    RETURN_AREA(area);
}

static PyObject *mod_blit_transparent(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target, *buf;
    int x, y, w, h, key;
    if (!PyArg_ParseTuple(args, "OOiiiii", &target, &buf, &x, &y, &w, &h, &key)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(buf, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit_transparent(&slot.canvas, view.buf, x, y, w, h, key, 2);
    PyBuffer_Release(&view);
    RETURN_AREA(area);
}

static PyObject *mod_text_common(PyObject *args, PyObject *kwds, int fixed_height) {
    static char *kwlist_h[] = {"canvas", "s", "x", "y", "c", "scale", "inverted", "font_data", "height", NULL};
    static char *kwlist[] = {"canvas", "s", "x", "y", "c", "scale", "inverted", "font_data", NULL};
    PyObject *target;
    const char *s;
    int x, y, c = 1, scale = 1, inverted = 0, height = 8;
    PyObject *font_data = NULL;
    if (fixed_height < 0) {
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "Osii|iipOi", kwlist_h,
                &target, &s, &x, &y, &c, &scale, &inverted, &font_data, &height)) {
            return NULL;
        }
    } else {
        height = fixed_height;
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "Osii|iipO", kwlist,
                &target, &s, &x, &y, &c, &scale, &inverted, &font_data)) {
            return NULL;
        }
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(target, &slot) < 0) {
        return NULL;
    }
    return do_text(&slot.canvas, s, x, y, c, scale, inverted, font_data, height);
}

static PyObject *mod_text(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    return mod_text_common(args, kwds, -1);
}
static PyObject *mod_text8(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    return mod_text_common(args, kwds, 8);
}
static PyObject *mod_text14(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    return mod_text_common(args, kwds, 14);
}
static PyObject *mod_text16(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    return mod_text_common(args, kwds, 16);
}

static PyObject *mod_load_image(PyObject *self, PyObject *args) {
    (void)self;
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }
    gfx_image_fb_t img = {0};
    if (gfx_files_load_image(path, &img) < 0) {
        PyErr_SetString(PyExc_ValueError, "Unsupported image");
        return NULL;
    }
    return framebuffer_from_image(&img);
}

static PyObject *mod_bmp_to_framebuffer(PyObject *self, PyObject *args) {
    (void)self;
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }
    gfx_image_fb_t img = {0};
    if (gfx_files_bmp_to_framebuffer(path, &img) < 0) {
        PyErr_SetString(PyExc_ValueError, "BMP load failed");
        return NULL;
    }
    return framebuffer_from_image(&img);
}

static PyObject *mod_pbm_to_framebuffer(PyObject *self, PyObject *args) {
    (void)self;
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }
    gfx_image_fb_t img = {0};
    if (gfx_files_pbm_to_framebuffer(path, &img) < 0) {
        PyErr_SetString(PyExc_ValueError, "PBM load failed");
        return NULL;
    }
    return framebuffer_from_image(&img);
}

static PyObject *mod_pgm_to_framebuffer(PyObject *self, PyObject *args) {
    (void)self;
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }
    gfx_image_fb_t img = {0};
    if (gfx_files_pgm_to_framebuffer(path, &img) < 0) {
        PyErr_SetString(PyExc_ValueError, "PGM load failed");
        return NULL;
    }
    return framebuffer_from_image(&img);
}

static PyObject *mod_save_image(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    const char *path = "screenshot";
    if (!PyArg_ParseTuple(args, "O|s", &target, &path)) {
        return NULL;
    }
    if (!PyObject_TypeCheck(target, &GfxFrameBufferType)) {
        PyErr_SetString(PyExc_TypeError, "FrameBuffer required");
        return NULL;
    }
    GfxFrameBufferObject *fb = (GfxFrameBufferObject *)target;
    char out_path[512];
    if (gfx_files_save_image(&fb->fb, path, out_path, sizeof(out_path)) < 0) {
        PyErr_SetString(PyExc_ValueError, "save_image failed");
        return NULL;
    }
    return PyUnicode_FromString(out_path);
}

/* ------------------------------------------------------------------------- */
/* Draw                                                                      */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    PyObject *canvas_obj;
    gfx_draw_t draw;
    cpy_canvas_slot_t slot;
} GfxDrawObject;

typedef struct {
    PyObject_HEAD
    PyObject *draw_obj;
} GfxClipCtxObject;

static PyTypeObject GfxClipCtxType;

static PyObject *draw_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    (void)kwds;
    PyObject *canvas;
    if (!PyArg_ParseTuple(args, "O", &canvas)) {
        return NULL;
    }
    GfxDrawObject *o = (GfxDrawObject *)type->tp_alloc(type, 0);
    if (!o) {
        return NULL;
    }
    o->canvas_obj = canvas;
    Py_INCREF(canvas);
    if (cpy_canvas_resolve(canvas, &o->slot) < 0) {
        Py_DECREF(o);
        return NULL;
    }
    gfx_draw_init(&o->draw, &o->slot.canvas);
    return (PyObject *)o;
}

static void draw_dealloc(GfxDrawObject *self) {
    Py_XDECREF(self->canvas_obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

/* Re-resolve the canvas each call (matches MP draw_target), rebinding the py
 * ctx pointer into the stable embedded slot. Returns NULL on failure. */
static const gfx_canvas_t *draw_target(GfxDrawObject *self) {
    if (cpy_canvas_resolve(self->canvas_obj, &self->slot) < 0) {
        return NULL;
    }
    self->draw.canvas = self->slot.canvas;
    return gfx_draw_target(&self->draw);
}

static PyObject *draw_fill(GfxDrawObject *self, PyObject *args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_fill(t, col);
    RETURN_AREA(area);
}

static PyObject *draw_fill_rect(GfxDrawObject *self, PyObject *args) {
    int x, y, w, h, c;
    if (!PyArg_ParseTuple(args, "iiiii", &x, &y, &w, &h, &c)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_fill_rect(t, x, y, w, h, c);
    RETURN_AREA(area);
}

static PyObject *draw_pixel(GfxDrawObject *self, PyObject *args) {
    int x, y, c;
    if (!PyArg_ParseTuple(args, "iii", &x, &y, &c)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_pixel(t, x, y, c);
    RETURN_AREA(area);
}

static PyObject *draw_hline(GfxDrawObject *self, PyObject *args) {
    int x, y, w, c;
    if (!PyArg_ParseTuple(args, "iiii", &x, &y, &w, &c)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_hline(t, x, y, w, c);
    RETURN_AREA(area);
}

static PyObject *draw_vline(GfxDrawObject *self, PyObject *args) {
    int x, y, h, c;
    if (!PyArg_ParseTuple(args, "iiii", &x, &y, &h, &c)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_vline(t, x, y, h, c);
    RETURN_AREA(area);
}

static PyObject *draw_line(GfxDrawObject *self, PyObject *args) {
    int x1, y1, x2, y2, col;
    if (!PyArg_ParseTuple(args, "iiiii", &x1, &y1, &x2, &y2, &col)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_line(t, x1, y1, x2, y2, col);
    RETURN_AREA(area);
}

static PyObject *draw_rect(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "w", "h", "c", "f", "fill", NULL};
    int x, y, w, h, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiii|pp", kwlist, &x, &y, &w, &h, &col, &f, &fill)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_rect(t, x, y, w, h, col, f || fill);
    RETURN_AREA(area);
}

static PyObject *draw_round_rect(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "w", "h", "r", "c", "f", "fill", NULL};
    int x, y, w, h, r, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiiii|pp", kwlist, &x, &y, &w, &h, &r, &col, &f, &fill)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_round_rect(t, x, y, w, h, r, col, f || fill);
    RETURN_AREA(area);
}

static PyObject *draw_circle(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "r", "c", "f", "fill", NULL};
    int x, y, r, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiii|pp", kwlist, &x, &y, &r, &col, &f, &fill)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_circle(t, x, y, r, col, f || fill);
    RETURN_AREA(area);
}

static PyObject *draw_ellipse(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "r1", "r2", "c", "f", "m", "fill", "w", "h", NULL};
    int cx, cy, rx, ry, col, f = 0, m = 0x0f, fill = 0;
    PyObject *w_ignored = NULL, *h_ignored = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiii|pipOO", kwlist,
            &cx, &cy, &rx, &ry, &col, &f, &m, &fill, &w_ignored, &h_ignored)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_ellipse(t, cx, cy, rx, ry, col, f || fill, m);
    RETURN_AREA(area);
}

static PyObject *draw_arc(GfxDrawObject *self, PyObject *args) {
    int x, y, r, col;
    double a0, a1;
    if (!PyArg_ParseTuple(args, "iiiddi", &x, &y, &r, &a0, &a1, &col)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_arc(t, x, y, r, (float)a0, (float)a1, col);
    RETURN_AREA(area);
}

static PyObject *draw_triangle(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x0", "y0", "x1", "y1", "x2", "y2", "c", "f", "fill", NULL};
    int x0, y0, x1, y1, x2, y2, col, f = 0, fill = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiiiii|pp", kwlist,
            &x0, &y0, &x1, &y1, &x2, &y2, &col, &f, &fill)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_triangle(t, x0, y0, x1, y1, x2, y2, col, f || fill);
    RETURN_AREA(area);
}

static PyObject *draw_gradient_rect(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "w", "h", "c1", "c2", "vertical", NULL};
    int x, y, w, h, c1, vertical = 1;
    PyObject *c2_obj = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiiii|Op", kwlist, &x, &y, &w, &h, &c1, &c2_obj, &vertical)) {
        return NULL;
    }
    int c2 = c1;
    if (c2_obj && c2_obj != Py_None) {
        c2 = (int)PyLong_AsLong(c2_obj);
        if (c2 == -1 && PyErr_Occurred()) {
            return NULL;
        }
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_gradient_rect(t, x, y, w, h, c1, c2, vertical);
    RETURN_AREA(area);
}

static PyObject *draw_poly(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "coords", "c", "f", "fill", NULL};
    int x, y, col, f = 0, fill = 0;
    PyObject *coords;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iiOi|pp", kwlist, &x, &y, &coords, &col, &f, &fill)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(coords, &view, PyBUF_FORMAT) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_poly(t, x, y, view.buf, (size_t)view.len, (size_t)view.itemsize, view.format, col, f || fill);
    PyBuffer_Release(&view);
    RETURN_AREA(area);
}

static PyObject *draw_polygon(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"points", "x", "y", "c", "angle", "center_x", "center_y", NULL};
    PyObject *points_seq;
    int x, y, col, cx = 0, cy = 0;
    double angle = 0.0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiii|dii", kwlist,
            &points_seq, &x, &y, &col, &angle, &cx, &cy)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    int points[128];
    size_t len;
    if (parse_points(points_seq, points, &len) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_polygon(t, points, len, x, y, col, (float)angle, cx, cy);
    RETURN_AREA(area);
}

static PyObject *draw_blit(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"source", "x", "y", "key", "palette", NULL};
    PyObject *source;
    int x, y, key = -1;
    PyObject *palette = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oii|iO", kwlist, &source, &x, &y, &key, &palette)) {
        return NULL;
    }
    gfx_fb_t source_fb;
    Py_buffer sview;
    int shave = 0;
    if (get_readonly_framebuffer(source, &source_fb, &sview, &shave) < 0) {
        return NULL;
    }
    const gfx_fb_t *pal = NULL;
    gfx_fb_t palette_fb;
    Py_buffer pview;
    int phave = 0;
    if (palette && palette != Py_None) {
        if (get_readonly_framebuffer(palette, &palette_fb, &pview, &phave) < 0) {
            if (shave) {
                PyBuffer_Release(&sview);
            }
            return NULL;
        }
        pal = &palette_fb;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        if (shave) {
            PyBuffer_Release(&sview);
        }
        if (phave) {
            PyBuffer_Release(&pview);
        }
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit(t, &source_fb, x, y, key, pal);
    if (shave) {
        PyBuffer_Release(&sview);
    }
    if (phave) {
        PyBuffer_Release(&pview);
    }
    RETURN_AREA(area);
}

static PyObject *draw_blit_rect(GfxDrawObject *self, PyObject *args) {
    PyObject *buf;
    int x, y, w, h;
    if (!PyArg_ParseTuple(args, "Oiiii", &buf, &x, &y, &w, &h)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(buf, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit_rect(t, view.buf, x, y, w, h, 2);
    PyBuffer_Release(&view);
    RETURN_AREA(area);
}

static PyObject *draw_blit_transparent(GfxDrawObject *self, PyObject *args) {
    PyObject *buf;
    int x, y, w, h, key;
    if (!PyArg_ParseTuple(args, "Oiiiii", &buf, &x, &y, &w, &h, &key)) {
        return NULL;
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(buf, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit_transparent(t, view.buf, x, y, w, h, key, 2);
    PyBuffer_Release(&view);
    RETURN_AREA(area);
}

static PyObject *draw_text_common(GfxDrawObject *self, PyObject *args, PyObject *kwds, int fixed_height) {
    static char *kwlist_h[] = {"s", "x", "y", "c", "scale", "inverted", "font_data", "height", NULL};
    static char *kwlist[] = {"s", "x", "y", "c", "scale", "inverted", "font_data", NULL};
    const char *s;
    int x, y, c = 1, scale = 1, inverted = 0, height = 8;
    PyObject *font_data = NULL;
    if (fixed_height < 0) {
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "sii|iipOi", kwlist_h,
                &s, &x, &y, &c, &scale, &inverted, &font_data, &height)) {
            return NULL;
        }
    } else {
        height = fixed_height;
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "sii|iipO", kwlist,
                &s, &x, &y, &c, &scale, &inverted, &font_data)) {
            return NULL;
        }
    }
    const gfx_canvas_t *t = draw_target(self);
    if (!t) {
        return NULL;
    }
    return do_text(t, s, x, y, c, scale, inverted, font_data, height);
}

static PyObject *draw_text(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    return draw_text_common(self, args, kwds, -1);
}
static PyObject *draw_text8(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    return draw_text_common(self, args, kwds, 8);
}
static PyObject *draw_text14(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    return draw_text_common(self, args, kwds, 14);
}
static PyObject *draw_text16(GfxDrawObject *self, PyObject *args, PyObject *kwds) {
    return draw_text_common(self, args, kwds, 16);
}

static PyObject *draw_clip(GfxDrawObject *self, PyObject *args) {
    gfx_area_t area;
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    if (n == 1 && PyObject_TypeCheck(PyTuple_GET_ITEM(args, 0), &GfxAreaType)) {
        area = ((GfxAreaObject *)PyTuple_GET_ITEM(args, 0))->area;
    } else if (n == 4) {
        int x, y, w, h;
        if (!PyArg_ParseTuple(args, "iiii", &x, &y, &w, &h)) {
            return NULL;
        }
        gfx_area_init(&area, x, y, w, h);
    } else {
        PyErr_SetString(PyExc_ValueError, "clip() requires x, y, w, h or an Area");
        return NULL;
    }
    gfx_draw_push_clip(&self->draw, &area);
    GfxClipCtxObject *ctx = PyObject_New(GfxClipCtxObject, &GfxClipCtxType);
    if (!ctx) {
        gfx_draw_pop_clip(&self->draw);
        return NULL;
    }
    ctx->draw_obj = (PyObject *)self;
    Py_INCREF(self);
    return (PyObject *)ctx;
}

static PyMethodDef draw_methods[] = {
    {"fill", (PyCFunction)draw_fill, METH_VARARGS, NULL},
    {"fill_rect", (PyCFunction)draw_fill_rect, METH_VARARGS, NULL},
    {"pixel", (PyCFunction)draw_pixel, METH_VARARGS, NULL},
    {"hline", (PyCFunction)draw_hline, METH_VARARGS, NULL},
    {"vline", (PyCFunction)draw_vline, METH_VARARGS, NULL},
    {"line", (PyCFunction)draw_line, METH_VARARGS, NULL},
    {"rect", (PyCFunction)draw_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"round_rect", (PyCFunction)draw_round_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"circle", (PyCFunction)draw_circle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"ellipse", (PyCFunction)draw_ellipse, METH_VARARGS | METH_KEYWORDS, NULL},
    {"arc", (PyCFunction)draw_arc, METH_VARARGS, NULL},
    {"triangle", (PyCFunction)draw_triangle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"gradient_rect", (PyCFunction)draw_gradient_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"poly", (PyCFunction)draw_poly, METH_VARARGS | METH_KEYWORDS, NULL},
    {"polygon", (PyCFunction)draw_polygon, METH_VARARGS | METH_KEYWORDS, NULL},
    {"blit", (PyCFunction)draw_blit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"blit_rect", (PyCFunction)draw_blit_rect, METH_VARARGS, NULL},
    {"blit_transparent", (PyCFunction)draw_blit_transparent, METH_VARARGS, NULL},
    {"text", (PyCFunction)draw_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text8", (PyCFunction)draw_text8, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text14", (PyCFunction)draw_text14, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text16", (PyCFunction)draw_text16, METH_VARARGS | METH_KEYWORDS, NULL},
    {"clip", (PyCFunction)draw_clip, METH_VARARGS, NULL},
    {NULL},
};

static PyTypeObject GfxDrawType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "graphics.Draw",
    .tp_basicsize = sizeof(GfxDrawObject),
    .tp_dealloc = (destructor)draw_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = draw_methods,
    .tp_new = draw_new,
};

/* ClipContext returned by Draw.clip() */
static PyObject *clipctx_enter(GfxClipCtxObject *self, PyObject *noargs) {
    (void)noargs;
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *clipctx_exit(GfxClipCtxObject *self, PyObject *args) {
    (void)args;
    GfxDrawObject *d = (GfxDrawObject *)self->draw_obj;
    gfx_draw_pop_clip(&d->draw);
    Py_RETURN_FALSE;
}

static void clipctx_dealloc(GfxClipCtxObject *self) {
    Py_XDECREF(self->draw_obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef clipctx_methods[] = {
    {"__enter__", (PyCFunction)clipctx_enter, METH_NOARGS, NULL},
    {"__exit__", (PyCFunction)clipctx_exit, METH_VARARGS, NULL},
    {NULL},
};

static PyTypeObject GfxClipCtxType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "graphics.ClipContext",
    .tp_basicsize = sizeof(GfxClipCtxObject),
    .tp_dealloc = (destructor)clipctx_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = clipctx_methods,
};

/* ------------------------------------------------------------------------- */
/* Font                                                                      */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    gfx_font_t font;
    PyObject *data_obj;
    PyObject *path_obj;
    Py_buffer data_view;
    int has_data_view;
} GfxFontObject;

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

static PyObject *font_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"font_data", "height", "cached", NULL};
    PyObject *font_data = Py_None;
    int height = 0;
    int cached = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oip", kwlist, &font_data, &height, &cached)) {
        return NULL;
    }
    (void)cached;
    GfxFontObject *o = (GfxFontObject *)type->tp_alloc(type, 0);
    if (!o) {
        return NULL;
    }
    o->data_obj = NULL;
    o->path_obj = NULL;
    o->has_data_view = 0;

    if (font_data == Py_None) {
        if (height == 0) {
            height = 8;
        }
        gfx_font_init_default(&o->font, height);
    } else if (PyUnicode_Check(font_data)) {
        const char *path = PyUnicode_AsUTF8(font_data);
        if (!path) {
            Py_DECREF(o);
            return NULL;
        }
        if (height == 0 && parse_font_height_from_name(path, &height) < 0) {
            Py_DECREF(o);
            PyErr_SetString(PyExc_ValueError, "Invalid font");
            return NULL;
        }
        if (height == 0) {
            height = 8;
        }
        FILE *f = fopen(path, "rb");
        if (!f) {
            Py_DECREF(o);
            PyErr_Format(PyExc_FileNotFoundError, "Could not find font file: %s", path);
            return NULL;
        }
        fseek(f, 0, SEEK_END);
        long flen = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8_t *data = (uint8_t *)malloc((size_t)flen);
        if (!data || fread(data, 1, (size_t)flen, f) != (size_t)flen) {
            fclose(f);
            free(data);
            Py_DECREF(o);
            PyErr_SetString(PyExc_OSError, "Font read failed");
            return NULL;
        }
        fclose(f);
        o->font.data = data;
        o->font.data_len = (size_t)flen;
        o->font.height = height;
        o->font.width = 8;
        o->font.owns_data = 1;
        o->path_obj = font_data;
        Py_INCREF(font_data);
    } else {
        if (PyObject_GetBuffer(font_data, &o->data_view, PyBUF_SIMPLE) < 0) {
            Py_DECREF(o);
            return NULL;
        }
        o->has_data_view = 1;
        o->data_obj = font_data;
        Py_INCREF(font_data);
        if (height == 0) {
            /* Match Python: derive height from a full 256-glyph memoryview. */
            height = o->data_view.len ? (int)(o->data_view.len / 256) : 8;
            if (height == 0) {
                height = 8;
            }
        }
        gfx_font_init_from_data(&o->font, o->data_view.buf, (size_t)o->data_view.len, height);
    }
    return (PyObject *)o;
}

static void font_dealloc(GfxFontObject *self) {
    gfx_font_deinit(&self->font);
    if (self->has_data_view) {
        PyBuffer_Release(&self->data_view);
    }
    Py_XDECREF(self->data_obj);
    Py_XDECREF(self->path_obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *font_text(GfxFontObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"canvas", "s", "x", "y", "color", "scale", "inverted", NULL};
    PyObject *canvas;
    const char *s;
    int x, y, col, scale = 1, inverted = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Osiii|ip", kwlist, &canvas, &s, &x, &y, &col, &scale, &inverted)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(canvas, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_font_text(&slot.canvas, &self->font, s, x, y, col, scale, inverted);
    RETURN_AREA(area);
}

static PyObject *font_draw_char(GfxFontObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"char", "x", "y", "canvas", "color", "scale", "inverted", NULL};
    const char *ch;
    int x, y, col, scale = 1, inverted = 0;
    PyObject *canvas;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "siiOi|ip", kwlist, &ch, &x, &y, &canvas, &col, &scale, &inverted)) {
        return NULL;
    }
    cpy_canvas_slot_t slot;
    if (cpy_canvas_resolve(canvas, &slot) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_font_draw_char(&slot.canvas, &self->font, (unsigned char)ch[0], x, y, col, scale, inverted);
    RETURN_AREA(area);
}

static PyObject *font_text_width(GfxFontObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"text", "scale", NULL};
    const char *text;
    int scale = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|i", kwlist, &text, &scale)) {
        return NULL;
    }
    return PyLong_FromLong(gfx_font_text_width(&self->font, text, scale));
}

static PyObject *font_deinit(GfxFontObject *self, PyObject *noargs) {
    (void)noargs;
    gfx_font_deinit(&self->font);
    if (self->has_data_view) {
        PyBuffer_Release(&self->data_view);
        self->has_data_view = 0;
    }
    Py_CLEAR(self->data_obj);
    Py_CLEAR(self->path_obj);
    Py_RETURN_NONE;
}

static PyObject *font_export(GfxFontObject *self, PyObject *args) {
    const char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }
    if (gfx_font_export(&self->font, filename) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "Font data not cached, cannot export");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *font_get_width(GfxFontObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->font.width);
}
static PyObject *font_get_height(GfxFontObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->font.height);
}
static PyObject *font_get_font_name(GfxFontObject *self, void *closure) {
    (void)closure;
    if (self->path_obj) {
        Py_INCREF(self->path_obj);
        return self->path_obj;
    }
    if (self->data_obj) {
        return PyUnicode_FromString("memoryview");
    }
    return PyUnicode_FromString("default");
}

static PyGetSetDef font_getset[] = {
    {"width", (getter)font_get_width, NULL, NULL},
    {"height", (getter)font_get_height, NULL, NULL},
    {"font_name", (getter)font_get_font_name, NULL, NULL},
    {NULL},
};

static PyMethodDef font_methods[] = {
    {"text", (PyCFunction)font_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"draw_char", (PyCFunction)font_draw_char, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text_width", (PyCFunction)font_text_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"deinit", (PyCFunction)font_deinit, METH_NOARGS, NULL},
    {"export", (PyCFunction)font_export, METH_VARARGS, NULL},
    {NULL},
};

static PyTypeObject GfxFontType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "graphics.Font",
    .tp_basicsize = sizeof(GfxFontObject),
    .tp_dealloc = (destructor)font_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = font_methods,
    .tp_getset = font_getset,
    .tp_new = font_new,
};

/* ------------------------------------------------------------------------- */
/* BMP565                                                                    */
/* ------------------------------------------------------------------------- */

static PyObject *bmp565_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    /* Mirror Python BMP565.__init__(filename=None, source=None, streamed=False,
     * mirrored=False, width=None, height=None). A non-str first positional is
     * also accepted as a buffer source (MicroPython binding compatibility). */
    static char *kwlist[] = {"filename", "source", "streamed", "mirrored", "width", "height", NULL};
    PyObject *filename_arg = Py_None;
    PyObject *source_arg = Py_None;
    int width = 0, height = 0, streamed = 0, mirrored = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOppii", kwlist,
            &filename_arg, &source_arg, &streamed, &mirrored, &width, &height)) {
        return NULL;
    }
    GfxBmp565Object *o = (GfxBmp565Object *)type->tp_alloc(type, 0);
    if (!o) {
        return NULL;
    }
    o->buf_obj = NULL;
    o->filename_obj = NULL;
    o->has_src_view = 0;
    memset(&o->bmp, 0, sizeof(o->bmp));

    PyObject *filename = NULL;
    PyObject *source = NULL;
    if (source_arg != Py_None) {
        source = source_arg;
    }
    if (filename_arg != Py_None) {
        if (PyUnicode_Check(filename_arg)) {
            filename = filename_arg;
        } else if (source == NULL) {
            source = filename_arg;
        }
    }

    if (source != NULL) {
        if (PyObject_GetBuffer(source, &o->src_view, PyBUF_SIMPLE) < 0) {
            Py_DECREF(o);
            return NULL;
        }
        o->has_src_view = 1;
        o->buf_obj = source;
        Py_INCREF(source);
        gfx_bmp565_init_from_buffer(&o->bmp, o->src_view.buf, (size_t)o->src_view.len, width, height);
    } else if (filename != NULL) {
        const char *path = PyUnicode_AsUTF8(filename);
        if (!path) {
            Py_DECREF(o);
            return NULL;
        }
        o->filename_obj = filename;
        Py_INCREF(filename);
        if (streamed) {
            if (gfx_bmp565_open_stream(path, &o->bmp) < 0) {
                Py_DECREF(o);
                PyErr_SetString(PyExc_ValueError, "BMP load failed");
                return NULL;
            }
            o->bmp.mirrored = mirrored;
        } else {
            if (gfx_bmp565_load_from_file(path, &o->bmp) < 0) {
                Py_DECREF(o);
                PyErr_SetString(PyExc_ValueError, "BMP load failed");
                return NULL;
            }
            o->buf_obj = PyByteArray_FromStringAndSize((const char *)o->bmp.buffer, (Py_ssize_t)o->bmp.buffer_len);
            if (!o->buf_obj) {
                Py_DECREF(o);
                return NULL;
            }
        }
    } else {
        Py_DECREF(o);
        PyErr_SetString(PyExc_ValueError, "Invalid arguments");
        return NULL;
    }
    return (PyObject *)o;
}

static void bmp565_dealloc(GfxBmp565Object *self) {
    gfx_bmp565_deinit(&self->bmp);
    if (self->has_src_view) {
        PyBuffer_Release(&self->src_view);
    }
    Py_XDECREF(self->buf_obj);
    Py_XDECREF(self->filename_obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *bmp565_region(GfxBmp565Object *self, long x0, long x1, long y0, long y1) {
    if (x1 < x0) {
        x1 = x0;
    }
    if (y1 < y0) {
        y1 = y0;
    }
    size_t cap = (size_t)(x1 - x0) * (size_t)(y1 - y0) * GFX_BMP565_BYTES_PER_PIXEL;
    PyObject *ba = PyByteArray_FromStringAndSize(NULL, (Py_ssize_t)cap);
    if (!ba) {
        return NULL;
    }
    size_t out_len = 0;
    if (cap > 0 && gfx_bmp565_read_region(&self->bmp, (int)x0, (int)x1, (int)y0, (int)y1,
            (uint8_t *)PyByteArray_AS_STRING(ba), cap, &out_len) < 0) {
        Py_DECREF(ba);
        PyErr_SetString(PyExc_ValueError, "BMP read failed");
        return NULL;
    }
    if ((size_t)PyByteArray_GET_SIZE(ba) != out_len) {
        if (PyByteArray_Resize(ba, (Py_ssize_t)out_len) < 0) {
            Py_DECREF(ba);
            return NULL;
        }
    }
    return ba;
}

static PyObject *bmp565_read_pixel(GfxBmp565Object *self, long pos) {
    uint8_t out[2];
    size_t out_len = 0;
    if (gfx_bmp565_read_bytes(&self->bmp, (int)pos, (int)pos + 1, out, sizeof(out), &out_len) < 0) {
        PyErr_SetString(PyExc_ValueError, "BMP read failed");
        return NULL;
    }
    return PyLong_FromLong((long)(out[0] | (out[1] << 8)));
}

static int slice_bounds(PyObject *slice, Py_ssize_t length, long *start, long *stop) {
    Py_ssize_t s, e, step;
    if (PySlice_Unpack(slice, &s, &e, &step) < 0) {
        return -1;
    }
    PySlice_AdjustIndices(length, &s, &e, step);
    *start = (long)s;
    *stop = (long)e;
    return 0;
}

static PyObject *bmp565_subscript(GfxBmp565Object *self, PyObject *key) {
    if (PyTuple_Check(key)) {
        if (PyTuple_GET_SIZE(key) != 2) {
            PyErr_SetString(PyExc_ValueError, "Invalid key");
            return NULL;
        }
        PyObject *kx = PyTuple_GET_ITEM(key, 0);
        PyObject *ky = PyTuple_GET_ITEM(key, 1);
        if (PyLong_Check(kx) && PyLong_Check(ky)) {
            long x = PyLong_AsLong(kx);
            long y = PyLong_AsLong(ky);
            if (PyErr_Occurred()) {
                return NULL;
            }
            return bmp565_read_pixel(self, y * self->bmp.width + x);
        }
        if (PySlice_Check(kx) && PySlice_Check(ky)) {
            long x0, x1, y0, y1;
            if (slice_bounds(kx, self->bmp.width, &x0, &x1) < 0) {
                return NULL;
            }
            if (slice_bounds(ky, self->bmp.height, &y0, &y1) < 0) {
                return NULL;
            }
            return bmp565_region(self, x0, x1, y0, y1);
        }
        PyErr_SetString(PyExc_ValueError, "Invalid key");
        return NULL;
    }
    if (PySlice_Check(key)) {
        long y0, y1;
        if (slice_bounds(key, self->bmp.height, &y0, &y1) < 0) {
            return NULL;
        }
        return bmp565_region(self, 0, self->bmp.width, y0, y1);
    }
    if (PyLong_Check(key)) {
        long k = PyLong_AsLong(key);
        if (k == -1 && PyErr_Occurred()) {
            return NULL;
        }
        return bmp565_read_pixel(self, k);
    }
    PyErr_SetString(PyExc_ValueError, "Invalid key");
    return NULL;
}

static PyObject *bmp565_call(GfxBmp565Object *self, PyObject *args, PyObject *kwds) {
    (void)kwds;
    int x, y, w, h;
    if (!PyArg_ParseTuple(args, "iiii", &x, &y, &w, &h)) {
        return NULL;
    }
    return bmp565_region(self, x, x + w, y, y + h);
}

static PyObject *bmp565_save(GfxBmp565Object *self, PyObject *args) {
    PyObject *filename = NULL;
    if (!PyArg_ParseTuple(args, "|O", &filename)) {
        return NULL;
    }
    const char *path = NULL;
    if (filename && filename != Py_None) {
        path = PyUnicode_AsUTF8(filename);
        if (!path) {
            return NULL;
        }
    } else if (self->filename_obj) {
        path = PyUnicode_AsUTF8(self->filename_obj);
        if (!path) {
            return NULL;
        }
    }
    char out_path[512];
    if (gfx_bmp565_save_versioned(&self->bmp, path, out_path, sizeof(out_path)) < 0) {
        PyErr_SetString(PyExc_ValueError, "BMP save failed");
        return NULL;
    }
    return PyUnicode_FromString(out_path);
}

static PyObject *bmp565_deinit(GfxBmp565Object *self, PyObject *noargs) {
    (void)noargs;
    gfx_bmp565_deinit(&self->bmp);
    if (self->has_src_view) {
        PyBuffer_Release(&self->src_view);
        self->has_src_view = 0;
    }
    Py_CLEAR(self->buf_obj);
    Py_CLEAR(self->filename_obj);
    Py_RETURN_NONE;
}

static PyObject *bmp565_get_width(GfxBmp565Object *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->bmp.width);
}
static PyObject *bmp565_get_height(GfxBmp565Object *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->bmp.height);
}
static PyObject *bmp565_get_buffer(GfxBmp565Object *self, void *closure) {
    (void)closure;
    if (!self->buf_obj) {
        Py_RETURN_NONE;
    }
    Py_INCREF(self->buf_obj);
    return self->buf_obj;
}
static PyObject *bmp565_get_bpp(GfxBmp565Object *self, void *closure) {
    (void)self;
    (void)closure;
    return PyLong_FromLong(GFX_BMP565_BPP);
}
static PyObject *bmp565_get_BPP(GfxBmp565Object *self, void *closure) {
    (void)self;
    (void)closure;
    return PyLong_FromLong(GFX_BMP565_BYTES_PER_PIXEL);
}

static PyGetSetDef bmp565_getset[] = {
    {"width", (getter)bmp565_get_width, NULL, NULL},
    {"height", (getter)bmp565_get_height, NULL, NULL},
    {"buffer", (getter)bmp565_get_buffer, NULL, NULL},
    {"bpp", (getter)bmp565_get_bpp, NULL, NULL},
    {"BPP", (getter)bmp565_get_BPP, NULL, NULL},
    {NULL},
};

static PyMethodDef bmp565_methods[] = {
    {"save", (PyCFunction)bmp565_save, METH_VARARGS, NULL},
    {"deinit", (PyCFunction)bmp565_deinit, METH_NOARGS, NULL},
    {NULL},
};

static PyMappingMethods bmp565_as_mapping = {
    .mp_subscript = (binaryfunc)bmp565_subscript,
};

static PyTypeObject GfxBmp565Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "graphics.BMP565",
    .tp_basicsize = sizeof(GfxBmp565Object),
    .tp_dealloc = (destructor)bmp565_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = bmp565_getset,
    .tp_methods = bmp565_methods,
    .tp_as_mapping = &bmp565_as_mapping,
    .tp_call = (ternaryfunc)bmp565_call,
    .tp_new = bmp565_new,
};

/* ------------------------------------------------------------------------- */
/* Module                                                                    */
/* ------------------------------------------------------------------------- */

static PyMethodDef module_methods[] = {
    {"framebuf_backend", mod_framebuf_backend, METH_NOARGS, NULL},
    {"implementation", mod_implementation, METH_NOARGS, NULL},
    {"capabilities", mod_capabilities, METH_NOARGS, NULL},
    {"fill", mod_fill, METH_VARARGS, NULL},
    {"fill_rect", mod_fill_rect, METH_VARARGS, NULL},
    {"pixel", mod_pixel, METH_VARARGS, NULL},
    {"hline", mod_hline, METH_VARARGS, NULL},
    {"vline", mod_vline, METH_VARARGS, NULL},
    {"line", mod_line, METH_VARARGS, NULL},
    {"rect", (PyCFunction)mod_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"round_rect", (PyCFunction)mod_round_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"circle", (PyCFunction)mod_circle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"ellipse", (PyCFunction)mod_ellipse, METH_VARARGS | METH_KEYWORDS, NULL},
    {"arc", mod_arc, METH_VARARGS, NULL},
    {"triangle", (PyCFunction)mod_triangle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"gradient_rect", (PyCFunction)mod_gradient_rect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"poly", (PyCFunction)mod_poly, METH_VARARGS | METH_KEYWORDS, NULL},
    {"blit", (PyCFunction)mod_blit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"blit_rect", mod_blit_rect, METH_VARARGS, NULL},
    {"blit_transparent", mod_blit_transparent, METH_VARARGS, NULL},
    {"polygon", (PyCFunction)mod_polygon, METH_VARARGS | METH_KEYWORDS, NULL},
    {"load_image", mod_load_image, METH_VARARGS, NULL},
    {"save_image", mod_save_image, METH_VARARGS, NULL},
    {"bmp_to_framebuffer", mod_bmp_to_framebuffer, METH_VARARGS, NULL},
    {"pbm_to_framebuffer", mod_pbm_to_framebuffer, METH_VARARGS, NULL},
    {"pgm_to_framebuffer", mod_pgm_to_framebuffer, METH_VARARGS, NULL},
    {"text", (PyCFunction)mod_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text8", (PyCFunction)mod_text8, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text14", (PyCFunction)mod_text14, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text16", (PyCFunction)mod_text16, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL},
};

static struct PyModuleDef graphics_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "graphics",
    .m_doc = "Native graphics module for CPython",
    .m_size = -1,
    .m_methods = module_methods,
};

PyMODINIT_FUNC PyInit_graphics(void) {
    PyObject *m = PyModule_Create(&graphics_module);
    if (!m) {
        return NULL;
    }
    if (PyType_Ready(&GfxAreaType) < 0 || PyType_Ready(&GfxFrameBufferType) < 0
        || PyType_Ready(&GfxDrawType) < 0 || PyType_Ready(&GfxClipCtxType) < 0
        || PyType_Ready(&GfxFontType) < 0 || PyType_Ready(&GfxBmp565Type) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    Py_INCREF(&GfxAreaType);
    Py_INCREF(&GfxFrameBufferType);
    Py_INCREF(&GfxDrawType);
    Py_INCREF(&GfxFontType);
    Py_INCREF(&GfxBmp565Type);
    if (PyModule_AddObject(m, "Area", (PyObject *)&GfxAreaType) < 0
        || PyModule_AddObject(m, "FrameBuffer", (PyObject *)&GfxFrameBufferType) < 0
        || PyModule_AddObject(m, "Draw", (PyObject *)&GfxDrawType) < 0
        || PyModule_AddObject(m, "Font", (PyObject *)&GfxFontType) < 0
        || PyModule_AddObject(m, "BMP565", (PyObject *)&GfxBmp565Type) < 0) {
        Py_DECREF(m);
        return NULL;
    }
#define ADD_INT(name, val) \
    do { \
        if (PyModule_AddIntConstant(m, name, val) < 0) { \
            Py_DECREF(m); \
            return NULL; \
        } \
    } while (0)
    ADD_INT("MONO_VLSB", GFX_MVLSB);
    ADD_INT("MONO_HLSB", GFX_MHLSB);
    ADD_INT("MONO_HMSB", GFX_MHMSB);
    ADD_INT("RGB565", GFX_RGB565);
    ADD_INT("GS2_HMSB", GFX_GS2_HMSB);
    ADD_INT("GS4_HMSB", GFX_GS4_HMSB);
    ADD_INT("GS8", GFX_GS8);
    ADD_INT("RGB888", GFX_RGB888);
#undef ADD_INT
    return m;
}
