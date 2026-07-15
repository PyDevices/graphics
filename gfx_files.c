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

/* Read one PNM header line from f, skipping '#' comments. Line excludes newline. */
static int read_header_line(FILE *f, char *line, size_t line_cap) {
    for (;;) {
        size_t i = 0;
        int c;
        for (;;) {
            c = fgetc(f);
            if (c == EOF) {
                return -1;
            }
            if (c == '\n') {
                break;
            }
            if (i + 1 < line_cap) {
                line[i++] = (char)c;
            }
        }
        line[i] = '\0';
        if (i > 0 && line[0] == '#') {
            continue;
        }
        return 0;
    }
}

static int read_exact(FILE *f, uint8_t *buf, size_t n) {
    size_t got = 0;
    while (got < n) {
        size_t nread = fread(buf + got, 1, n - got, f);
        if (nread == 0) {
            return -1;
        }
        got += nread;
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
    char line[128];
    if (read_header_line(f, line, sizeof(line)) < 0) {
        fclose(f);
        return -1;
    }
    int width, height;
    if (sscanf(line, "%d %d", &width, &height) != 2 || width <= 0 || height <= 0) {
        fclose(f);
        return -1;
    }
    size_t buf_len = (size_t)((width + 7) / 8) * (size_t)height;
    uint8_t *buf = (uint8_t *)malloc(buf_len);
    if (!buf) {
        fclose(f);
        return -1;
    }
    if (read_exact(f, buf, buf_len) < 0) {
        free(buf);
        fclose(f);
        return -1;
    }
    fclose(f);
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
    char line[128];
    if (read_header_line(f, line, sizeof(line)) < 0) {
        fclose(f);
        return -1;
    }
    int width, height;
    if (sscanf(line, "%d %d", &width, &height) != 2 || width <= 0 || height <= 0) {
        fclose(f);
        return -1;
    }
    if (read_header_line(f, line, sizeof(line)) < 0) {
        fclose(f);
        return -1;
    }
    int max_value = atoi(line);
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
        fclose(f);
        return -1;
    }
    uint8_t *buf = (uint8_t *)malloc(buf_len);
    if (!buf) {
        fclose(f);
        return -1;
    }
    if (read_exact(f, buf, buf_len) < 0) {
        free(buf);
        fclose(f);
        return -1;
    }
    fclose(f);
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
