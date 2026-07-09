/*
 * Font and text rendering.
 * SPDX-License-Identifier: MIT
 */

#include "gfx_font.h"

#include "font_petme128_8x8.h"

gfx_area_t gfx_font_text8(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col) {
    int start_x = x0;
    int max_x = x0;
    int max_y = y0 + 7;

    for (; *str; ++str) {
        int chr = (unsigned char)*str;
        if (chr < 32 || chr > 127) {
            chr = 127;
        }
        const uint8_t *chr_data = &font_petme128_8x8[(chr - 32) * 8];
        for (int j = 0; j < 8; j++, x0++) {
            if (0 <= x0 && x0 < canvas->width) {
                unsigned int vline_data = chr_data[j];
                for (int yy = y0; vline_data; vline_data >>= 1, yy++) {
                    if (vline_data & 1) {
                        if (0 <= yy && yy < canvas->height) {
                            canvas->pixel(canvas->ctx, x0, yy, col, 1);
                        }
                    }
                }
            }
        }
        max_x = x0;
    }
    return gfx_area_from_rect(start_x, y0, max_x - start_x, max_y - y0 + 1);
}
