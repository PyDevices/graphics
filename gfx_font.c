/*
 * Font and text rendering.
 * SPDX-License-Identifier: MIT
 */

#include "gfx_font.h"

#include <string.h>
#if GFX_ENABLE_HOST_STDIO
#include <stdio.h>
#include <stdlib.h>
#endif

#include "font_8x8.h"
#include "font_8x14.h"
#include "font_8x16.h"

static gfx_font_t default_font8;
static gfx_font_t default_font14;
static gfx_font_t default_font16;
static int defaults_ready;

static void ensure_defaults(void) {
    if (defaults_ready) {
        return;
    }
    gfx_font_init_from_data(&default_font8, font_8x8, FONT_8X8_LEN, 8);
    gfx_font_init_from_data(&default_font14, font_8x14, FONT_8X14_LEN, 14);
    gfx_font_init_from_data(&default_font16, font_8x16, FONT_8X16_LEN, 16);
    defaults_ready = 1;
}

void gfx_font_init_default(gfx_font_t *font, int height) {
    ensure_defaults();
    if (height == 14) {
        *font = default_font14;
    } else if (height == 16) {
        *font = default_font16;
    } else {
        *font = default_font8;
    }
}

void gfx_font_init_from_data(gfx_font_t *font, const uint8_t *data, size_t len, int height) {
    font->data = data;
    font->data_len = len;
    font->height = height;
    font->width = 8;
    font->owns_data = 0;
}

void gfx_font_deinit(gfx_font_t *font) {
#if GFX_ENABLE_HOST_STDIO
    /* The host/CPython binding may load glyph data into a malloc'd buffer it
     * owns; free it. MicroPython builds always borrow a GC-managed buffer
     * (owns_data == 0), so this compiles out and pulls in no libc free(). */
    if (font->owns_data && font->data) {
        free((void *)font->data);
    }
#endif
    font->data = NULL;
    font->data_len = 0;
    font->owns_data = 0;
}

static uint8_t font_read_line(const gfx_font_t *font, unsigned char ch, int line) {
    if (!font->data || line < 0 || line >= font->height) {
        return 0;
    }
    size_t offset = (size_t)ch * (size_t)font->height + (size_t)line;
    if (offset >= font->data_len) {
        return 0;
    }
    return font->data[offset];
}

static gfx_area_t draw_char(const gfx_canvas_t *canvas, const gfx_font_t *font, unsigned char ch, int x, int y, int color, int scale, int inverted) {
    if (scale < 1) {
        scale = 1;
    }
    for (int char_y = 0; char_y < font->height; char_y++) {
        uint8_t line = font_read_line(font, ch, char_y);
        if (!line) {
            continue;
        }
        for (int char_x = 0; char_x < font->width; char_x++) {
            if ((line >> (font->width - char_x - 1)) & 0x1) {
                int px = x + (inverted ? (font->width - char_x - 1) : char_x) * scale;
                int py = y + (inverted ? (font->height - char_y - 1) : char_y) * scale;
                gfx_shapes_fill_rect(canvas, px, py, scale, scale, color);
            }
        }
    }
    return gfx_area_from_rect(x, y, font->width * scale, font->height * scale);
}

gfx_area_t gfx_font_draw_char(const gfx_canvas_t *canvas, const gfx_font_t *font, unsigned char ch, int x, int y, int color, int scale, int inverted) {
    return draw_char(canvas, font, ch, x, y, color, scale, inverted);
}

int gfx_font_text_width(const gfx_font_t *font, const char *str, int scale) {
    if (scale < 1) {
        scale = 1;
    }
    int n = 0;
    if (str) {
        while (str[n]) {
            n++;
        }
    }
    return n * font->width * scale;
}

gfx_area_t gfx_font_text(const gfx_canvas_t *canvas, const gfx_font_t *font, const char *str, int x0, int y0, int col, int scale, int inverted) {
    int start_x = x0;
    int char_y = y0;
    int largest_x = 0;
    const char *line_start = str;
    for (const char *p = str; ; p++) {
        if (*p == '\n' || *p == '\0') {
            int last_x = x0;
            int i = 0;
            for (const char *c = line_start; c < p; c++, i++) {
                int char_x = x0 + i * font->width * scale;
                draw_char(canvas, font, (unsigned char)*c, char_x, char_y, col, scale, inverted);
                last_x = char_x + font->width * scale;
            }
            if (last_x > largest_x) {
                largest_x = last_x;
            }
            if (*p == '\0') {
                break;
            }
            line_start = p + 1;
            char_y += font->height * scale;
        }
    }
    return gfx_area_from_rect(start_x, y0, largest_x - start_x, char_y + font->height * scale - y0);
}

#if GFX_ENABLE_HOST_STDIO
int gfx_font_export(const gfx_font_t *font, const char *filename) {
    /* Mirror Python Font.export: dump font->data to a .py file with a single
     * bytes object named _FONT (256 lines, one per character) and a trailing
     * FONT = memoryview(_FONT). Requires cached data (font->data). */
    if (!font->data) {
        return -1;
    }
    FILE *f = fopen(filename, "w");
    if (!f) {
        return -1;
    }
    fputs("_FONT =\\\n", f);
    for (int i = 0; i < 256; i++) {
        fputs("b'", f);
        for (int j = 0; j < font->height; j++) {
            size_t off = (size_t)i * (size_t)font->height + (size_t)j;
            uint8_t b = off < font->data_len ? font->data[off] : 0;
            fprintf(f, "\\x%02x", b);
        }
        fputs("'\\\n", f);
    }
    fputs("\nFONT = memoryview(_FONT)\n", f);
    fclose(f);
    return 0;
}
#endif /* GFX_ENABLE_HOST_STDIO */

gfx_area_t gfx_font_text8(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col) {
    ensure_defaults();
    return gfx_font_text(canvas, &default_font8, str, x0, y0, col, 1, 0);
}

gfx_area_t gfx_font_text14(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col) {
    ensure_defaults();
    return gfx_font_text(canvas, &default_font14, str, x0, y0, col, 1, 0);
}

gfx_area_t gfx_font_text16(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col) {
    ensure_defaults();
    return gfx_font_text(canvas, &default_font16, str, x0, y0, col, 1, 0);
}
