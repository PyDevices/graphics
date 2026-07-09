/*
 * RGB565 BMP asset helpers.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_BMP565_H
#define GFX_BMP565_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define GFX_BMP565_BPP 16
#define GFX_BMP565_BYTES_PER_PIXEL 2

typedef struct gfx_bmp565 {
    uint8_t *buffer;
    size_t buffer_len;
    int width;
    int height;
    int owns_buffer;
    int streamed;
    int mirrored;
    unsigned int data_offset;
    FILE *file;
} gfx_bmp565_t;

int gfx_bmp565_read_header_from_file(const char *path, int *width, int *height, unsigned int *data_offset);
int gfx_bmp565_open_stream(const char *path, gfx_bmp565_t *out);
int gfx_bmp565_load_from_file(const char *path, gfx_bmp565_t *out);
int gfx_bmp565_init_from_buffer(gfx_bmp565_t *out, const uint8_t *buf, size_t len, int width, int height);
void gfx_bmp565_deinit(gfx_bmp565_t *bmp);
int gfx_bmp565_save(const gfx_bmp565_t *bmp, const char *path);
int gfx_bmp565_read_bytes(const gfx_bmp565_t *bmp, int start, int stop, uint8_t *out, size_t out_cap, size_t *out_len);
int gfx_bmp565_read_region(const gfx_bmp565_t *bmp, int x0, int x1, int y0, int y1, uint8_t *out, size_t out_cap, size_t *out_len);

#endif
