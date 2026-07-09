/*
 * Image file I/O helpers.
 * SPDX-License-Identifier: MIT
 */

#include "gfx_files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx_bmp565.h"
#include "gfx_core.h"
#include "gfx_framebuffer.h"

void gfx_image_fb_free(gfx_image_fb_t *img) {
    if (img->owns_buffer && img->buffer) {
        free(img->buffer);
    }
    img->buffer = NULL;
    img->buffer_len = 0;
    img->owns_buffer = 0;
}

static int skip_comments(const uint8_t **data, size_t *len) {
    while (*len > 0 && **data == '#') {
        const uint8_t *nl = memchr(*data, '\n', *len);
        if (!nl) {
            return -1;
        }
        size_t skip = (size_t)(nl - *data) + 1;
        *data += skip;
        *len -= skip;
    }
    return 0;
}

static int parse_dims_line(const uint8_t **data, size_t *len, int *width, int *height) {
    if (skip_comments(data, len) < 0) {
        return -1;
    }
    char line[128];
    size_t i = 0;
    while (*len > 0 && **data != '\n' && i + 1 < sizeof(line)) {
        line[i++] = (char)**data;
        (*data)++;
        (*len)--;
    }
    if (*len == 0 || **data != '\n') {
        return -1;
    }
    (*data)++;
    (*len)--;
    line[i] = '\0';
    if (sscanf(line, "%d %d", width, height) != 2) {
        return -1;
    }
    return 0;
}

int gfx_files_pbm_to_framebuffer(const char *path, gfx_image_fb_t *out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    char sig[3];
    if (fread(sig, 1, 3, f) != 3 || sig[0] != 'P' || sig[1] != '4' || sig[2] != '\n') {
        fclose(f);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    fseek(f, 3, SEEK_SET);
    uint8_t *all = (uint8_t *)malloc((size_t)flen - 3);
    if (!all) {
        fclose(f);
        return -1;
    }
    if (fread(all, 1, (size_t)flen - 3, f) != (size_t)flen - 3) {
        free(all);
        fclose(f);
        return -1;
    }
    fclose(f);
    const uint8_t *data = all;
    size_t len = (size_t)flen - 3;
    int width, height;
    if (parse_dims_line(&data, &len, &width, &height) < 0) {
        free(all);
        return -1;
    }
    size_t buf_len = (size_t)((width + 7) / 8) * (size_t)height;
    uint8_t *buf = (uint8_t *)malloc(buf_len);
    if (!buf) {
        free(all);
        return -1;
    }
    if (len < buf_len) {
        free(buf);
        free(all);
        return -1;
    }
    memcpy(buf, data, buf_len);
    free(all);
    out->buffer = buf;
    out->buffer_len = buf_len;
    out->width = width;
    out->height = height;
    out->format = GFX_MHLSB;
    out->owns_buffer = 1;
    return 0;
}

int gfx_files_pgm_to_framebuffer(const char *path, gfx_image_fb_t *out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    char sig[3];
    if (fread(sig, 1, 3, f) != 3 || sig[0] != 'P' || sig[1] != '5' || sig[2] != '\n') {
        fclose(f);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    fseek(f, 3, SEEK_SET);
    uint8_t *all = (uint8_t *)malloc((size_t)flen - 3);
    if (!all) {
        fclose(f);
        return -1;
    }
    if (fread(all, 1, (size_t)flen - 3, f) != (size_t)flen - 3) {
        free(all);
        fclose(f);
        return -1;
    }
    fclose(f);
    const uint8_t *data = all;
    size_t len = (size_t)flen - 3;
    int width, height;
    if (parse_dims_line(&data, &len, &width, &height) < 0) {
        free(all);
        return -1;
    }
    if (skip_comments(&data, &len) < 0) {
        free(all);
        return -1;
    }
    char max_line[32];
    size_t i = 0;
    while (len > 0 && *data != '\n' && i + 1 < sizeof(max_line)) {
        max_line[i++] = (char)*data;
        data++;
        len--;
    }
    if (len == 0 || *data != '\n') {
        free(all);
        return -1;
    }
    data++;
    len--;
    max_line[i] = '\0';
    int max_value = atoi(max_line);
    int format;
    size_t buf_len;
    if (max_value == 3) {
        format = GFX_GS2_HMSB;
        buf_len = (size_t)((width + 3) / 4) * (size_t)height;
    } else if (max_value == 15) {
        format = GFX_GS4_HMSB;
        buf_len = (size_t)((width + 1) / 2) * (size_t)height;
    } else if (max_value == 255) {
        format = GFX_GS8;
        buf_len = (size_t)width * (size_t)height;
    } else {
        free(all);
        return -1;
    }
    uint8_t *buf = (uint8_t *)malloc(buf_len);
    if (!buf) {
        free(all);
        return -1;
    }
    if (len < buf_len) {
        free(buf);
        free(all);
        return -1;
    }
    memcpy(buf, data, buf_len);
    free(all);
    out->buffer = buf;
    out->buffer_len = buf_len;
    out->width = width;
    out->height = height;
    out->format = format;
    out->owns_buffer = 1;
    return 0;
}

int gfx_files_bmp_to_framebuffer(const char *path, gfx_image_fb_t *out) {
    gfx_bmp565_t bmp;
    if (gfx_bmp565_load_from_file(path, &bmp) < 0) {
        return -1;
    }
    out->buffer = bmp.buffer;
    out->buffer_len = bmp.buffer_len;
    out->width = bmp.width;
    out->height = bmp.height;
    out->format = GFX_RGB565;
    out->owns_buffer = 1;
    return 0;
}

int gfx_files_load_image(const char *path, gfx_image_fb_t *out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    char header[2];
    if (fread(header, 1, 2, f) != 2) {
        fclose(f);
        return -1;
    }
    fclose(f);
    if (header[0] == 'P' && header[1] == '4') {
        return gfx_files_pbm_to_framebuffer(path, out);
    }
    if (header[0] == 'P' && header[1] == '5') {
        return gfx_files_pgm_to_framebuffer(path, out);
    }
    if (header[0] == 'B' && header[1] == 'M') {
        return gfx_files_bmp_to_framebuffer(path, out);
    }
    return -1;
}

static const char *ext_for_format(int format) {
    switch (format) {
        case GFX_MHLSB:
            return "pbm";
        case GFX_GS2_HMSB:
        case GFX_GS4_HMSB:
        case GFX_GS8:
            return "pgm";
        case GFX_RGB565:
            return "bmp";
        default:
            return NULL;
    }
}

int gfx_files_save_image(const gfx_fb_t *fb, const char *path, char *out_path, size_t out_path_len) {
    const char *ext = ext_for_format(fb->format);
    if (!ext) {
        return -1;
    }
    const char *dot = strrchr(path, '.');
    if (!dot || strcmp(dot + 1, ext) != 0) {
        snprintf(out_path, out_path_len, "%s.%s", path, ext);
    } else {
        strncpy(out_path, path, out_path_len - 1);
        out_path[out_path_len - 1] = '\0';
    }
    if (fb->format == GFX_MHLSB) {
        FILE *f = fopen(out_path, "wb");
        if (!f) {
            return -1;
        }
        fprintf(f, "P4\n%d %d\n", fb->width, fb->height);
        size_t len = (size_t)((fb->width + 7) / 8) * (size_t)fb->height;
        fwrite(fb->buf, 1, len, f);
        fclose(f);
        return 0;
    }
    if (fb->format == GFX_GS2_HMSB || fb->format == GFX_GS4_HMSB || fb->format == GFX_GS8) {
        FILE *f = fopen(out_path, "wb");
        if (!f) {
            return -1;
        }
        fprintf(f, "P5\n%d %d\n", fb->width, fb->height);
        if (fb->format == GFX_GS2_HMSB) {
            fprintf(f, "3\n");
        } else if (fb->format == GFX_GS4_HMSB) {
            fprintf(f, "15\n");
        } else {
            fprintf(f, "255\n");
        }
        size_t len;
        if (fb->format == GFX_GS2_HMSB) {
            len = (size_t)((fb->width + 3) / 4) * (size_t)fb->height;
        } else if (fb->format == GFX_GS4_HMSB) {
            len = (size_t)((fb->width + 1) / 2) * (size_t)fb->height;
        } else {
            len = (size_t)fb->width * (size_t)fb->height;
        }
        fwrite(fb->buf, 1, len, f);
        fclose(f);
        return 0;
    }
    if (fb->format == GFX_RGB565) {
        gfx_bmp565_t bmp;
        gfx_bmp565_init_from_buffer(&bmp, (const uint8_t *)fb->buf,
            (size_t)fb->width * (size_t)fb->height * 2, fb->width, fb->height);
        return gfx_bmp565_save(&bmp, out_path);
    }
    return -1;
}
