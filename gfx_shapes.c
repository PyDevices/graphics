/*
 * Shape drawing algorithms via gfx_canvas_t.
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
#include <string.h>

#include "gfx_shapes.h"

#define ELLIPSE_MASK_FILL (0x10)
#define ELLIPSE_MASK_ALL (0x0f)
#define ELLIPSE_MASK_Q1 (0x01)
#define ELLIPSE_MASK_Q2 (0x02)
#define ELLIPSE_MASK_Q3 (0x04)
#define ELLIPSE_MASK_Q4 (0x08)

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

gfx_area_t gfx_shapes_line(const gfx_canvas_t *canvas, int ox1, int oy1, int ox2, int oy2, int col) {
    int x1 = ox1;
    int y1 = oy1;
    int x2 = ox2;
    int y2 = oy2;
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
            if (0 <= y1 && y1 < canvas->width && 0 <= x1 && x1 < canvas->height) {
                canvas->pixel(canvas->ctx, y1, x1, col, 1);
            }
        } else {
            if (0 <= x1 && x1 < canvas->width && 0 <= y1 && y1 < canvas->height) {
                canvas->pixel(canvas->ctx, x1, y1, col, 1);
            }
        }
        while (e >= 0) {
            y1 += sy;
            e -= 2 * dx;
        }
        x1 += sx;
        e += 2 * dy;
    }
    canvas->pixel(canvas->ctx, x2, y2, col, 1);

    int x_min = MIN(ox1, ox2);
    int x_max = MAX(ox1, ox2);
    int y_min = MIN(oy1, oy2);
    int y_max = MAX(oy1, oy2);
    return gfx_area_from_rect(x_min, y_min, x_max - x_min + 1, y_max - y_min + 1);
}

static void draw_ellipse_points(const gfx_canvas_t *canvas, int cx, int cy, int x, int y, int col, int mask) {
    if (mask & ELLIPSE_MASK_FILL) {
        if (mask & ELLIPSE_MASK_Q1) {
            canvas->fill_rect(canvas->ctx, cx, cy - y, x + 1, 1, col);
        }
        if (mask & ELLIPSE_MASK_Q2) {
            canvas->fill_rect(canvas->ctx, cx - x, cy - y, x + 1, 1, col);
        }
        if (mask & ELLIPSE_MASK_Q3) {
            canvas->fill_rect(canvas->ctx, cx - x, cy + y, x + 1, 1, col);
        }
        if (mask & ELLIPSE_MASK_Q4) {
            canvas->fill_rect(canvas->ctx, cx, cy + y, x + 1, 1, col);
        }
    } else {
        if (mask & ELLIPSE_MASK_Q1) {
            canvas->pixel(canvas->ctx, cx + x, cy - y, col, 1);
        }
        if (mask & ELLIPSE_MASK_Q2) {
            canvas->pixel(canvas->ctx, cx - x, cy - y, col, 1);
        }
        if (mask & ELLIPSE_MASK_Q3) {
            canvas->pixel(canvas->ctx, cx - x, cy + y, col, 1);
        }
        if (mask & ELLIPSE_MASK_Q4) {
            canvas->pixel(canvas->ctx, cx + x, cy + y, col, 1);
        }
    }
}

gfx_area_t gfx_shapes_ellipse(const gfx_canvas_t *canvas, int cx, int cy, int xradius, int yradius, int col, int fill, int mask_part) {
    int mask = fill ? ELLIPSE_MASK_FILL : 0;
    mask |= mask_part & ELLIPSE_MASK_ALL;

    if (xradius == 0 && yradius == 0) {
        canvas->pixel(canvas->ctx, cx, cy, col, 1);
        return gfx_area_from_rect(cx, cy, 1, 1);
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
        draw_ellipse_points(canvas, cx, cy, x, y, col, mask);
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
        draw_ellipse_points(canvas, cx, cy, x, y, col, mask);
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
    return gfx_area_from_rect(cx - xradius, cy - yradius, 2 * xradius + 1, 2 * yradius + 1);
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
