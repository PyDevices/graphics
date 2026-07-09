/*
 * graphics — CPython extension module.
 * SPDX-License-Identifier: MIT
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
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

/* ------------------------------------------------------------------------- */
/* Area                                                                      */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    gfx_area_t area;
} GfxAreaObject;

static PyTypeObject GfxAreaType;

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
    GfxAreaObject *a = area_check(self);
    GfxAreaObject *b = area_check(other);
    if (!a || !b) {
        return NULL;
    }
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
/* FrameBuffer                                                               */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    PyObject *buf_obj;
    gfx_fb_t fb;
    gfx_canvas_t canvas;
} GfxFrameBufferObject;

static PyTypeObject GfxFrameBufferType;

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
    gfx_area_t result = gfx_shapes_pixel(&self->canvas, x, y, col);
    return area_from_gfx(&result);
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

static PyObject *framebuffer_rect(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, w, h, col;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "iiiii|p", &x, &y, &w, &h, &col, &fill)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_rect(&self->canvas, x, y, w, h, col, fill);
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

static PyObject *framebuffer_ellipse(GfxFrameBufferObject *self, PyObject *args) {
    int cx, cy, xradius, yradius, col;
    int fill = 0;
    int mask_part = 0x0f;
    if (!PyArg_ParseTuple(args, "iiiii|pi", &cx, &cy, &xradius, &yradius, &col, &fill, &mask_part)) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_ellipse(&self->canvas, cx, cy, xradius, yradius, col, fill, mask_part);
    return area_from_gfx(&result);
}

static PyObject *framebuffer_poly(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, col;
    PyObject *coords;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "iiOi|p", &x, &y, &coords, &col, &fill)) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(coords, &view, PyBUF_FORMAT) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_poly(
        &self->canvas, x, y, view.buf, (size_t)view.len, (size_t)view.itemsize, view.format, col, fill);
    PyBuffer_Release(&view);
    return area_from_gfx(&area);
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
    {"fill", (PyCFunction)framebuffer_fill, METH_VARARGS, NULL},
    {"fill_rect", (PyCFunction)framebuffer_fill_rect, METH_VARARGS, NULL},
    {"pixel", (PyCFunction)framebuffer_pixel, METH_VARARGS, NULL},
    {"hline", (PyCFunction)framebuffer_hline, METH_VARARGS, NULL},
    {"vline", (PyCFunction)framebuffer_vline, METH_VARARGS, NULL},
    {"rect", (PyCFunction)framebuffer_rect, METH_VARARGS, NULL},
    {"line", (PyCFunction)framebuffer_line, METH_VARARGS, NULL},
    {"ellipse", (PyCFunction)framebuffer_ellipse, METH_VARARGS, NULL},
    {"poly", (PyCFunction)framebuffer_poly, METH_VARARGS, NULL},
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
/* Module helpers                                                            */
/* ------------------------------------------------------------------------- */

static int get_canvas_from_target(PyObject *target, gfx_canvas_t *canvas, gfx_fb_t *fb_storage, PyObject **fb_keep) {
    if (PyObject_TypeCheck(target, &GfxFrameBufferType)) {
        GfxFrameBufferObject *fb = (GfxFrameBufferObject *)target;
        *canvas = fb->canvas;
        if (fb_storage) {
            *fb_storage = fb->fb;
        }
        if (fb_keep) {
            *fb_keep = target;
        }
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "FrameBuffer or canvas required");
    (void)fb_storage;
    (void)fb_keep;
    return -1;
}

static int get_readonly_framebuffer(PyObject *arg, gfx_fb_t *fb_out, gfx_canvas_t *canvas_out, PyObject **keep) {
    if (PyObject_TypeCheck(arg, &GfxFrameBufferType)) {
        GfxFrameBufferObject *fb = (GfxFrameBufferObject *)arg;
        *fb_out = fb->fb;
        *canvas_out = fb->canvas;
        if (keep) {
            *keep = arg;
        }
        return 0;
    }
    PyObject *fast = PySequence_Fast(arg, "expected sequence");
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
    Py_buffer view;
    if (PyObject_GetBuffer(items[0], &view, PyBUF_READ) < 0) {
        Py_DECREF(fast);
        return -1;
    }
    long width = PyLong_AsLong(items[1]);
    long height = PyLong_AsLong(items[2]);
    long format = PyLong_AsLong(items[3]);
    long stride = len >= 5 ? PyLong_AsLong(items[4]) : width;
    if ((width == -1 || height == -1 || format == -1 || stride == -1) && PyErr_Occurred()) {
        PyBuffer_Release(&view);
        Py_DECREF(fast);
        return -1;
    }
    if (gfx_fb_validate_buffer(view.len, (int)width, (int)height, (int)format, (int)stride) < 0) {
        PyBuffer_Release(&view);
        Py_DECREF(fast);
        PyErr_SetString(PyExc_ValueError, "invalid framebuffer parameters");
        return -1;
    }
    fb_out->buf = view.buf;
    fb_out->width = (uint16_t)width;
    fb_out->height = (uint16_t)height;
    fb_out->stride = (uint16_t)stride;
    fb_out->format = (uint8_t)format;
    gfx_fb_canvas_init(canvas_out, fb_out);
    if (keep) {
        Py_INCREF(items[0]);
        *keep = items[0];
    }
    Py_DECREF(fast);
    return 0;
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
    PyObject *fb = PyObject_New(GfxFrameBufferObject, &GfxFrameBufferType);
    if (!fb) {
        Py_DECREF(buf);
        return NULL;
    }
    if (framebuffer_init_from_buffer(buf, width, height, format, width, (GfxFrameBufferObject *)fb) < 0) {
        Py_DECREF(fb);
        Py_DECREF(buf);
        return NULL;
    }
    return fb;
}

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
    return caps;
}

static PyObject *mod_fill_rect(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, w, h, c;
    if (!PyArg_ParseTuple(args, "Oiiiii", &target, &x, &y, &w, &h, &c)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fb_storage;
    PyObject *fb_keep = NULL;
    if (get_canvas_from_target(target, &canvas, &fb_storage, &fb_keep) < 0) {
        return NULL;
    }
    gfx_area_t result = gfx_shapes_fill_rect(&canvas, x, y, w, h, c);
    return area_from_gfx(&result);
}

static PyObject *mod_text8(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    const char *str;
    int x, y, col;
    if (!PyArg_ParseTuple(args, "Osiii", &target, &str, &x, &y, &col)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fb_storage;
    PyObject *fb_keep = NULL;
    if (get_canvas_from_target(target, &canvas, &fb_storage, &fb_keep) < 0) {
        return NULL;
    }
    gfx_area_t result = gfx_font_text8(&canvas, str, x, y, col);
    return area_from_gfx(&result);
}

#define MOD_SHAPE5(name, fn) \
static PyObject *mod_##name(PyObject *self, PyObject *args) { \
    (void)self; \
    PyObject *target; int x, y, a, b, c; \
    if (!PyArg_ParseTuple(args, "Oiiiii", &target, &x, &y, &a, &b, &c)) return NULL; \
    gfx_canvas_t canvas; gfx_fb_t fs; PyObject *fk = NULL; \
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) return NULL; \
    gfx_area_t area = fn(&canvas, x, y, a, b, c); \
    return area_from_gfx(&area); \
}

static PyObject *mod_fill(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int col;
    if (!PyArg_ParseTuple(args, "Oi", &target, &col)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_fill(&canvas, col);
    return area_from_gfx(&area);
}

static PyObject *mod_pixel(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, c;
    if (!PyArg_ParseTuple(args, "Oiii", &target, &x, &y, &c)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_pixel(&canvas, x, y, c);
    return area_from_gfx(&area);
}

static PyObject *mod_hline(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, w, c;
    if (!PyArg_ParseTuple(args, "Oiiii", &target, &x, &y, &w, &c)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_hline(&canvas, x, y, w, c);
    return area_from_gfx(&area);
}

static PyObject *mod_vline(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, h, c;
    if (!PyArg_ParseTuple(args, "Oiiii", &target, &x, &y, &h, &c)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_vline(&canvas, x, y, h, c);
    return area_from_gfx(&area);
}

static PyObject *mod_line(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x1, y1, x2, y2, col;
    if (!PyArg_ParseTuple(args, "Oiiiii", &target, &x1, &y1, &x2, &y2, &col)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_line(&canvas, x1, y1, x2, y2, col);
    return area_from_gfx(&area);
}

static PyObject *mod_rect(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, w, h, col;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "Oiiiii|p", &target, &x, &y, &w, &h, &col, &fill)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_rect(&canvas, x, y, w, h, col, fill);
    return area_from_gfx(&area);
}

static PyObject *mod_round_rect(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, w, h, r, col;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "Oiiiiii|p", &target, &x, &y, &w, &h, &r, &col, &fill)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_round_rect(&canvas, x, y, w, h, r, col, fill);
    return area_from_gfx(&area);
}

static PyObject *mod_circle(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, r, col;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "Oiiii|p", &target, &x, &y, &r, &col, &fill)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_circle(&canvas, x, y, r, col, fill);
    return area_from_gfx(&area);
}

static PyObject *mod_text14(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    const char *str;
    int x, y, col;
    if (!PyArg_ParseTuple(args, "Osiii", &target, &str, &x, &y, &col)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_font_text14(&canvas, str, x, y, col);
    return area_from_gfx(&area);
}

static PyObject *mod_text16(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    const char *str;
    int x, y, col;
    if (!PyArg_ParseTuple(args, "Osiii", &target, &str, &x, &y, &col)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_font_text16(&canvas, str, x, y, col);
    return area_from_gfx(&area);
}

static PyObject *mod_text(PyObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"canvas", "s", "x", "y", "c", "height", NULL};
    PyObject *target;
    const char *str;
    int x, y, col = 1;
    int height = 8;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Osii|ii", kwlist, &target, &str, &x, &y, &col, &height)) {
        return NULL;
    }
    if (height == 14) {
        return mod_text14(self, Py_BuildValue("Osiii", target, str, x, y, col));
    }
    if (height == 16) {
        return mod_text16(self, Py_BuildValue("Osiii", target, str, x, y, col));
    }
    return mod_text8(self, Py_BuildValue("Osiii", target, str, x, y, col));
}

static PyObject *mod_ellipse(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int cx, cy, xradius, yradius, col;
    int fill = 0;
    int mask_part = 0x0f;
    if (!PyArg_ParseTuple(args, "Oiiiii|pi", &target, &cx, &cy, &xradius, &yradius, &col, &fill, &mask_part)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_ellipse(&canvas, cx, cy, xradius, yradius, col, fill, mask_part);
    return area_from_gfx(&area);
}

static PyObject *mod_arc(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, r, col;
    double a0, a1;
    if (!PyArg_ParseTuple(args, "Oiiiddi", &target, &x, &y, &r, &a0, &a1, &col)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_arc(&canvas, x, y, r, (float)a0, (float)a1, col);
    return area_from_gfx(&area);
}

static PyObject *mod_triangle(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x0, y0, x1, y1, x2, y2, col;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "Oiiiiiii|p", &target, &x0, &y0, &x1, &y1, &x2, &y2, &col, &fill)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_triangle(&canvas, x0, y0, x1, y1, x2, y2, col, fill);
    return area_from_gfx(&area);
}

static PyObject *mod_gradient_rect(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, w, h, c1, c2;
    int vertical = 1;
    if (!PyArg_ParseTuple(args, "Oiiiii|ip", &target, &x, &y, &w, &h, &c1, &c2, &vertical)) {
        return NULL;
    }
    if (PyTuple_GET_SIZE(args) < 7) {
        c2 = c1;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_gradient_rect(&canvas, x, y, w, h, c1, c2, vertical);
    return area_from_gfx(&area);
}

static PyObject *mod_poly(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    int x, y, col;
    PyObject *coords;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "OiiOi|p", &target, &x, &y, &coords, &col, &fill)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(coords, &view, PyBUF_READ) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_poly(
        &canvas, x, y, view.buf, (size_t)view.len, (size_t)view.itemsize, view.format, col, fill);
    PyBuffer_Release(&view);
    return area_from_gfx(&area);
}

static PyObject *mod_blit(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    PyObject *source;
    int x, y;
    int key = -1;
    PyObject *palette = NULL;
    if (!PyArg_ParseTuple(args, "OOii|iO", &target, &source, &x, &y, &key, &palette)) {
        return NULL;
    }
    gfx_canvas_t dest_canvas;
    gfx_fb_t dest_fb;
    PyObject *dest_keep = NULL;
    if (get_canvas_from_target(target, &dest_canvas, &dest_fb, &dest_keep) < 0) {
        return NULL;
    }
    gfx_fb_t source_fb;
    gfx_canvas_t source_canvas;
    PyObject *source_keep = NULL;
    if (get_readonly_framebuffer(source, &source_fb, &source_canvas, &source_keep) < 0) {
        return NULL;
    }
    const gfx_fb_t *pal = NULL;
    gfx_fb_t palette_fb;
    gfx_canvas_t palette_canvas;
    PyObject *palette_keep = NULL;
    if (palette != NULL && palette != Py_None) {
        if (get_readonly_framebuffer(palette, &palette_fb, &palette_canvas, &palette_keep) < 0) {
            Py_XDECREF(source_keep);
            return NULL;
        }
        pal = &palette_fb;
    }
    gfx_area_t area = gfx_shapes_blit(&dest_canvas, &source_fb, x, y, key, pal);
    Py_XDECREF(source_keep);
    Py_XDECREF(palette_keep);
    return area_from_gfx(&area);
}

static PyObject *mod_blit_rect(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    PyObject *buf;
    int x, y, w, h;
    if (!PyArg_ParseTuple(args, "OOiiii", &target, &buf, &x, &y, &w, &h)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(buf, &view, PyBUF_READ) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit_rect(&canvas, view.buf, x, y, w, h, 2);
    PyBuffer_Release(&view);
    return area_from_gfx(&area);
}

static PyObject *mod_blit_transparent(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    PyObject *buf;
    int x, y, w, h, key;
    if (!PyArg_ParseTuple(args, "OOiiiii", &target, &buf, &x, &y, &w, &h, &key)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(buf, &view, PyBUF_READ) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_blit_transparent(&canvas, view.buf, x, y, w, h, key, 2);
    PyBuffer_Release(&view);
    return area_from_gfx(&area);
}

static PyObject *mod_polygon(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *target;
    PyObject *points_seq;
    int x, y, col;
    double angle = 0.0;
    int cx = 0;
    int cy = 0;
    if (!PyArg_ParseTuple(args, "OOiii|ddi", &target, &points_seq, &x, &y, &col, &angle, &cx, &cy)) {
        return NULL;
    }
    gfx_canvas_t canvas;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(target, &canvas, &fs, &fk) < 0) {
        return NULL;
    }
    PyObject *fast = PySequence_Fast(points_seq, "expected sequence of points");
    if (!fast) {
        return NULL;
    }
    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast);
    if (len < 3 || len > 64) {
        Py_DECREF(fast);
        PyErr_SetString(PyExc_ValueError, "Polygon must have 3 to 64 points");
        return NULL;
    }
    int points[128];
    PyObject **items = PySequence_Fast_ITEMS(fast);
    for (Py_ssize_t i = 0; i < len; i++) {
        if (!PyTuple_Check(items[i]) || PyTuple_GET_SIZE(items[i]) != 2) {
            Py_DECREF(fast);
            PyErr_SetString(PyExc_TypeError, "expected sequence of (x, y) tuples");
            return NULL;
        }
        points[i * 2] = (int)PyLong_AsLong(PyTuple_GET_ITEM(items[i], 0));
        points[i * 2 + 1] = (int)PyLong_AsLong(PyTuple_GET_ITEM(items[i], 1));
        if (PyErr_Occurred()) {
            Py_DECREF(fast);
            return NULL;
        }
    }
    Py_DECREF(fast);
    gfx_area_t area = gfx_shapes_polygon(&canvas, points, (size_t)len, x, y, col, (float)angle, cx, cy);
    return area_from_gfx(&area);
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
    char out_path[256];
    if (gfx_files_save_image(&fb->fb, path, out_path, sizeof(out_path)) < 0) {
        PyErr_SetString(PyExc_ValueError, "save_image failed");
        return NULL;
    }
    return PyUnicode_FromString(out_path);
}

typedef struct {
    PyObject_HEAD
    PyObject *canvas_obj;
    gfx_draw_t draw;
} GfxDrawObject;

static PyObject *draw_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    (void)kwds;
    PyObject *canvas;
    if (!PyArg_ParseTuple(args, "O", &canvas)) {
        return NULL;
    }
    if (!PyObject_TypeCheck(canvas, &GfxFrameBufferType)) {
        PyErr_SetString(PyExc_TypeError, "FrameBuffer required");
        return NULL;
    }
    GfxDrawObject *o = (GfxDrawObject *)type->tp_alloc(type, 0);
    if (!o) {
        return NULL;
    }
    o->canvas_obj = canvas;
    Py_INCREF(canvas);
    GfxFrameBufferObject *fb = (GfxFrameBufferObject *)canvas;
    gfx_draw_init(&o->draw, &fb->canvas);
    return (PyObject *)o;
}

static void draw_dealloc(GfxDrawObject *self) {
    Py_XDECREF(self->canvas_obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static const gfx_canvas_t *draw_target(GfxDrawObject *self) {
    GfxFrameBufferObject *fb = (GfxFrameBufferObject *)self->canvas_obj;
    self->draw.canvas = fb->canvas;
    return gfx_draw_target(&self->draw);
}

static PyObject *draw_fill_rect(GfxDrawObject *self, PyObject *args) {
    int x, y, w, h, c;
    if (!PyArg_ParseTuple(args, "iiiii", &x, &y, &w, &h, &c)) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_fill_rect(draw_target(self), x, y, w, h, c);
    return area_from_gfx(&area);
}

static PyObject *draw_fill(GfxDrawObject *self, PyObject *args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_fill(draw_target(self), col);
    return area_from_gfx(&area);
}

static PyObject *draw_rect(GfxDrawObject *self, PyObject *args) {
    int x, y, w, h, col;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "iiiii|p", &x, &y, &w, &h, &col, &fill)) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_rect(draw_target(self), x, y, w, h, col, fill);
    return area_from_gfx(&area);
}

static PyObject *draw_line(GfxDrawObject *self, PyObject *args) {
    int x1, y1, x2, y2, col;
    if (!PyArg_ParseTuple(args, "iiiii", &x1, &y1, &x2, &y2, &col)) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_line(draw_target(self), x1, y1, x2, y2, col);
    return area_from_gfx(&area);
}

static PyObject *draw_round_rect(GfxDrawObject *self, PyObject *args) {
    int x, y, w, h, r, col;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "iiiiii|p", &x, &y, &w, &h, &r, &col, &fill)) {
        return NULL;
    }
    gfx_area_t area = gfx_shapes_round_rect(draw_target(self), x, y, w, h, r, col, fill);
    return area_from_gfx(&area);
}

static PyObject *draw_text(GfxDrawObject *self, PyObject *args) {
    const char *str;
    int x, y;
    int col = 1;
    if (!PyArg_ParseTuple(args, "sii|i", &str, &x, &y, &col)) {
        return NULL;
    }
    gfx_area_t area = gfx_font_text8(draw_target(self), str, x, y, col);
    return area_from_gfx(&area);
}

static PyObject *draw_text14(GfxDrawObject *self, PyObject *args) {
    const char *str;
    int x, y;
    int col = 1;
    if (!PyArg_ParseTuple(args, "sii|i", &str, &x, &y, &col)) {
        return NULL;
    }
    gfx_area_t area = gfx_font_text14(draw_target(self), str, x, y, col);
    return area_from_gfx(&area);
}

static PyMethodDef draw_methods[] = {
    {"fill", (PyCFunction)draw_fill, METH_VARARGS, NULL},
    {"fill_rect", (PyCFunction)draw_fill_rect, METH_VARARGS, NULL},
    {"rect", (PyCFunction)draw_rect, METH_VARARGS, NULL},
    {"line", (PyCFunction)draw_line, METH_VARARGS, NULL},
    {"round_rect", (PyCFunction)draw_round_rect, METH_VARARGS, NULL},
    {"text", (PyCFunction)draw_text, METH_VARARGS, NULL},
    {"text14", (PyCFunction)draw_text14, METH_VARARGS, NULL},
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

typedef struct {
    PyObject_HEAD
    gfx_font_t font;
} GfxFontObject;

static PyTypeObject GfxFontType;

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
    (void)kwds;
    PyObject *font_data = Py_None;
    int height = 8;
    if (!PyArg_ParseTuple(args, "|Oi", &font_data, &height)) {
        return NULL;
    }
    GfxFontObject *o = (GfxFontObject *)type->tp_alloc(type, 0);
    if (!o) {
        return NULL;
    }
    if (font_data == Py_None) {
        gfx_font_init_default(&o->font, height);
    } else if (PyUnicode_Check(font_data)) {
        const char *path = PyUnicode_AsUTF8(font_data);
        if (PyTuple_GET_SIZE(args) < 2 && parse_font_height_from_name(path, &height) < 0) {
            Py_DECREF(o);
            PyErr_SetString(PyExc_ValueError, "Invalid font");
            return NULL;
        }
        FILE *f = fopen(path, "rb");
        if (!f) {
            Py_DECREF(o);
            PyErr_SetString(PyExc_OSError, "Font not found");
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
    } else {
        Py_buffer view;
        if (PyObject_GetBuffer(font_data, &view, PyBUF_READ) < 0) {
            Py_DECREF(o);
            return NULL;
        }
        gfx_font_init_from_data(&o->font, view.buf, (size_t)view.len, height);
        PyBuffer_Release(&view);
    }
    return (PyObject *)o;
}

static void font_dealloc(GfxFontObject *self) {
    gfx_font_deinit(&self->font);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *font_text(GfxFontObject *self, PyObject *args) {
    PyObject *canvas;
    const char *str;
    int x, y, col;
    int scale = 1;
    int inverted = 0;
    if (!PyArg_ParseTuple(args, "Osiii|ip", &canvas, &str, &x, &y, &col, &scale, &inverted)) {
        return NULL;
    }
    gfx_canvas_t target;
    gfx_fb_t fs;
    PyObject *fk = NULL;
    if (get_canvas_from_target(canvas, &target, &fs, &fk) < 0) {
        return NULL;
    }
    gfx_area_t area = gfx_font_text(&target, &self->font, str, x, y, col, scale, inverted);
    return area_from_gfx(&area);
}

static PyObject *font_get_width(GfxFontObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->font.width);
}

static PyObject *font_get_height(GfxFontObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(self->font.height);
}

static PyGetSetDef font_getset[] = {
    {"width", (getter)font_get_width, NULL, NULL},
    {"height", (getter)font_get_height, NULL, NULL},
    {NULL},
};

static PyMethodDef font_methods[] = {
    {"text", (PyCFunction)font_text, METH_VARARGS, NULL},
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

typedef struct {
    PyObject_HEAD
    gfx_bmp565_t bmp;
    PyObject *buf_obj;
} GfxBmp565Object;

static PyTypeObject GfxBmp565Type;

static PyObject *bmp565_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    (void)kwds;
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    if (n == 0) {
        PyErr_SetString(PyExc_ValueError, "Invalid arguments");
        return NULL;
    }
    GfxBmp565Object *o = (GfxBmp565Object *)type->tp_alloc(type, 0);
    if (!o) {
        return NULL;
    }
    o->buf_obj = Py_None;
    Py_INCREF(Py_None);
    PyObject *arg0 = PyTuple_GET_ITEM(args, 0);
    if (PyUnicode_Check(arg0)) {
        const char *path = PyUnicode_AsUTF8(arg0);
        if (gfx_bmp565_load_from_file(path, &o->bmp) < 0) {
            Py_DECREF(o);
            PyErr_SetString(PyExc_ValueError, "BMP load failed");
            return NULL;
        }
        Py_SETREF(o->buf_obj, PyByteArray_FromStringAndSize((const char *)o->bmp.buffer, (Py_ssize_t)o->bmp.buffer_len));
        if (!o->buf_obj) {
            gfx_bmp565_deinit(&o->bmp);
            Py_DECREF(o);
            return NULL;
        }
    } else {
        if (n < 3) {
            Py_DECREF(o);
            PyErr_SetString(PyExc_ValueError, "Invalid arguments");
            return NULL;
        }
        int width = (int)PyLong_AsLong(PyTuple_GET_ITEM(args, 1));
        int height = (int)PyLong_AsLong(PyTuple_GET_ITEM(args, 2));
        if (PyErr_Occurred()) {
            Py_DECREF(o);
            return NULL;
        }
        Py_buffer view;
        if (PyObject_GetBuffer(arg0, &view, PyBUF_READ) < 0) {
            Py_DECREF(o);
            return NULL;
        }
        Py_SETREF(o->buf_obj, arg0);
        Py_INCREF(arg0);
        Py_DECREF(Py_None);
        gfx_bmp565_init_from_buffer(&o->bmp, view.buf, (size_t)view.len, width, height);
        PyBuffer_Release(&view);
    }
    return (PyObject *)o;
}

static void bmp565_dealloc(GfxBmp565Object *self) {
    gfx_bmp565_deinit(&self->bmp);
    Py_XDECREF(self->buf_obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
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
    Py_INCREF(self->buf_obj);
    return self->buf_obj;
}

static PyGetSetDef bmp565_getset[] = {
    {"width", (getter)bmp565_get_width, NULL, NULL},
    {"height", (getter)bmp565_get_height, NULL, NULL},
    {"buffer", (getter)bmp565_get_buffer, NULL, NULL},
    {NULL},
};

static PyTypeObject GfxBmp565Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "graphics.BMP565",
    .tp_basicsize = sizeof(GfxBmp565Object),
    .tp_dealloc = (destructor)bmp565_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = bmp565_getset,
    .tp_new = bmp565_new,
};

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
    {"rect", mod_rect, METH_VARARGS, NULL},
    {"round_rect", mod_round_rect, METH_VARARGS, NULL},
    {"circle", mod_circle, METH_VARARGS, NULL},
    {"ellipse", mod_ellipse, METH_VARARGS, NULL},
    {"arc", mod_arc, METH_VARARGS, NULL},
    {"triangle", mod_triangle, METH_VARARGS, NULL},
    {"gradient_rect", mod_gradient_rect, METH_VARARGS, NULL},
    {"poly", mod_poly, METH_VARARGS, NULL},
    {"blit", mod_blit, METH_VARARGS, NULL},
    {"blit_rect", mod_blit_rect, METH_VARARGS, NULL},
    {"blit_transparent", mod_blit_transparent, METH_VARARGS, NULL},
    {"polygon", mod_polygon, METH_VARARGS, NULL},
    {"load_image", mod_load_image, METH_VARARGS, NULL},
    {"save_image", mod_save_image, METH_VARARGS, NULL},
    {"bmp_to_framebuffer", mod_bmp_to_framebuffer, METH_VARARGS, NULL},
    {"pbm_to_framebuffer", mod_pbm_to_framebuffer, METH_VARARGS, NULL},
    {"pgm_to_framebuffer", mod_pgm_to_framebuffer, METH_VARARGS, NULL},
    {"text", (PyCFunction)mod_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"text8", mod_text8, METH_VARARGS, NULL},
    {"text14", mod_text14, METH_VARARGS, NULL},
    {"text16", mod_text16, METH_VARARGS, NULL},
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
        || PyType_Ready(&GfxDrawType) < 0 || PyType_Ready(&GfxFontType) < 0
        || PyType_Ready(&GfxBmp565Type) < 0) {
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
