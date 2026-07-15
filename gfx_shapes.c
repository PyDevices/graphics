/*
 * Shape drawing algorithms via gfx_canvas_t.
 * SPDX-License-Identifier: MIT
 */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "gfx_shapes.h"

static int clipped_pixel(void *ctx, int x, int y, int c, int set) {
    gfx_clipped_canvas_t *cc = (gfx_clipped_canvas_t *)ctx;
    if (!gfx_area_contains_point(&cc->clip, x, y)) {
        return set ? 0 : 0;
    }
    return cc->parent->pixel(cc->parent->ctx, x, y, c, set);
}

static void clipped_hline(void *ctx, int x, int y, int w, int c) {
    gfx_clipped_canvas_t *cc = (gfx_clipped_canvas_t *)ctx;
    gfx_area_t line = gfx_area_from_rect(x, y, w, 1);
    gfx_area_t clipped = gfx_area_clip(&line, &cc->clip);
    if (clipped.w > 0 && clipped.h > 0) {
        cc->parent->hline(cc->parent->ctx, clipped.x, clipped.y, clipped.w, c);
    }
}

static void clipped_vline(void *ctx, int x, int y, int h, int c) {
    gfx_clipped_canvas_t *cc = (gfx_clipped_canvas_t *)ctx;
    gfx_area_t line = gfx_area_from_rect(x, y, 1, h);
    gfx_area_t clipped = gfx_area_clip(&line, &cc->clip);
    if (clipped.w > 0 && clipped.h > 0) {
        cc->parent->vline(cc->parent->ctx, clipped.x, clipped.y, clipped.h, c);
    }
}

static void clipped_fill_rect(void *ctx, int x, int y, int w, int h, int c) {
    gfx_clipped_canvas_t *cc = (gfx_clipped_canvas_t *)ctx;
    gfx_area_t rect = gfx_area_from_rect(x, y, w, h);
    gfx_area_t clipped = gfx_area_clip(&rect, &cc->clip);
    if (clipped.w > 0 && clipped.h > 0) {
        cc->parent->fill_rect(cc->parent->ctx, clipped.x, clipped.y, clipped.w, clipped.h, c);
    }
}

void gfx_clipped_canvas_init(gfx_clipped_canvas_t *cc, const gfx_canvas_t *parent, const gfx_area_t *clip) {
    cc->parent = parent;
    cc->clip = *clip;
    cc->base.ctx = cc;
    cc->base.width = parent->width;
    cc->base.height = parent->height;
    cc->base.pixel = clipped_pixel;
    cc->base.hline = clipped_hline;
    cc->base.vline = clipped_vline;
    cc->base.fill_rect = clipped_fill_rect;
}

gfx_area_t gfx_shapes_pixel(const gfx_canvas_t *canvas, int x, int y, int c) {
    canvas->pixel(canvas->ctx, x, y, c, 1);
    return gfx_area_from_rect(x, y, 1, 1);
}

gfx_area_t gfx_shapes_hline(const gfx_canvas_t *canvas, int x, int y, int w, int c) {
    canvas->hline(canvas->ctx, x, y, w, c);
    return gfx_area_from_rect(x, y, w, 1);
}

gfx_area_t gfx_shapes_vline(const gfx_canvas_t *canvas, int x, int y, int h, int c) {
    canvas->vline(canvas->ctx, x, y, h, c);
    return gfx_area_from_rect(x, y, 1, h);
}

gfx_area_t gfx_shapes_fill_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c) {
    canvas->fill_rect(canvas->ctx, x, y, w, h, c);
    return gfx_area_from_rect(x, y, w, h);
}

gfx_area_t gfx_shapes_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c, int fill) {
    if (fill) {
        canvas->fill_rect(canvas->ctx, x, y, w, h, c);
    } else {
        canvas->fill_rect(canvas->ctx, x, y, w, 1, c);
        canvas->fill_rect(canvas->ctx, x, y + h - 1, w, 1, c);
        canvas->fill_rect(canvas->ctx, x, y, 1, h, c);
        canvas->fill_rect(canvas->ctx, x + w - 1, y, 1, h, c);
    }
    return gfx_area_from_rect(x, y, w, h);
}

gfx_area_t gfx_shapes_fill(const gfx_canvas_t *canvas, int c) {
    canvas->fill_rect(canvas->ctx, 0, 0, canvas->width, canvas->height, c);
    return gfx_area_from_rect(0, 0, canvas->width, canvas->height);
}

gfx_area_t gfx_shapes_line(const gfx_canvas_t *canvas, int ox0, int oy0, int ox1, int oy1, int col) {
    int x0 = ox0;
    int y0 = oy0;
    int x1 = ox1;
    int y1 = oy1;

    if (x0 == x1) {
        int y = y0 < y1 ? y0 : y1;
        return gfx_shapes_vline(canvas, x0, y, abs(y1 - y0) + 1, col);
    }
    if (y0 == y1) {
        int x = x0 < x1 ? x0 : x1;
        return gfx_shapes_hline(canvas, x, y0, abs(x1 - x0) + 1, col);
    }

    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        int t = x0;
        x0 = y0;
        y0 = t;
        t = x1;
        x1 = y1;
        y1 = t;
    }
    if (x0 > x1) {
        int t = x0;
        x0 = x1;
        x1 = t;
        t = y0;
        y0 = y1;
        y1 = t;
    }

    int dx = x1 - x0;
    int dy = abs(y1 - y0);
    int err = dx / 2;
    int ystep = y0 < y1 ? 1 : -1;
    while (x0 <= x1) {
        if (steep) {
            gfx_shapes_pixel(canvas, y0, x0, col);
        } else {
            gfx_shapes_pixel(canvas, x0, y0, col);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
        x0 += 1;
    }

    int x_min = MIN(ox0, ox1);
    int y_min = MIN(oy0, oy1);
    int x_max = MAX(ox0, ox1);
    int y_max = MAX(oy0, oy1);
    return gfx_area_from_rect(x_min, y_min, x_max - x_min + 1, y_max - y_min + 1);
}

int gfx_intersect_rect(int x, int y, int w, int h, const gfx_area_t *clip, gfx_area_t *out) {
    if (w <= 0 || h <= 0) {
        return 0;
    }
    gfx_area_t rect = gfx_area_from_rect(x, y, w, h);
    *out = gfx_area_clip(&rect, clip);
    if (out->w <= 0 || out->h <= 0) {
        return 0;
    }
    return 1;
}

void gfx_crop_rgb565_buffer(const void *src, int src_w, int src_x, int src_y, int crop_w, int crop_h, void *dst) {
    const uint8_t *src_bytes = (const uint8_t *)src;
    uint8_t *dst_bytes = (uint8_t *)dst;
    int row_bytes = crop_w * 2;
    for (int row = 0; row < crop_h; row++) {
        int src_start = ((src_y + row) * src_w + src_x) * 2;
        int dst_start = row * row_bytes;
        memcpy(dst_bytes + dst_start, src_bytes + src_start, (size_t)row_bytes);
    }
}

static void ellipse_plot(const gfx_canvas_t *canvas, int x0, int y0, int x1, int y1, int x_offset, int y_offset, int c, int fill, int m) {
    if (fill) {
        if (m & 0x01) {
            gfx_shapes_hline(canvas, x0 + x_offset, y0 - y1 - y_offset, x1, c);
        }
        if (m & 0x02) {
            gfx_shapes_hline(canvas, x0 - x1 - x_offset, y0 - y1 - y_offset, x1, c);
        }
        if (m & 0x04) {
            gfx_shapes_hline(canvas, x0 - x1 - x_offset, y0 + y1 + y_offset, x1, c);
        }
        if (m & 0x08) {
            gfx_shapes_hline(canvas, x0 + x_offset, y0 + y1 + y_offset, x1, c);
        }
    } else {
        if (m & 0x01) {
            gfx_shapes_pixel(canvas, x0 + x1 + x_offset, y0 - y1 - y_offset, c);
        }
        if (m & 0x02) {
            gfx_shapes_pixel(canvas, x0 - x1 - x_offset, y0 - y1 - y_offset, c);
        }
        if (m & 0x04) {
            gfx_shapes_pixel(canvas, x0 - x1 - x_offset, y0 + y1 + y_offset, c);
        }
        if (m & 0x08) {
            gfx_shapes_pixel(canvas, x0 + x1 + x_offset, y0 + y1 + y_offset, c);
        }
    }
}

gfx_area_t gfx_shapes_ellipse(const gfx_canvas_t *canvas, int x0, int y0, int r1, int r2, int c, int fill, int mask_part, int w, int h) {
    if (r1 < 1 || r2 < 1) {
        return gfx_area_from_rect(0, 0, 0, 0);
    }

    int x_side = w ? (w - 2 * r1) : 0;
    int y_side = h ? (h - 2 * r2) : 0;
    int x_offset = w ? (x_side / 2) : 0;
    int y_offset = h ? (y_side / 2) : 0;

    if (fill) {
        if (y_offset > 0) {
            gfx_shapes_fill_rect(canvas, x0 - w / 2, y0 - y_offset, w, y_side, c);
        }
        if (x_offset > 0) {
            gfx_shapes_fill_rect(canvas, x0 - x_offset, y0 - h / 2, x_side, r1, c);
            gfx_shapes_fill_rect(canvas, x0 - x_offset, y0 + h / 2 - r1, x_side, r1, c);
        }
    }

    if (x_offset > 0) {
        gfx_shapes_hline(canvas, x0 - x_offset, y0 - h / 2, x_side, c);
        gfx_shapes_hline(canvas, x0 - x_offset, y0 + h / 2, x_side, c);
    }
    if (y_offset > 0) {
        gfx_shapes_vline(canvas, x0 - w / 2, y0 - y_offset, y_side, c);
        gfx_shapes_vline(canvas, x0 + w / 2, y0 - y_offset, y_side, c);
    }

    int a2 = r1 * r1;
    int b2 = r2 * r2;
    int fa2 = 4 * a2;
    int fb2 = 4 * b2;

    int x1 = r1;
    int y1 = 0;
    int sigma = 2 * a2 + b2 * (1 - 2 * r1);
    while (a2 * y1 <= b2 * x1) {
        ellipse_plot(canvas, x0, y0, x1, y1, x_offset, y_offset, c, fill, mask_part);
        if (sigma >= 0) {
            sigma += fb2 * (1 - x1);
            x1 -= 1;
        }
        sigma += a2 * ((4 * y1) + 6);
        y1 += 1;
    }

    x1 = 0;
    y1 = r2;
    sigma = 2 * b2 + a2 * (1 - 2 * r2);
    while (b2 * x1 <= a2 * y1) {
        ellipse_plot(canvas, x0, y0, x1, y1, x_offset, y_offset, c, fill, mask_part);
        if (sigma >= 0) {
            sigma += fa2 * (1 - y1);
            y1 -= 1;
        }
        sigma += b2 * ((4 * x1) + 6);
        x1 += 1;
    }

    return gfx_area_from_rect(x0 - r1 - x_offset, y0 - r2 - y_offset, 2 * (r1 + x_offset), 2 * (r2 + y_offset));
}

int gfx_shapes_poly_int_from_buffer(const void *buf, size_t len, size_t itemsize, const char *fmt, size_t index, int *out) {
    size_t offset = index * itemsize;
    if (!fmt || !fmt[0] || offset + itemsize > len) {
        return -1;
    }
    const char *ptr = (const char *)buf + offset;
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
            return -1;
    }
}

gfx_area_t gfx_shapes_poly(const gfx_canvas_t *canvas, int x, int y, const void *coords, size_t coords_len, size_t itemsize, const char *fmt, int col, int fill) {
    int n_poly = (int)(coords_len / (itemsize * 2));
    if (n_poly == 0) {
        return gfx_area_from_rect(0, 0, 0, 0);
    }

    if (fill) {
        int y_min = INT_MAX;
        int y_max = INT_MIN;
        for (int i = 0; i < n_poly; i++) {
            int py;
            if (gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, (size_t)i * 2 + 1, &py) < 0) {
                return gfx_area_from_rect(0, 0, 0, 0);
            }
            y_min = MIN(y_min, py);
            y_max = MAX(y_max, py);
        }
        for (int row = y_min; row <= y_max; row++) {
            int nodes[256];
            if (n_poly > 256) {
                return gfx_area_from_rect(0, 0, 0, 0);
            }
            int n_nodes = 0;
            int px1, py1;
            if (gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, 0, &px1) < 0
                || gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, 1, &py1) < 0) {
                return gfx_area_from_rect(0, 0, 0, 0);
            }
            int i = n_poly * 2 - 1;
            do {
                int py2, px2;
                if (gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, (size_t)i, &py2) < 0) {
                    return gfx_area_from_rect(0, 0, 0, 0);
                }
                i--;
                if (gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, (size_t)i, &px2) < 0) {
                    return gfx_area_from_rect(0, 0, 0, 0);
                }
                i--;
                if (py1 != py2 && ((py1 > row && py2 <= row) || (py1 <= row && py2 > row))) {
                    int node = (32 * px1 + 32 * (px2 - px1) * (row - py1) / (py2 - py1) + 16) / 32;
                    nodes[n_nodes++] = node;
                } else if (row == MAX(py1, py2)) {
                    if (py1 < py2) {
                        canvas->pixel(canvas->ctx, x + px2, y + py2, col, 1);
                    } else if (py2 < py1) {
                        canvas->pixel(canvas->ctx, x + px1, y + py1, col, 1);
                    } else {
                        gfx_shapes_line(canvas, x + px1, y + py1, x + px2, y + py2, col);
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
                canvas->fill_rect(canvas->ctx, x + nodes[i], y + row, (nodes[i + 1] - nodes[i]) + 1, 1, col);
            }
        }
    } else {
        int px1, py1;
        if (gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, 0, &px1) < 0
            || gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, 1, &py1) < 0) {
            return gfx_area_from_rect(0, 0, 0, 0);
        }
        int i = n_poly * 2 - 1;
        do {
            int py2, px2;
            if (gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, (size_t)i, &py2) < 0) {
                return gfx_area_from_rect(0, 0, 0, 0);
            }
            i--;
            if (gfx_shapes_poly_int_from_buffer(coords, coords_len, itemsize, fmt, (size_t)i, &px2) < 0) {
                return gfx_area_from_rect(0, 0, 0, 0);
            }
            i--;
            gfx_shapes_line(canvas, x + px1, y + py1, x + px2, y + py2, col);
            px1 = px2;
            py1 = py2;
        } while (i >= 0);
    }
    return gfx_area_from_rect(x, y, 0, 0);
}

gfx_area_t gfx_shapes_blit(const gfx_canvas_t *canvas, const gfx_fb_t *source, int x, int y, int key, const gfx_fb_t *palette) {
    if ((x >= canvas->width) || (y >= canvas->height) || (-x >= source->width) || (-y >= source->height)) {
        return gfx_area_from_rect(0, 0, 0, 0);
    }

    int x0 = MAX(0, x);
    int y0 = MAX(0, y);
    int x1 = MAX(0, -x);
    int y1 = MAX(0, -y);
    int x0end = MIN(canvas->width, x + source->width);
    int y0end = MIN(canvas->height, y + source->height);

    for (; y0 < y0end; ++y0) {
        int cx1 = x1;
        for (int cx0 = x0; cx0 < x0end; ++cx0) {
            uint32_t col = gfx_fb_getpixel(source, (unsigned int)cx1, (unsigned int)y1);
            if (palette) {
                col = gfx_fb_getpixel(palette, (unsigned int)col, 0);
            }
            if ((int)col != key) {
                canvas->pixel(canvas->ctx, cx0, y0, (int)col, 1);
            }
            ++cx1;
        }
        ++y1;
    }
    return gfx_area_from_rect(x0, y0, x0end - x0, y0end - y0);
}

static void circle_helper(const gfx_canvas_t *canvas, int x0, int y0, int r, int c, int x_offset, int y_offset) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;
    while (x < y) {
        if (f >= 0) {
            y -= 1;
            ddF_y += 2;
            f += ddF_y;
        }
        x += 1;
        ddF_x += 2;
        f += ddF_x;
        int offset_x = x + x_offset;
        int offset_y = y + y_offset;
        gfx_shapes_pixel(canvas, x0 + offset_x - 1, y0 - offset_y, c);
        gfx_shapes_pixel(canvas, x0 - offset_x, y0 - offset_y, c);
        gfx_shapes_pixel(canvas, x0 + offset_x - 1, y0 + offset_y - 1, c);
        gfx_shapes_pixel(canvas, x0 - offset_x, y0 + offset_y - 1, c);
        offset_x = y + x_offset;
        offset_y = x + y_offset;
        gfx_shapes_pixel(canvas, x0 + offset_x - 1, y0 + offset_y - 1, c);
        gfx_shapes_pixel(canvas, x0 - offset_x, y0 + offset_y - 1, c);
        gfx_shapes_pixel(canvas, x0 + offset_x - 1, y0 - offset_y, c);
        gfx_shapes_pixel(canvas, x0 - offset_x, y0 - offset_y, c);
    }
}

static gfx_area_t fill_circle_helper(const gfx_canvas_t *canvas, int x0, int y0, int r, int c, int x_offset, int y_offset) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;
    while (x < y) {
        if (f >= 0) {
            y -= 1;
            ddF_y += 2;
            f += ddF_y;
        }
        x += 1;
        ddF_x += 2;
        f += ddF_x;
        int offset_x = x + x_offset;
        int offset_y = y + y_offset;
        gfx_shapes_vline(canvas, x0 - offset_x, y0 - offset_y, 2 * offset_y, c);
        gfx_shapes_vline(canvas, x0 + offset_x - 1, y0 - offset_y, 2 * offset_y, c);
        offset_x = y + x_offset;
        offset_y = x + y_offset;
        gfx_shapes_vline(canvas, x0 - offset_x, y0 - offset_y, 2 * offset_y, c);
        gfx_shapes_vline(canvas, x0 + offset_x - 1, y0 - offset_y, 2 * offset_y, c);
    }
    return gfx_area_from_rect(x0 - r, y0 - r, 2 * r, 2 * r);
}

gfx_area_t gfx_shapes_circle(const gfx_canvas_t *canvas, int x0, int y0, int r, int c, int fill) {
    if (fill) {
        return fill_circle_helper(canvas, x0, y0, r, c, 0, 0);
    }
    circle_helper(canvas, x0, y0, r, c, 0, 0);
    return gfx_area_from_rect(x0 - r, y0 - r, 2 * r, 2 * r);
}

static gfx_area_t fill_round_rect(const gfx_canvas_t *canvas, int x0, int y0, int w, int h, int r, int c) {
    if (r > w / 2) {
        r = w / 2;
    }
    if (r > h / 2) {
        r = h / 2;
    }
    gfx_shapes_fill_rect(canvas, x0 + r, y0, w - 2 * r, h, c);
    fill_circle_helper(canvas, x0 + w / 2, y0 + h / 2, r, c, w / 2 - r, h / 2 - r);
    return gfx_area_from_rect(x0, y0, w, h);
}

gfx_area_t gfx_shapes_round_rect(const gfx_canvas_t *canvas, int x0, int y0, int w, int h, int r, int c, int fill) {
    if (r == 0) {
        return gfx_shapes_rect(canvas, x0, y0, w, h, c, fill);
    }
    if (fill) {
        return fill_round_rect(canvas, x0, y0, w, h, r, c);
    }
    if (r > w / 2) {
        r = w / 2;
    }
    if (r > h / 2) {
        r = h / 2;
    }
    gfx_shapes_hline(canvas, x0 + r, y0, w - 2 * r, c);
    gfx_shapes_hline(canvas, x0 + r, y0 + h - 1, w - 2 * r, c);
    gfx_shapes_vline(canvas, x0, y0 + r, h - 2 * r, c);
    gfx_shapes_vline(canvas, x0 + w - 1, y0 + r, h - 2 * r, c);
    circle_helper(canvas, x0 + w / 2, y0 + h / 2, r, c, w / 2 - r, h / 2 - r);
    return gfx_area_from_rect(x0, y0, w, h);
}

static gfx_area_t fill_triangle(const gfx_canvas_t *canvas, int x0, int y0, int x1, int y1, int x2, int y2, int c) {
    if (y0 > y1) {
        int t;
        t = y0; y0 = y1; y1 = t;
        t = x0; x0 = x1; x1 = t;
    }
    if (y1 > y2) {
        int t;
        t = y2; y2 = y1; y1 = t;
        t = x2; x2 = x1; x1 = t;
    }
    if (y0 > y1) {
        int t;
        t = y0; y0 = y1; y1 = t;
        t = x0; x0 = x1; x1 = t;
    }
    int a, b, last;
    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1; else if (x1 > b) b = x1;
        if (x2 < a) a = x2; else if (x2 > b) b = x2;
        gfx_shapes_hline(canvas, a, y0, b - a + 1, c);
        return gfx_area_from_rect(MIN(x0, MIN(x1, x2)), MIN(y0, MIN(y1, y2)),
            MAX(x0, MAX(x1, x2)) - MIN(x0, MIN(x1, x2)) + 1,
            MAX(y0, MAX(y1, y2)) - MIN(y0, MIN(y1, y2)) + 1);
    }
    int dx01 = x1 - x0, dy01 = y1 - y0;
    int dx02 = x2 - x0, dy02 = y2 - y0;
    int dx12 = x2 - x1, dy12 = y2 - y1;
    if (dy01 == 0) dy01 = 1;
    if (dy02 == 0) dy02 = 1;
    if (dy12 == 0) dy12 = 1;
    int sa = 0, sb = 0;
    int y = y0;
    last = (y0 == y1) ? y1 - 1 : y1;
    while (y <= last) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) { int t = a; a = b; b = t; }
        gfx_shapes_hline(canvas, a, y, b - a + 1, c);
        y += 1;
    }
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    while (y <= y2) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) { int t = a; a = b; b = t; }
        gfx_shapes_hline(canvas, a, y, b - a + 1, c);
        y += 1;
    }
    return gfx_area_from_rect(MIN(x0, MIN(x1, x2)), MIN(y0, MIN(y1, y2)),
        MAX(x0, MAX(x1, x2)) - MIN(x0, MIN(x1, x2)) + 1,
        MAX(y0, MAX(y1, y2)) - MIN(y0, MIN(y1, y2)) + 1);
}

gfx_area_t gfx_shapes_triangle(const gfx_canvas_t *canvas, int x0, int y0, int x1, int y1, int x2, int y2, int c, int fill) {
    if (fill) {
        return fill_triangle(canvas, x0, y0, x1, y1, x2, y2, c);
    }
    gfx_shapes_line(canvas, x0, y0, x1, y1, c);
    gfx_shapes_line(canvas, x1, y1, x2, y2, c);
    gfx_shapes_line(canvas, x2, y2, x0, y0, c);
    int left = MIN(x0, MIN(x1, x2));
    int top = MIN(y0, MIN(y1, y2));
    int right = MAX(x0, MAX(x1, x2));
    int bottom = MAX(y0, MAX(y1, y2));
    return gfx_area_from_rect(left, top, right - left, bottom - top);
}

gfx_area_t gfx_shapes_arc(const gfx_canvas_t *canvas, int x, int y, int r, float a0, float a1, int c) {
    const int resolution = 60;
    float ra0 = a0 * (float)M_PI / 180.0f;
    float ra1 = a1 * (float)M_PI / 180.0f;
    int x0 = x + (int)(r * cosf(ra0));
    int y0 = y + (int)(r * sinf(ra0));
    int x_min = x0, x_max = x0;
    int y_min = y0, y_max = y0;
    int start = (int)(ra0 * resolution);
    int end = (int)(ra1 * resolution);
    int step = (a1 > a0) ? 1 : -1;
    for (int a = start; step > 0 ? a < end : a > end; a += step) {
        float ar = (float)a / resolution;
        int x1 = x + (int)(r * cosf(ar));
        int y1 = y + (int)(r * sinf(ar));
        gfx_shapes_line(canvas, x0, y0, x1, y1, c);
        x_min = MIN(x0, MIN(x1, x_min));
        x_max = MAX(x0, MAX(x1, x_max));
        y_min = MIN(y0, MIN(y1, y_min));
        y_max = MAX(y0, MAX(y1, y_max));
        x0 = x1;
        y0 = y1;
    }
    return gfx_area_from_rect(x_min, y_min, x_max - x_min, y_max - y_min);
}

static int rgb565_from_rgb(int r, int g, int b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

static void rgb565_unpack(int c, int *r, int *g, int *b) {
    *r = (c >> 8) & 0xF8;
    *g = (c >> 3) & 0xFC;
    *b = (c << 3) & 0xF8;
}

gfx_area_t gfx_shapes_gradient_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c1, int c2, int vertical) {
    if (c1 == c2) {
        return gfx_shapes_fill_rect(canvas, x, y, w, h, c1);
    }
    int r1, g1, b1, r2, g2, b2;
    rgb565_unpack(c1, &r1, &g1, &b1);
    rgb565_unpack(c2, &r2, &g2, &b2);
    if (vertical) {
        for (int j = 0; j < h; j++) {
            int r = r1 + (r2 - r1) * j / h;
            int g = g1 + (g2 - g1) * j / h;
            int b = b1 + (b2 - b1) * j / h;
            gfx_shapes_fill_rect(canvas, x, y + j, w, 1, rgb565_from_rgb(r, g, b));
        }
    } else {
        for (int i = 0; i < w; i++) {
            int r = r1 + (r2 - r1) * i / w;
            int g = g1 + (g2 - g1) * i / w;
            int b = b1 + (b2 - b1) * i / w;
            gfx_shapes_fill_rect(canvas, x + i, y, 1, h, rgb565_from_rgb(r, g, b));
        }
    }
    return gfx_area_from_rect(x, y, w, h);
}

gfx_area_t gfx_shapes_blit_rect(const gfx_canvas_t *canvas, const void *buf, int x, int y, int w, int h, int bpp) {
    const uint8_t *src = (const uint8_t *)buf;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int col_val;
            if (bpp == 2) {
                col_val = (int)(src[(row * w + col) * 2] | (src[(row * w + col) * 2 + 1] << 8));
            } else {
                col_val = src[row * w + col];
            }
            gfx_shapes_pixel(canvas, x + col, y + row, col_val);
        }
    }
    return gfx_area_from_rect(x, y, w, h);
}

gfx_area_t gfx_shapes_blit_transparent(const gfx_canvas_t *canvas, const void *buf, int x, int y, int w, int h, int key, int bpp) {
    const uint8_t *src = (const uint8_t *)buf;
    int stride = w * bpp;
    uint8_t key_bytes[2];
    key_bytes[0] = (uint8_t)(key & 0xFF);
    key_bytes[1] = (uint8_t)((key >> 8) & 0xFF);
    for (int j = 0; j < h; j++) {
        int rowstart = j * stride;
        int colstart = 0;
        while (colstart < stride) {
            int startoffset = rowstart + colstart;
            int match = 1;
            for (int b = 0; b < bpp; b++) {
                if (src[startoffset + b] != key_bytes[b]) {
                    match = 0;
                    break;
                }
            }
            if (!match) {
                int colend = colstart;
                while (colend < stride) {
                    int endoffset = rowstart + colend;
                    int km = 1;
                    for (int b = 0; b < bpp; b++) {
                        if (src[endoffset + b] != key_bytes[b]) {
                            km = 0;
                            break;
                        }
                    }
                    if (km) {
                        break;
                    }
                    colend += bpp;
                }
                gfx_shapes_blit_rect(canvas, src + rowstart + colstart, x + colstart / bpp, y + j, (colend - colstart) / bpp, 1, bpp);
                colstart = colend;
            } else {
                colstart += bpp;
            }
        }
    }
    return gfx_area_from_rect(x, y, w, h);
}

gfx_area_t gfx_shapes_polygon(const gfx_canvas_t *canvas, const int *points, size_t n_points, int x, int y, int color, float angle, int center_x, int center_y) {
    if (n_points < 3) {
        return gfx_area_from_rect(0, 0, 0, 0);
    }
    int rx[64], ry[64];
    if (n_points > 64) {
        return gfx_area_from_rect(0, 0, 0, 0);
    }
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    for (size_t i = 0; i < n_points; i++) {
        int px = points[i * 2];
        int py = points[i * 2 + 1];
        if (angle != 0.0f) {
            rx[i] = x + center_x + (int)((px - center_x) * cos_a - (py - center_y) * sin_a);
            ry[i] = y + center_y + (int)((px - center_x) * sin_a + (py - center_y) * cos_a);
        } else {
            rx[i] = x + px;
            ry[i] = y + py;
        }
    }
    int left = rx[0], right = rx[0], top = ry[0], bottom = ry[0];
    for (size_t i = 1; i < n_points; i++) {
        left = MIN(left, rx[i]);
        right = MAX(right, rx[i]);
        top = MIN(top, ry[i]);
        bottom = MAX(bottom, ry[i]);
        gfx_shapes_line(canvas, rx[i - 1], ry[i - 1], rx[i], ry[i], color);
    }
    return gfx_area_from_rect(left, top, right - left, bottom - top);
}
