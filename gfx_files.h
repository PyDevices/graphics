/*
 * Image file I/O helpers.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_FILES_H
#define GFX_FILES_H

#include <stddef.h>
#include <stdint.h>

#include "gfx_framebuffer.h"

typedef struct gfx_image_fb {
    uint8_t *buffer;
    size_t buffer_len;
    int width;
    int height;
    int format;
    int owns_buffer;
} gfx_image_fb_t;

void gfx_image_fb_free(gfx_image_fb_t *img);
int gfx_files_load_image(const char *path, gfx_image_fb_t *out);
int gfx_files_save_image(const gfx_fb_t *fb, const char *path, char *out_path, size_t out_path_len);
int gfx_files_pbm_to_framebuffer(const char *path, gfx_image_fb_t *out);
int gfx_files_pgm_to_framebuffer(const char *path, gfx_image_fb_t *out);
int gfx_files_bmp_to_framebuffer(const char *path, gfx_image_fb_t *out);

#endif
