/*
 * Framebuffer pixel format operations.
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
#include <string.h>

#include "gfx_framebuffer.h"

typedef void (*setpixel_t)(const gfx_fb_t *, unsigned int, unsigned int, uint32_t);
typedef uint32_t (*getpixel_t)(const gfx_fb_t *, unsigned int, unsigned int);
typedef void (*fill_rect_t)(const gfx_fb_t *, unsigned int, unsigned int, unsigned int, unsigned int, uint32_t);

typedef struct {
    setpixel_t setpixel;
    getpixel_t getpixel;
    fill_rect_t fill_rect;
} gfx_fb_ops_t;

static void mono_horiz_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col) {
    size_t index = (x + y * fb->stride) >> 3;
    unsigned int offset = fb->format == GFX_MHMSB ? x & 0x07 : 7 - (x & 0x07);
    ((uint8_t *)fb->buf)[index] = (((uint8_t *)fb->buf)[index] & ~(0x01 << offset)) | ((col != 0) << offset);
}

static uint32_t mono_horiz_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y) {
    size_t index = (x + y * fb->stride) >> 3;
    unsigned int offset = fb->format == GFX_MHMSB ? x & 0x07 : 7 - (x & 0x07);
    return (((uint8_t *)fb->buf)[index] >> (offset)) & 0x01;
}

static void mono_horiz_fill_rect(const gfx_fb_t *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
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

static void mvlsb_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col) {
    size_t index = (y >> 3) * fb->stride + x;
    uint8_t offset = y & 0x07;
    ((uint8_t *)fb->buf)[index] = (((uint8_t *)fb->buf)[index] & ~(0x01 << offset)) | ((col != 0) << offset);
}

static uint32_t mvlsb_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y) {
    return (((uint8_t *)fb->buf)[(y >> 3) * fb->stride + x] >> (y & 0x07)) & 0x01;
}

static void mvlsb_fill_rect(const gfx_fb_t *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
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

static void rgb565_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col) {
    ((uint16_t *)fb->buf)[x + y * fb->stride] = (uint16_t)col;
}

static uint32_t rgb565_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y) {
    return ((uint16_t *)fb->buf)[x + y * fb->stride];
}

static void rgb565_fill_rect(const gfx_fb_t *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    uint16_t *b = &((uint16_t *)fb->buf)[x + y * fb->stride];
    while (h--) {
        for (unsigned int ww = w; ww; --ww) {
            *b++ = (uint16_t)col;
        }
        b += fb->stride - w;
    }
}

static void rgb888_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col) {
    size_t index = ((size_t)y * fb->stride + x) * 3;
    uint8_t *p = (uint8_t *)fb->buf + index;
    p[0] = (uint8_t)((col >> 16) & 0xff);
    p[1] = (uint8_t)((col >> 8) & 0xff);
    p[2] = (uint8_t)(col & 0xff);
}

static uint32_t rgb888_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y) {
    size_t index = ((size_t)y * fb->stride + x) * 3;
    const uint8_t *p = (const uint8_t *)fb->buf + index;
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
}

static void rgb888_fill_rect(const gfx_fb_t *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    uint8_t rgb[3] = {
        (uint8_t)((col >> 16) & 0xff),
        (uint8_t)((col >> 8) & 0xff),
        (uint8_t)(col & 0xff),
    };
    for (unsigned int yy = y; yy < y + h; yy++) {
        uint8_t *row = (uint8_t *)fb->buf + ((size_t)yy * fb->stride + x) * 3;
        for (unsigned int xx = 0; xx < w; xx++) {
            row[xx * 3] = rgb[0];
            row[xx * 3 + 1] = rgb[1];
            row[xx * 3 + 2] = rgb[2];
        }
    }
}

static void gs2_hmsb_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col) {
    uint8_t *pixel = &((uint8_t *)fb->buf)[(x + y * fb->stride) >> 2];
    uint8_t shift = (x & 0x3) << 1;
    uint8_t mask = 0x3 << shift;
    uint8_t color = (col & 0x3) << shift;
    *pixel = color | (*pixel & (~mask));
}

static uint32_t gs2_hmsb_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y) {
    uint8_t pixel = ((uint8_t *)fb->buf)[(x + y * fb->stride) >> 2];
    uint8_t shift = (x & 0x3) << 1;
    return (pixel >> shift) & 0x3;
}

static void gs2_hmsb_fill_rect(const gfx_fb_t *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    for (unsigned int xx = x; xx < x + w; xx++) {
        for (unsigned int yy = y; yy < y + h; yy++) {
            gs2_hmsb_setpixel(fb, xx, yy, col);
        }
    }
}

static void gs4_hmsb_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col) {
    uint8_t *pixel = &((uint8_t *)fb->buf)[(x + y * fb->stride) >> 1];
    if (x % 2) {
        *pixel = ((uint8_t)col & 0x0f) | (*pixel & 0xf0);
    } else {
        *pixel = ((uint8_t)col << 4) | (*pixel & 0x0f);
    }
}

static uint32_t gs4_hmsb_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y) {
    if (x % 2) {
        return ((uint8_t *)fb->buf)[(x + y * fb->stride) >> 1] & 0x0f;
    }
    return ((uint8_t *)fb->buf)[(x + y * fb->stride) >> 1] >> 4;
}

static void gs4_hmsb_fill_rect(const gfx_fb_t *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
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

static void gs8_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col) {
    ((uint8_t *)fb->buf)[(x + y * fb->stride)] = (uint8_t)(col & 0xff);
}

static uint32_t gs8_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y) {
    return ((uint8_t *)fb->buf)[(x + y * fb->stride)];
}

static void gs8_fill_rect(const gfx_fb_t *fb, unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint32_t col) {
    uint8_t *pixel = &((uint8_t *)fb->buf)[(x + y * fb->stride)];
    while (h--) {
        memset(pixel, (int)col, w);
        pixel += fb->stride;
    }
}

static gfx_fb_ops_t formats[] = {
    [GFX_MVLSB] = {mvlsb_setpixel, mvlsb_getpixel, mvlsb_fill_rect},
    [GFX_RGB565] = {rgb565_setpixel, rgb565_getpixel, rgb565_fill_rect},
    [GFX_GS2_HMSB] = {gs2_hmsb_setpixel, gs2_hmsb_getpixel, gs2_hmsb_fill_rect},
    [GFX_GS4_HMSB] = {gs4_hmsb_setpixel, gs4_hmsb_getpixel, gs4_hmsb_fill_rect},
    [GFX_GS8] = {gs8_setpixel, gs8_getpixel, gs8_fill_rect},
    [GFX_MHLSB] = {mono_horiz_setpixel, mono_horiz_getpixel, mono_horiz_fill_rect},
    [GFX_MHMSB] = {mono_horiz_setpixel, mono_horiz_getpixel, mono_horiz_fill_rect},
    [GFX_RGB888] = {rgb888_setpixel, rgb888_getpixel, rgb888_fill_rect},
};

int gfx_fb_color_depth(int format) {
    switch (format) {
        case GFX_MVLSB:
        case GFX_MHLSB:
        case GFX_MHMSB:
            return 1;
        case GFX_GS2_HMSB:
            return 2;
        case GFX_GS4_HMSB:
            return 4;
        case GFX_GS8:
            return 8;
        case GFX_RGB565:
            return 16;
        case GFX_RGB888:
            return 24;
        default:
            return 0;
    }
}

int gfx_fb_validate_buffer(size_t buf_len, int width, int height, int format, int stride) {
    if (width < 1 || height < 1 || width > 0xffff || height > 0xffff || stride > 0xffff || stride < width) {
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
        case GFX_RGB888:
            bpp = 24;
            break;
        default:
            return -1;
    }

    if ((strides_required * (size_t)stride + (height_required - strides_required) * width_required) * bpp / 8 > buf_len) {
        return -1;
    }
    return 0;
}

void gfx_fb_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col) {
    formats[fb->format].setpixel(fb, x, y, col);
}

uint32_t gfx_fb_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y) {
    return formats[fb->format].getpixel(fb, x, y);
}

void gfx_fb_setpixel_checked(const gfx_fb_t *fb, int x, int y, int col, int mask) {
    if (mask && 0 <= x && x < fb->width && 0 <= y && y < fb->height) {
        gfx_fb_setpixel(fb, (unsigned int)x, (unsigned int)y, (uint32_t)col);
    }
}

void gfx_fb_fill_rect_raw(const gfx_fb_t *fb, int x, int y, int w, int h, uint32_t col) {
    if (h < 1 || w < 1 || x + w <= 0 || y + h <= 0 || y >= fb->height || x >= fb->width) {
        return;
    }
    int xend = MIN(fb->width, x + w);
    int yend = MIN(fb->height, y + h);
    x = MAX(x, 0);
    y = MAX(y, 0);
    formats[fb->format].fill_rect(fb, (unsigned int)x, (unsigned int)y, (unsigned int)(xend - x), (unsigned int)(yend - y), col);
}

gfx_area_t gfx_fb_fill_rect(const gfx_fb_t *fb, int x, int y, int w, int h, uint32_t col) {
    gfx_fb_fill_rect_raw(fb, x, y, w, h, col);
    return gfx_area_from_rect(x, y, w, h);
}

gfx_area_t gfx_fb_fill(const gfx_fb_t *fb, uint32_t col) {
    formats[fb->format].fill_rect(fb, 0, 0, fb->width, fb->height, col);
    return gfx_area_from_rect(0, 0, fb->width, fb->height);
}

static int fb_canvas_pixel(void *ctx, int x, int y, int c, int set) {
    const gfx_fb_t *fb = (const gfx_fb_t *)ctx;
    if (set) {
        if (0 <= x && x < fb->width && 0 <= y && y < fb->height) {
            gfx_fb_setpixel(fb, (unsigned int)x, (unsigned int)y, (uint32_t)c);
        }
        return 0;
    }
    if (0 <= x && x < fb->width && 0 <= y && y < fb->height) {
        return (int)gfx_fb_getpixel(fb, (unsigned int)x, (unsigned int)y);
    }
    return 0;
}

static void fb_canvas_hline(void *ctx, int x, int y, int w, int c) {
    gfx_fb_fill_rect_raw((const gfx_fb_t *)ctx, x, y, w, 1, (uint32_t)c);
}

static void fb_canvas_vline(void *ctx, int x, int y, int h, int c) {
    gfx_fb_fill_rect_raw((const gfx_fb_t *)ctx, x, y, 1, h, (uint32_t)c);
}

static void fb_canvas_fill_rect(void *ctx, int x, int y, int w, int h, int c) {
    gfx_fb_fill_rect_raw((const gfx_fb_t *)ctx, x, y, w, h, (uint32_t)c);
}

void gfx_fb_canvas_init(gfx_canvas_t *canvas, const gfx_fb_t *fb) {
    canvas->ctx = (void *)fb;
    canvas->width = fb->width;
    canvas->height = fb->height;
    canvas->pixel = fb_canvas_pixel;
    canvas->hline = fb_canvas_hline;
    canvas->vline = fb_canvas_vline;
    canvas->fill_rect = fb_canvas_fill_rect;
}

void gfx_fb_scroll(gfx_fb_t *fb, int xstep, int ystep) {
    unsigned int sx, y, xend, yend;
    int dx, dy;
    if (xstep < 0) {
        if (-xstep >= fb->width) {
            return;
        }
        sx = 0;
        xend = fb->width + xstep;
        dx = 1;
    } else {
        if (xstep >= fb->width) {
            return;
        }
        sx = fb->width - 1;
        xend = xstep - 1;
        dx = -1;
    }
    if (ystep < 0) {
        if (-ystep >= fb->height) {
            return;
        }
        y = 0;
        yend = fb->height + ystep;
        dy = 1;
    } else {
        if (ystep >= fb->height) {
            return;
        }
        y = fb->height - 1;
        yend = ystep - 1;
        dy = -1;
    }
    for (; (int)y != yend; y += dy) {
        for (unsigned int x = sx; (int)x != xend; x += dx) {
            uint32_t col = gfx_fb_getpixel(fb, x - xstep, y - ystep);
            gfx_fb_setpixel(fb, x, y, col);
        }
    }
}
