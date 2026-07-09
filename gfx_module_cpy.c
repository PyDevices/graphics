/*
 * graphics — CPython extension module.
 * SPDX-License-Identifier: MIT
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <limits.h>
#include <stdint.h>

#include "gfx_core.h"
#include "gfx_framebuffer.h"
#include "gfx_shapes.h"
#include "gfx_draw.h"
#include "gfx_font.h"
#include "gfx_capabilities.h"

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
        return 0;
    }
    PyErr_SetString(PyExc_TypeError, "FrameBuffer or canvas required");
    (void)fb_storage;
    (void)fb_keep;
    return -1;
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

static PyMethodDef module_methods[] = {
    {"framebuf_backend", mod_framebuf_backend, METH_NOARGS, NULL},
    {"implementation", mod_implementation, METH_NOARGS, NULL},
    {"capabilities", mod_capabilities, METH_NOARGS, NULL},
    {"fill_rect", mod_fill_rect, METH_VARARGS, NULL},
    {"text8", mod_text8, METH_VARARGS, NULL},
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
    if (PyType_Ready(&GfxAreaType) < 0 || PyType_Ready(&GfxFrameBufferType) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    Py_INCREF(&GfxAreaType);
    Py_INCREF(&GfxFrameBufferType);
    if (PyModule_AddObject(m, "Area", (PyObject *)&GfxAreaType) < 0
        || PyModule_AddObject(m, "FrameBuffer", (PyObject *)&GfxFrameBufferType) < 0) {
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
