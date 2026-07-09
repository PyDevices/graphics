/*
 * RGB565 BMP asset helpers.
 * SPDX-License-Identifier: MIT
 */

#include "gfx_bmp565.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_u16_le(FILE *f, uint16_t *out) {
    uint8_t b[2];
    if (fread(b, 1, 2, f) != 2) {
        return -1;
    }
    *out = (uint16_t)(b[0] | (b[1] << 8));
    return 0;
}

static int read_u32_le(FILE *f, uint32_t *out) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) {
        return -1;
    }
    *out = (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
    return 0;
}

static int write_u16_le(FILE *f, uint16_t val) {
    uint8_t b[2] = { (uint8_t)(val & 0xFF), (uint8_t)((val >> 8) & 0xFF) };
    return fwrite(b, 1, 2, f) == 2 ? 0 : -1;
}

static int write_u32_le(FILE *f, uint32_t val) {
    uint8_t b[4] = {
        (uint8_t)(val & 0xFF),
        (uint8_t)((val >> 8) & 0xFF),
        (uint8_t)((val >> 16) & 0xFF),
        (uint8_t)((val >> 24) & 0xFF),
    };
    return fwrite(b, 1, 4, f) == 4 ? 0 : -1;
}

int gfx_bmp565_read_header_from_file(const char *path, int *width, int *height, unsigned int *data_offset) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    char sig[2];
    if (fread(sig, 1, 2, f) != 2 || sig[0] != 'B' || sig[1] != 'M') {
        fclose(f);
        return -1;
    }
    if (fseek(f, 10, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }
    uint32_t off;
    if (read_u32_le(f, &off) < 0) {
        fclose(f);
        return -1;
    }
    if (fseek(f, 14, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }
    uint32_t header_size, w, h;
    uint16_t planes, bpp;
    if (read_u32_le(f, &header_size) < 0 || read_u32_le(f, &w) < 0 || read_u32_le(f, &h) < 0
        || read_u16_le(f, &planes) < 0 || read_u16_le(f, &bpp) < 0) {
        fclose(f);
        return -1;
    }
    fclose(f);
    if (planes != 1 || bpp != GFX_BMP565_BPP) {
        return -1;
    }
    *width = (int)w;
    *height = (int)h;
    *data_offset = off;
    return 0;
}

static int load_rows(FILE *f, int width, int height, unsigned int data_offset, uint8_t *buffer) {
    size_t row_bytes = (size_t)width * GFX_BMP565_BYTES_PER_PIXEL;
    for (int row = 0; row < height; row++) {
        if (fseek(f, (long)data_offset + (long)(height - row - 1) * (long)row_bytes, SEEK_SET) != 0) {
            return -1;
        }
        if (fread(buffer + row * row_bytes, 1, row_bytes, f) != row_bytes) {
            return -1;
        }
    }
    return 0;
}

int gfx_bmp565_load_from_file(const char *path, gfx_bmp565_t *out) {
    int width, height;
    unsigned int data_offset;
    if (gfx_bmp565_read_header_from_file(path, &width, &height, &data_offset) < 0) {
        return -1;
    }
    size_t len = (size_t)width * (size_t)height * GFX_BMP565_BYTES_PER_PIXEL;
    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf) {
        return -1;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        free(buf);
        return -1;
    }
    if (load_rows(f, width, height, data_offset, buf) < 0) {
        fclose(f);
        free(buf);
        return -1;
    }
    fclose(f);
    out->buffer = buf;
    out->buffer_len = len;
    out->width = width;
    out->height = height;
    out->owns_buffer = 1;
    return 0;
}

int gfx_bmp565_init_from_buffer(gfx_bmp565_t *out, const uint8_t *buf, size_t len, int width, int height) {
    out->buffer = (uint8_t *)buf;
    out->buffer_len = len;
    out->width = width;
    out->height = height;
    out->owns_buffer = 0;
    return 0;
}

void gfx_bmp565_deinit(gfx_bmp565_t *bmp) {
    if (bmp->owns_buffer && bmp->buffer) {
        free(bmp->buffer);
    }
    bmp->buffer = NULL;
    bmp->buffer_len = 0;
    bmp->owns_buffer = 0;
}

int gfx_bmp565_save(const gfx_bmp565_t *bmp, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return -1;
    }
    size_t data_size = (size_t)bmp->width * (size_t)bmp->height * GFX_BMP565_BYTES_PER_PIXEL;
    if (fwrite("BM", 1, 2, f) != 2) {
        fclose(f);
        return -1;
    }
    if (write_u32_le(f, (uint32_t)(14 + 40 + data_size)) < 0
        || fwrite("\x00\x00\x00\x00", 1, 4, f) != 4
        || write_u32_le(f, 14 + 40) < 0
        || write_u32_le(f, 40) < 0
        || write_u32_le(f, (uint32_t)bmp->width) < 0
        || write_u32_le(f, (uint32_t)bmp->height) < 0
        || write_u16_le(f, 1) < 0
        || write_u16_le(f, GFX_BMP565_BPP) < 0
        || fwrite("\x00\x00\x00\x00", 1, 4, f) != 4
        || write_u32_le(f, (uint32_t)data_size) < 0
        || fwrite("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 16, f) != 16) {
        fclose(f);
        return -1;
    }
    size_t row_bytes = (size_t)bmp->width * GFX_BMP565_BYTES_PER_PIXEL;
    for (int row = bmp->height - 1; row >= 0; row--) {
        if (fwrite(bmp->buffer + (size_t)row * row_bytes, 1, row_bytes, f) != row_bytes) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}
