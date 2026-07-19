/*
 * Extended MicroPython bindings header.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_BINDINGS_MP_H
#define GFX_BINDINGS_MP_H

#include "py/obj.h"
#include "gfx_bmp565.h"
#include "gfx_files.h"

/* MicroPython VFS file I/O backend. When enabled, image/font loaders read and
 * write through the same filesystem Python `open()` uses, so they work on any
 * port with a VFS (unix, webassembly, esp32, rp2, ...). Keyed off MICROPY_VFS;
 * define GFX_ENABLE_MP_VFS before including to override. */
#ifndef GFX_ENABLE_MP_VFS
#if defined(MICROPY_VFS) && MICROPY_VFS
#define GFX_ENABLE_MP_VFS 1
#else
#define GFX_ENABLE_MP_VFS 0
#endif
#endif

/* True when at least one file backend (VFS or host stdio) is available. */
#define GFX_MP_HAS_FILE_IO (GFX_ENABLE_MP_VFS || GFX_ENABLE_HOST_STDIO)

/* Image kinds accepted by gfxmp_load_framebuffer(). */
enum {
    GFXMP_ANY = 0,
    GFXMP_PBM,
    GFXMP_PGM,
    GFXMP_BMP,
};

/* Shared file-I/O helpers (implemented in gfx_module_mp.c). All raise a Python
 * exception on failure rather than returning an error code. */
mp_obj_t gfxmp_slurp(const char *path);              /* whole file -> bytes/bytearray */
void gfxmp_spew(const char *path, mp_obj_t data);    /* write a buffer object to file */
mp_obj_t gfxmp_load_framebuffer(const char *path, int expect_kind); /* -> FrameBuffer */
void gfxmp_save_framebuffer(const gfx_fb_t *fb, const char *path, char *out_path, size_t out_path_len);

/* mp_obj_get_float() is only declared on ports that enable float objects
 * (MICROPY_PY_BUILTINS_FLOAT). The shape APIs take C floats regardless, so on
 * float-less ports fall back to integer conversion: angle-taking primitives
 * still work with integer degrees instead of failing to link/compile. */
#if MICROPY_PY_BUILTINS_FLOAT
#define GFX_OBJ_GET_FLOAT(o) ((float)mp_obj_get_float(o))
#else
#define GFX_OBJ_GET_FLOAT(o) ((float)mp_obj_get_int(o))
#endif

typedef struct _mp_obj_framebuf_t {
    mp_obj_base_t base;
    mp_obj_t buf_obj;
    gfx_fb_t fb;
    gfx_canvas_t canvas;
} mp_obj_framebuf_t;

mp_obj_t framebuf_make_new_helper(size_t n_args, const mp_obj_t *args_in, unsigned int buf_flags, mp_obj_framebuf_t *o);

extern const mp_obj_type_t mp_type_framebuf;
extern const mp_obj_type_t mp_type_draw;
extern const mp_obj_type_t mp_type_font;
extern const mp_obj_type_t mp_type_bmp565;

typedef struct _mp_obj_bmp565_t {
    mp_obj_base_t base;
    gfx_bmp565_t bmp;
    mp_obj_t buf_obj;
    mp_obj_t filename_obj;
} mp_obj_bmp565_t;

#endif
