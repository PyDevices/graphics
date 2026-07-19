/*
 * Image codec helpers.
 *
 * These functions are pure and I/O-free: they decode/encode image byte streams
 * that live entirely in memory. The actual reading and writing of files is done
 * by the caller (the MicroPython binding uses the VFS; a host build may use C
 * stdio) so that the same codec works on every port, including bare-metal
 * targets that have no host filesystem.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_FILES_H
#define GFX_FILES_H

#include <stddef.h>
#include <stdint.h>

#include "gfx_core.h"
#include "gfx_framebuffer.h"

/* Decoded image + its backing buffer. Used by the host stdio helpers and the
 * CPython binding, which own a malloc'd pixel buffer. */
typedef struct gfx_image_fb {
    uint8_t *buffer;
    size_t buffer_len;
    int width;
    int height;
    int format;
    int owns_buffer;
} gfx_image_fb_t;

/* Result of probing an encoded image (PBM/PGM/BMP) held in memory. */
typedef struct gfx_image_info {
    int width;
    int height;
    int format;          /* one of the GFX_* framebuffer formats */
    size_t buffer_len;   /* decoded framebuffer size, in bytes */
    size_t src_offset;   /* offset of pixel data within the encoded bytes */
} gfx_image_info_t;

/* Inspect encoded image bytes (P4 PBM, P5 PGM, or BM BMP565) and fill `info`
 * without decoding pixels. Returns 0 on success, -1 if unrecognized/invalid. */
int gfx_image_probe(const uint8_t *data, size_t len, gfx_image_info_t *info);

/* Decode probed image bytes into `dst` (must be at least info->buffer_len).
 * Returns 0 on success, -1 on bounds/format error. */
int gfx_image_decode(const uint8_t *data, size_t len, const gfx_image_info_t *info,
    uint8_t *dst, size_t dst_len);

/* Compute the encoded size (and file extension without the dot) for a
 * framebuffer that would be written with gfx_image_encode(). Returns 0 on
 * success, -1 if the framebuffer format cannot be encoded. */
int gfx_image_encoded_size(const gfx_fb_t *fb, size_t *out_len, const char **out_ext);

/* Encode a framebuffer into `dst` (must be at least the size reported by
 * gfx_image_encoded_size()). Writes the encoded length to *out_len. Returns 0
 * on success, -1 on error. */
int gfx_image_encode(const gfx_fb_t *fb, uint8_t *dst, size_t dst_len, size_t *out_len);

/* Host C stdio convenience wrappers (desktop / CPython binding). These read and
 * write real files via fopen() and are compiled only where GFX_ENABLE_HOST_STDIO
 * is set. MicroPython builds use the VFS path in the binding layer instead. */
#if GFX_ENABLE_HOST_STDIO
void gfx_image_fb_free(gfx_image_fb_t *img);
int gfx_files_load_image(const char *path, gfx_image_fb_t *out);
int gfx_files_save_image(const gfx_fb_t *fb, const char *path, char *out_path, size_t out_path_len);
int gfx_files_pbm_to_framebuffer(const char *path, gfx_image_fb_t *out);
int gfx_files_pgm_to_framebuffer(const char *path, gfx_image_fb_t *out);
int gfx_files_bmp_to_framebuffer(const char *path, gfx_image_fb_t *out);
#endif

#endif
