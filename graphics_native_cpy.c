/*
 * graphics_native — CPython extension (FrameBuffer + Area).
 * SPDX-License-Identifier: MIT
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "gfx_core.h"
#include "font_petme128_8x8.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

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

static PyObject *area_from_gfx(const gfx_area_t *a) {
    GfxAreaObject *o = PyObject_New(GfxAreaObject, &GfxAreaType);
    if (!o) {
        return NULL;
    }
    o->area = *a;
    return (PyObject *)o;
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
    .tp_name = "graphics_native.Area",
    .tp_basicsize = sizeof(GfxAreaObject),
    .tp_dealloc = (destructor)area_dealloc,
    .tp_repr = (reprfunc)area_repr,
    .tp_as_number = &area_as_number,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "Rectangle geometry helper",
    .tp_methods = area_methods,
    .tp_getset = area_getset,
    .tp_new = area_new,
    .tp_iter = (getiterfunc)area_iter,
    .tp_richcompare = area_richcompare,
    .tp_hash = PyObject_HashNotImplemented,
};

/* ------------------------------------------------------------------------- */
/* FrameBuffer (algorithms from modframebuf.c)                                 */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    PyObject *buf_obj;
    void *buf;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint8_t format;
} GfxFrameBufferObject;

static PyTypeObject GfxFrameBufferType;

typedef void (*setpixel_t)(const GfxFrameBufferObject *, unsigned int, unsigned int, uint32_t);
typedef uint32_t (*getpixel_t)(const GfxFrameBufferObject *, unsigned int, unsigned int);
typedef void (*fill_rect_t)(const GfxFrameBufferObject *, unsigned int, unsigned int, unsigned int, unsigned int, uint32_t);

typedef struct {
    setpixel_t setpixel;
    getpixel_t getpixel;
    fill_rect_t fill_rect;
} gfx_framebuf_p_t;

static void mono_horiz_setpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, uint32_t col) {
    size_t index = (x + y * fb->stride) >> 3;
    unsigned int offset = fb->format == GFX_MHMSB ? x & 0x07 : 7 - (x & 0x07);
    ((uint8_t *)fb->buf)[index] = (((uint8_t *)fb->buf)[index] & ~(0x01 << offset)) | ((col != 0) << offset);
}

static uint32_t mono_horiz_getpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y) {
    size_t index = (x + y * fb->stride) >> 3;
    unsigned int offset = fb->format == GFX_MHMSB ? x & 0x07 : 7 - (x & 0x07);
    return (((uint8_t *)fb->buf)[index] >> (offset)) & 0x01;
}

static void mono_horiz_fill_rect(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    unsigned int reverse = fb->format == GFX_MHMSB;
    unsigned int advance = fb->stride >> 3;
    while (w--) {
        uint8_t *b = &((uint8_t *)fb->buf)[(x >> 3) + y * advance];
        unsigned int offset = reverse ? x & 7 : 7 - (x & 7);
        for (unsigned int hh = h; hh; --hh) {
            *b = (*b & ~(0x01 << offset)) | ((col != 0) << offset);
            b += advance;
        }
        ++x;
    }
}

static void mvlsb_setpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, uint32_t col) {
    size_t index = (y >> 3) * fb->stride + x;
    uint8_t offset = y & 0x07;
    ((uint8_t *)fb->buf)[index] = (((uint8_t *)fb->buf)[index] & ~(0x01 << offset)) | ((col != 0) << offset);
}

static uint32_t mvlsb_getpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y) {
    return (((uint8_t *)fb->buf)[(y >> 3) * fb->stride + x] >> (y & 0x07)) & 0x01;
}

static void mvlsb_fill_rect(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    while (h--) {
        uint8_t *b = &((uint8_t *)fb->buf)[(y >> 3) * fb->stride + x];
        uint8_t offset = y & 0x07;
        for (unsigned int ww = w; ww; --ww) {
            *b = (*b & ~(0x01 << offset)) | ((col != 0) << offset);
            ++b;
        }
        ++y;
    }
}

static void rgb565_setpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, uint32_t col) {
    ((uint16_t *)fb->buf)[x + y * fb->stride] = (uint16_t)col;
}

static uint32_t rgb565_getpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y) {
    return ((uint16_t *)fb->buf)[x + y * fb->stride];
}

static void rgb565_fill_rect(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    uint16_t *b = &((uint16_t *)fb->buf)[x + y * fb->stride];
    while (h--) {
        for (unsigned int ww = w; ww; --ww) {
            *b++ = (uint16_t)col;
        }
        b += fb->stride - w;
    }
}

static void gs2_hmsb_setpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, uint32_t col) {
    uint8_t *pixel = &((uint8_t *)fb->buf)[(x + y * fb->stride) >> 2];
    uint8_t shift = (x & 0x3) << 1;
    uint8_t mask = 0x3 << shift;
    uint8_t color = (col & 0x3) << shift;
    *pixel = color | (*pixel & (~mask));
}

static uint32_t gs2_hmsb_getpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y) {
    uint8_t pixel = ((uint8_t *)fb->buf)[(x + y * fb->stride) >> 2];
    uint8_t shift = (x & 0x3) << 1;
    return (pixel >> shift) & 0x3;
}

static void gs2_hmsb_fill_rect(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    for (unsigned int xx = x; xx < x + w; xx++) {
        for (unsigned int yy = y; yy < y + h; yy++) {
            gs2_hmsb_setpixel(fb, xx, yy, col);
        }
    }
}

static void gs4_hmsb_setpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, uint32_t col) {
    uint8_t *pixel = &((uint8_t *)fb->buf)[(x + y * fb->stride) >> 1];
    if (x % 2) {
        *pixel = ((uint8_t)col & 0x0f) | (*pixel & 0xf0);
    } else {
        *pixel = ((uint8_t)col << 4) | (*pixel & 0x0f);
    }
}

static uint32_t gs4_hmsb_getpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y) {
    if (x % 2) {
        return ((uint8_t *)fb->buf)[(x + y * fb->stride) >> 1] & 0x0f;
    }
    return ((uint8_t *)fb->buf)[(x + y * fb->stride) >> 1] >> 4;
}

static void gs4_hmsb_fill_rect(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    col &= 0x0f;
    uint8_t *pixel_pair = &((uint8_t *)fb->buf)[(x + y * fb->stride) >> 1];
    uint8_t col_shifted_left = (uint8_t)(col << 4);
    uint8_t col_pixel_pair = col_shifted_left | (uint8_t)col;
    unsigned int pixel_count_till_next_line = (fb->stride - w) >> 1;
    bool odd_x = (x % 2 == 1);

    while (h--) {
        unsigned int ww = w;
        if (odd_x && ww > 0) {
            *pixel_pair = (*pixel_pair & 0xf0) | (uint8_t)col;
            pixel_pair++;
            ww--;
        }
        memset(pixel_pair, col_pixel_pair, ww >> 1);
        pixel_pair += ww >> 1;
        if (ww % 2) {
            *pixel_pair = col_shifted_left | (*pixel_pair & 0x0f);
            if (!odd_x) {
                pixel_pair++;
            }
        }
        pixel_pair += pixel_count_till_next_line;
    }
}

static void gs8_setpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, uint32_t col) {
    uint8_t *pixel = &((uint8_t *)fb->buf)[(x + y * fb->stride)];
    *pixel = (uint8_t)(col & 0xff);
}

static uint32_t gs8_getpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y) {
    return ((uint8_t *)fb->buf)[(x + y * fb->stride)];
}

static void gs8_fill_rect(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    uint8_t *pixel = &((uint8_t *)fb->buf)[(x + y * fb->stride)];
    while (h--) {
        memset(pixel, (int)col, w);
        pixel += fb->stride;
    }
}

static gfx_framebuf_p_t formats[] = {
    [GFX_MVLSB] = {mvlsb_setpixel, mvlsb_getpixel, mvlsb_fill_rect},
    [GFX_RGB565] = {rgb565_setpixel, rgb565_getpixel, rgb565_fill_rect},
    [GFX_GS2_HMSB] = {gs2_hmsb_setpixel, gs2_hmsb_getpixel, gs2_hmsb_fill_rect},
    [GFX_GS4_HMSB] = {gs4_hmsb_setpixel, gs4_hmsb_getpixel, gs4_hmsb_fill_rect},
    [GFX_GS8] = {gs8_setpixel, gs8_getpixel, gs8_fill_rect},
    [GFX_MHLSB] = {mono_horiz_setpixel, mono_horiz_getpixel, mono_horiz_fill_rect},
    [GFX_MHMSB] = {mono_horiz_setpixel, mono_horiz_getpixel, mono_horiz_fill_rect},
};

static inline void setpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y, uint32_t col) {
    formats[fb->format].setpixel(fb, x, y, col);
}

static void setpixel_checked(const GfxFrameBufferObject *fb, int x, int y, int col, int mask) {
    if (mask && 0 <= x && x < fb->width && 0 <= y && y < fb->height) {
        setpixel(fb, (unsigned int)x, (unsigned int)y, (uint32_t)col);
    }
}

static inline uint32_t getpixel(const GfxFrameBufferObject *fb, unsigned int x, unsigned int y) {
    return formats[fb->format].getpixel(fb, x, y);
}

static void fill_rect(const GfxFrameBufferObject *fb, int x, int y, int w, int h, uint32_t col) {
    if (h < 1 || w < 1 || x + w <= 0 || y + h <= 0 || y >= fb->height || x >= fb->width) {
        return;
    }
    int xend = MIN(fb->width, x + w);
    int yend = MIN(fb->height, y + h);
    x = MAX(x, 0);
    y = MAX(y, 0);
    formats[fb->format].fill_rect(fb, (unsigned int)x, (unsigned int)y, (unsigned int)(xend - x), (unsigned int)(yend - y), col);
}

static int framebuffer_init_from_args(PyObject *buf_obj, int width, int height, int format, int stride, GfxFrameBufferObject *o) {
    if (width < 1 || height < 1 || width > 0xffff || height > 0xffff || stride > 0xffff || stride < width) {
        PyErr_SetString(PyExc_ValueError, "invalid dimensions");
        return -1;
    }

    size_t bpp = 1;
    size_t height_required = (size_t)height;
    size_t width_required = (size_t)width;
    size_t strides_required = (size_t)height - 1;

    switch (format) {
        case GFX_MVLSB:
            height_required = ((size_t)height + 7) & ~7u;
            strides_required = height_required - 8;
            break;
        case GFX_MHLSB:
        case GFX_MHMSB:
            stride = (stride + 7) & ~7;
            width_required = ((size_t)width + 7) & ~7u;
            break;
        case GFX_GS2_HMSB:
            stride = (stride + 3) & ~3;
            width_required = ((size_t)width + 3) & ~3u;
            bpp = 2;
            break;
        case GFX_GS4_HMSB:
            stride = (stride + 1) & ~1;
            width_required = ((size_t)width + 1) & ~1u;
            bpp = 4;
            break;
        case GFX_GS8:
            bpp = 8;
            break;
        case GFX_RGB565:
            bpp = 16;
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "invalid format");
            return -1;
    }

    Py_buffer view;
    if (PyObject_GetBuffer(buf_obj, &view, PyBUF_WRITABLE) < 0) {
        return -1;
    }
    size_t need = (strides_required * (size_t)stride + (height_required - strides_required) * width_required) * bpp / 8;
    if (need > (size_t)view.len) {
        PyBuffer_Release(&view);
        PyErr_SetString(PyExc_ValueError, "buffer too small");
        return -1;
    }

    Py_INCREF(buf_obj);
    if (o->buf_obj) {
        Py_DECREF(o->buf_obj);
    }
    o->buf_obj = buf_obj;
    o->buf = view.buf;
    o->width = (uint16_t)width;
    o->height = (uint16_t)height;
    o->format = (uint8_t)format;
    o->stride = (uint16_t)stride;
    PyBuffer_Release(&view);
    return 0;
}

static int framebuffer_parse_sequence(PyObject *seq, GfxFrameBufferObject *out) {
    PyObject *fast = PySequence_Fast(seq, "expected sequence");
    if (!fast) {
        return -1;
    }
    Py_ssize_t len = PySequence_Fast_GET_SIZE(fast);
    if (len < 4 || len > 5) {
        Py_DECREF(fast);
        PyErr_SetString(PyExc_ValueError, "invalid framebuffer spec");
        return -1;
    }
    PyObject **items = PySequence_Fast_ITEMS(fast);
    long width = PyLong_AsLong(items[1]);
    long height = PyLong_AsLong(items[2]);
    long format = PyLong_AsLong(items[3]);
    long stride = width;
    if (len >= 5) {
        stride = PyLong_AsLong(items[4]);
    }
    if (PyErr_Occurred()) {
        Py_DECREF(fast);
        return -1;
    }
    int rc = framebuffer_init_from_args(items[0], (int)width, (int)height, (int)format, (int)stride, out);
    Py_DECREF(fast);
    return rc;
}

static void framebuffer_copy_from(GfxFrameBufferObject *dst, const GfxFrameBufferObject *src) {
    dst->buf_obj = src->buf_obj;
    dst->buf = src->buf;
    dst->width = src->width;
    dst->height = src->height;
    dst->stride = src->stride;
    dst->format = src->format;
}

static int get_readonly_framebuffer(PyObject *arg, GfxFrameBufferObject *rofb) {
    if (PyObject_TypeCheck(arg, &GfxFrameBufferType)) {
        framebuffer_copy_from(rofb, (const GfxFrameBufferObject *)arg);
        return 0;
    }
    if (PyTuple_Check(arg) || PyList_Check(arg)) {
        return framebuffer_parse_sequence(arg, rofb);
    }
    PyErr_SetString(PyExc_TypeError, "FrameBuffer or sequence required");
    return -1;
}

static PyObject *framebuffer_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"buffer", "width", "height", "format", "stride", NULL};
    PyObject *buf;
    int width, height, format, stride = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oiii|i", kwlist, &buf, &width, &height, &format, &stride)) {
        return NULL;
    }
    if (stride < 0) {
        stride = width;
    }
    GfxFrameBufferObject *o = (GfxFrameBufferObject *)type->tp_alloc(type, 0);
    if (!o) {
        return NULL;
    }
    o->buf_obj = NULL;
    if (framebuffer_init_from_args(buf, width, height, format, stride, o) < 0) {
        Py_DECREF(o);
        return NULL;
    }
    return (PyObject *)o;
}

static void framebuffer_dealloc(GfxFrameBufferObject *self) {
    Py_XDECREF(self->buf_obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int framebuffer_init(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)self;
    (void)args;
    (void)kwds;
    return 0;
}

static PyObject *framebuffer_fill(GfxFrameBufferObject *self, PyObject *args) {
    int col;
    if (!PyArg_ParseTuple(args, "i", &col)) {
        return NULL;
    }
    formats[self->format].fill_rect(self, 0, 0, self->width, self->height, (uint32_t)col);
    Py_RETURN_NONE;
}

static PyObject *framebuffer_fill_rect(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, w, h, col;
    if (!PyArg_ParseTuple(args, "iiiii", &x, &y, &w, &h, &col)) {
        return NULL;
    }
    fill_rect(self, x, y, w, h, (uint32_t)col);
    Py_RETURN_NONE;
}

static PyObject *framebuffer_pixel(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, col = 0;
    if (!PyArg_ParseTuple(args, "ii|i", &x, &y, &col)) {
        return NULL;
    }
    if (0 <= x && x < self->width && 0 <= y && y < self->height) {
        if (PyTuple_GET_SIZE(args) < 3) {
            return PyLong_FromUnsignedLong(getpixel(self, (unsigned int)x, (unsigned int)y));
        }
        setpixel(self, (unsigned int)x, (unsigned int)y, (uint32_t)col);
    }
    Py_RETURN_NONE;
}

static PyObject *framebuffer_hline(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, w, col;
    if (!PyArg_ParseTuple(args, "iiii", &x, &y, &w, &col)) {
        return NULL;
    }
    fill_rect(self, x, y, w, 1, (uint32_t)col);
    Py_RETURN_NONE;
}

static PyObject *framebuffer_vline(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, h, col;
    if (!PyArg_ParseTuple(args, "iiii", &x, &y, &h, &col)) {
        return NULL;
    }
    fill_rect(self, x, y, 1, h, (uint32_t)col);
    Py_RETURN_NONE;
}

static PyObject *framebuffer_rect(GfxFrameBufferObject *self, PyObject *args) {
    int x, y, w, h, col;
    int fill = 0;
    if (!PyArg_ParseTuple(args, "iiiii|p", &x, &y, &w, &h, &col, &fill)) {
        return NULL;
    }
    if (fill) {
        fill_rect(self, x, y, w, h, (uint32_t)col);
    } else {
        fill_rect(self, x, y, w, 1, (uint32_t)col);
        fill_rect(self, x, y + h - 1, w, 1, (uint32_t)col);
        fill_rect(self, x, y, 1, h, (uint32_t)col);
        fill_rect(self, x + w - 1, y, 1, h, (uint32_t)col);
    }
    Py_RETURN_NONE;
}

static void line(const GfxFrameBufferObject *fb, int x1, int y1, int x2, int y2, int col) {
    int dx = x2 - x1;
    int sx;
    if (dx > 0) {
        sx = 1;
    } else {
        dx = -dx;
        sx = -1;
    }
    int dy = y2 - y1;
    int sy;
    if (dy > 0) {
        sy = 1;
    } else {
        dy = -dy;
        sy = -1;
    }
    bool steep;
    if (dy > dx) {
        int temp = x1;
        x1 = y1;
        y1 = temp;
        temp = dx;
        dx = dy;
        dy = temp;
        temp = sx;
        sx = sy;
        sy = temp;
        steep = true;
    } else {
        steep = false;
    }
    int e = 2 * dy - dx;
    for (int i = 0; i < dx; ++i) {
        if (steep) {
            if (0 <= y1 && y1 < fb->width && 0 <= x1 && x1 < fb->height) {
                setpixel(fb, (unsigned int)y1, (unsigned int)x1, (uint32_t)col);
            }
        } else {
            if (0 <= x1 && x1 < fb->width && 0 <= y1 && y1 < fb->height) {
                setpixel(fb, (unsigned int)x1, (unsigned int)y1, (uint32_t)col);
            }
        }
        while (e >= 0) {
            y1 += sy;
            e -= 2 * dx;
        }
        x1 += sx;
        e += 2 * dy;
    }
    setpixel_checked(fb, x2, y2, col, 1);
}

static PyObject *framebuffer_line(GfxFrameBufferObject *self, PyObject *args) {
    int x1, y1, x2, y2, col;
    if (!PyArg_ParseTuple(args, "iiiii", &x1, &y1, &x2, &y2, &col)) {
        return NULL;
    }
    line(self, x1, y1, x2, y2, col);
    Py_RETURN_NONE;
}

#define ELLIPSE_MASK_FILL (0x10)
#define ELLIPSE_MASK_ALL (0x0f)
#define ELLIPSE_MASK_Q1 (0x01)
#define ELLIPSE_MASK_Q2 (0x02)
#define ELLIPSE_MASK_Q3 (0x04)
#define ELLIPSE_MASK_Q4 (0x08)

static void draw_ellipse_points(const GfxFrameBufferObject *fb, int cx, int cy, int x, int y, int col, int mask) {
    if (mask & ELLIPSE_MASK_FILL) {
        if (mask & ELLIPSE_MASK_Q1) {
            fill_rect(fb, cx, cy - y, x + 1, 1, (uint32_t)col);
        }
        if (mask & ELLIPSE_MASK_Q2) {
            fill_rect(fb, cx - x, cy - y, x + 1, 1, (uint32_t)col);
        }
        if (mask & ELLIPSE_MASK_Q3) {
            fill_rect(fb, cx - x, cy + y, x + 1, 1, (uint32_t)col);
        }
        if (mask & ELLIPSE_MASK_Q4) {
            fill_rect(fb, cx, cy + y, x + 1, 1, (uint32_t)col);
        }
    } else {
        setpixel_checked(fb, cx + x, cy - y, col, mask & ELLIPSE_MASK_Q1);
        setpixel_checked(fb, cx - x, cy - y, col, mask & ELLIPSE_MASK_Q2);
        setpixel_checked(fb, cx - x, cy + y, col, mask & ELLIPSE_MASK_Q3);
        setpixel_checked(fb, cx + x, cy + y, col, mask & ELLIPSE_MASK_Q4);
    }
}

static PyObject *framebuffer_ellipse(GfxFrameBufferObject *self, PyObject *args) {
    int cx, cy, xradius, yradius, col;
    int fill = 0;
    int mask_part = ELLIPSE_MASK_ALL;
    if (!PyArg_ParseTuple(args, "iiiii|pi", &cx, &cy, &xradius, &yradius, &col, &fill, &mask_part)) {
        return NULL;
    }
    int mask = fill ? ELLIPSE_MASK_FILL : 0;
    mask |= mask_part & ELLIPSE_MASK_ALL;
    if (xradius == 0 && yradius == 0) {
        setpixel_checked(self, cx, cy, col, mask & ELLIPSE_MASK_ALL);
        Py_RETURN_NONE;
    }
    int two_asquare = 2 * xradius * xradius;
    int two_bsquare = 2 * yradius * yradius;
    int x = xradius;
    int y = 0;
    int xchange = yradius * yradius * (1 - 2 * xradius);
    int ychange = xradius * xradius;
    int ellipse_error = 0;
    int stoppingx = two_bsquare * xradius;
    int stoppingy = 0;
    while (stoppingx >= stoppingy) {
        draw_ellipse_points(self, cx, cy, x, y, col, mask);
        y += 1;
        stoppingy += two_asquare;
        ellipse_error += ychange;
        ychange += two_asquare;
        if ((2 * ellipse_error + xchange) > 0) {
            x -= 1;
            stoppingx -= two_bsquare;
            ellipse_error += xchange;
            xchange += two_bsquare;
        }
    }
    x = 0;
    y = yradius;
    xchange = yradius * yradius;
    ychange = xradius * xradius * (1 - 2 * yradius);
    ellipse_error = 0;
    stoppingx = 0;
    stoppingy = two_asquare * yradius;
    while (stoppingx <= stoppingy) {
        draw_ellipse_points(self, cx, cy, x, y, col, mask);
        x += 1;
        stoppingx += two_bsquare;
        ellipse_error += xchange;
        xchange += two_bsquare;
        if ((2 * ellipse_error + ychange) > 0) {
            y -= 1;
            stoppingy -= two_asquare;
            ellipse_error += ychange;
            ychange += two_asquare;
        }
    }
    Py_RETURN_NONE;
}

static int poly_int_from_buffer(Py_buffer *view, size_t index, int *out) {
    const char *fmt = view->format;
    if (!fmt || !fmt[0]) {
        PyErr_SetString(PyExc_TypeError, "buffer format required");
        return -1;
    }
    size_t itemsize = (size_t)view->itemsize;
    size_t offset = index * itemsize;
    if (offset + itemsize > (size_t)view->len) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return -1;
    }
    const void *ptr = (const char *)view->buf + offset;
    switch (fmt[0]) {
        case 'b':
            *out = *(const signed char *)ptr;
            return 0;
        case 'B':
            *out = *(const unsigned char *)ptr;
            return 0;
        case 'h':
            *out = *(const short *)ptr;
            return 0;
        case 'H':
            *out = *(const unsigned short *)ptr;
            return 0;
        case 'i':
            *out = *(const int *)ptr;
            return 0;
        case 'I':
            *out = (int)*(const unsigned int *)ptr;
            return 0;
        case 'l':
            *out = (int)*(const long *)ptr;
            return 0;
        case 'L':
            *out = (int)*(const unsigned long *)ptr;
            return 0;
        default:
            PyErr_Format(PyExc_TypeError, "unsupported buffer format: %s", fmt);
            return -1;
    }
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
    int n_poly = (int)(view.len / (view.itemsize * 2));
    if (n_poly == 0) {
        PyBuffer_Release(&view);
        Py_RETURN_NONE;
    }
    if (fill) {
        int y_min = INT_MAX;
        int y_max = INT_MIN;
        for (int i = 0; i < n_poly; i++) {
            int py;
            if (poly_int_from_buffer(&view, (size_t)i * 2 + 1, &py) < 0) {
                PyBuffer_Release(&view);
                return NULL;
            }
            y_min = MIN(y_min, py);
            y_max = MAX(y_max, py);
        }
        for (int row = y_min; row <= y_max; row++) {
            int nodes[256];
            if (n_poly > 256) {
                PyBuffer_Release(&view);
                PyErr_SetString(PyExc_ValueError, "polygon too large");
                return NULL;
            }
            int n_nodes = 0;
            int px1, py1;
            if (poly_int_from_buffer(&view, 0, &px1) < 0 || poly_int_from_buffer(&view, 1, &py1) < 0) {
                PyBuffer_Release(&view);
                return NULL;
            }
            int i = n_poly * 2 - 1;
            do {
                int py2, px2;
                if (poly_int_from_buffer(&view, (size_t)i, &py2) < 0) {
                    PyBuffer_Release(&view);
                    return NULL;
                }
                i--;
                if (poly_int_from_buffer(&view, (size_t)i, &px2) < 0) {
                    PyBuffer_Release(&view);
                    return NULL;
                }
                i--;
                if (py1 != py2 && ((py1 > row && py2 <= row) || (py1 <= row && py2 > row))) {
                    int node = (32 * px1 + 32 * (px2 - px1) * (row - py1) / (py2 - py1) + 16) / 32;
                    nodes[n_nodes++] = node;
                } else if (row == MAX(py1, py2)) {
                    if (py1 < py2) {
                        setpixel_checked(self, x + px2, y + py2, col, 1);
                    } else if (py2 < py1) {
                        setpixel_checked(self, x + px1, y + py1, col, 1);
                    } else {
                        line(self, x + px1, y + py1, x + px2, y + py2, col);
                    }
                }
                px1 = px2;
                py1 = py2;
            } while (i >= 0);
            if (!n_nodes) {
                continue;
            }
            i = 0;
            while (i < n_nodes - 1) {
                if (nodes[i] > nodes[i + 1]) {
                    int swap = nodes[i];
                    nodes[i] = nodes[i + 1];
                    nodes[i + 1] = swap;
                    if (i) {
                        i--;
                    }
                } else {
                    i++;
                }
            }
            for (i = 0; i < n_nodes; i += 2) {
                fill_rect(self, x + nodes[i], y + row, (nodes[i + 1] - nodes[i]) + 1, 1, (uint32_t)col);
            }
        }
    } else {
        int px1, py1;
        if (poly_int_from_buffer(&view, 0, &px1) < 0 || poly_int_from_buffer(&view, 1, &py1) < 0) {
            PyBuffer_Release(&view);
            return NULL;
        }
        int i = n_poly * 2 - 1;
        do {
            int py2, px2;
            if (poly_int_from_buffer(&view, (size_t)i, &py2) < 0) {
                PyBuffer_Release(&view);
                return NULL;
            }
            i--;
            if (poly_int_from_buffer(&view, (size_t)i, &px2) < 0) {
                PyBuffer_Release(&view);
                return NULL;
            }
            i--;
            line(self, x + px1, y + py1, x + px2, y + py2, col);
            px1 = px2;
            py1 = py2;
        } while (i >= 0);
    }
    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

static PyObject *framebuffer_blit(GfxFrameBufferObject *self, PyObject *args) {
    PyObject *source_obj;
    int x, y;
    int key = -1;
    PyObject *palette_obj = NULL;
    if (!PyArg_ParseTuple(args, "Oii|iO", &source_obj, &x, &y, &key, &palette_obj)) {
        return NULL;
    }
    GfxFrameBufferObject source;
    source.buf_obj = NULL;
    if (get_readonly_framebuffer(source_obj, &source) < 0) {
        return NULL;
    }
    GfxFrameBufferObject palette;
    palette.buf = NULL;
    if (palette_obj != NULL && palette_obj != Py_None) {
        if (get_readonly_framebuffer(palette_obj, &palette) < 0) {
            return NULL;
        }
    }
    if ((x >= self->width) || (y >= self->height) || (-x >= source.width) || (-y >= source.height)) {
        Py_RETURN_NONE;
    }
    int x0 = MAX(0, x);
    int y0 = MAX(0, y);
    int x1 = MAX(0, -x);
    int y1 = MAX(0, -y);
    int x0end = MIN(self->width, x + source.width);
    int y0end = MIN(self->height, y + source.height);
    for (; y0 < y0end; ++y0) {
        int cx1 = x1;
        for (int cx0 = x0; cx0 < x0end; ++cx0) {
            uint32_t col = getpixel(&source, (unsigned int)cx1, (unsigned int)y1);
            if (palette.buf) {
                col = getpixel(&palette, col, 0);
            }
            if (col != (uint32_t)key) {
                setpixel(self, (unsigned int)cx0, (unsigned int)y0, col);
            }
            ++cx1;
        }
        ++y1;
    }
    Py_RETURN_NONE;
}

static PyObject *framebuffer_scroll(GfxFrameBufferObject *self, PyObject *args) {
    int xstep, ystep;
    if (!PyArg_ParseTuple(args, "ii", &xstep, &ystep)) {
        return NULL;
    }
    unsigned int sx, yy, xend, yend;
    int dx, dy;
    if (xstep < 0) {
        if (-xstep >= self->width) {
            Py_RETURN_NONE;
        }
        sx = 0;
        xend = self->width + (unsigned int)xstep;
        dx = 1;
    } else {
        if (xstep >= self->width) {
            Py_RETURN_NONE;
        }
        sx = self->width - 1;
        xend = (unsigned int)xstep - 1u;
        dx = -1;
    }
    if (ystep < 0) {
        if (-ystep >= self->height) {
            Py_RETURN_NONE;
        }
        yy = 0;
        yend = self->height + (unsigned int)ystep;
        dy = 1;
    } else {
        if (ystep >= self->height) {
            Py_RETURN_NONE;
        }
        yy = self->height - 1;
        yend = (unsigned int)ystep - 1u;
        dy = -1;
    }
    for (; yy != yend; yy = (unsigned int)((int)yy + dy)) {
        for (unsigned int xx = sx; xx != xend; xx = (unsigned int)((int)xx + dx)) {
            setpixel(self, xx, yy, getpixel(self, (unsigned int)((int)xx - xstep), (unsigned int)((int)yy - ystep)));
        }
    }
    Py_RETURN_NONE;
}

static PyObject *framebuffer_text(GfxFrameBufferObject *self, PyObject *args) {
    const char *str;
    int x0, y0;
    int col = 1;
    if (!PyArg_ParseTuple(args, "sii|i", &str, &x0, &y0, &col)) {
        return NULL;
    }
    for (; *str; ++str) {
        int chr = (unsigned char)*str;
        if (chr < 32 || chr > 127) {
            chr = 127;
        }
        const uint8_t *chr_data = &font_petme128_8x8[(chr - 32) * 8];
        for (int j = 0; j < 8; j++, x0++) {
            if (0 <= x0 && x0 < self->width) {
                unsigned int vline_data = chr_data[j];
                for (int yy = y0; vline_data; vline_data >>= 1, yy++) {
                    if (vline_data & 1) {
                        if (0 <= yy && yy < self->height) {
                            setpixel(self, (unsigned int)x0, (unsigned int)yy, (uint32_t)col);
                        }
                    }
                }
            }
        }
    }
    Py_RETURN_NONE;
}

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
    {"blit", (PyCFunction)framebuffer_blit, METH_VARARGS, NULL},
    {"scroll", (PyCFunction)framebuffer_scroll, METH_VARARGS, NULL},
    {"text", (PyCFunction)framebuffer_text, METH_VARARGS, NULL},
    {NULL},
};

static PyTypeObject GfxFrameBufferType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "graphics_native.FrameBuffer",
    .tp_basicsize = sizeof(GfxFrameBufferObject),
    .tp_dealloc = (destructor)framebuffer_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "Frame buffer compatible with MicroPython framebuf API",
    .tp_methods = framebuffer_methods,
    .tp_init = (initproc)framebuffer_init,
    .tp_new = framebuffer_new,
};

/* ------------------------------------------------------------------------- */
/* FrameBuffer1 factory + module                                             */
/* ------------------------------------------------------------------------- */

static PyObject *framebuffer1(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *buf;
    int width, height, stride = -1;
    if (!PyArg_ParseTuple(args, "Oii|i", &buf, &width, &height, &stride)) {
        return NULL;
    }
    if (stride < 0) {
        stride = width;
    }
    return PyObject_CallFunction(
        (PyObject *)&GfxFrameBufferType, "Oiiii", buf, width, height, GFX_MVLSB, stride);
}

static PyMethodDef module_methods[] = {
    {"FrameBuffer1", framebuffer1, METH_VARARGS, "Legacy MONO_VLSB FrameBuffer factory"},
    {NULL},
};

static struct PyModuleDef graphics_native_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "graphics_native",
    .m_doc = "Native graphics framebuf and Area for CPython",
    .m_size = -1,
    .m_methods = module_methods,
};

PyMODINIT_FUNC PyInit_graphics_native(void) {
    PyObject *m = PyModule_Create(&graphics_native_module);
    if (!m) {
        return NULL;
    }
    if (PyType_Ready(&GfxAreaType) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyType_Ready(&GfxFrameBufferType) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    Py_INCREF(&GfxAreaType);
    Py_INCREF(&GfxFrameBufferType);
    if (PyModule_AddObject(m, "Area", (PyObject *)&GfxAreaType) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddObject(m, "FrameBuffer", (PyObject *)&GfxFrameBufferType) < 0) {
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
#undef ADD_INT
    return m;
}
