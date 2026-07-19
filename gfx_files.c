/*
 * Image codec helpers (pure, I/O-free).
 *
 * Decodes/encodes PBM (P4), PGM (P5) and RGB565 BMP (BM) images entirely in
 * memory. No fopen/malloc here: callers supply the source bytes and the
 * destination buffer, so the same code links and runs on every port.
 *
 * SPDX-License-Identifier: MIT
 */

#include "gfx_files.h"

#include <string.h>

#include "gfx_bmp565.h"
#include "gfx_core.h"
#include "gfx_framebuffer.h"

/* --- little-endian scalar reads --------------------------------------- */

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

/* --- tiny numeric helpers (no libc string/number deps) ---------------- */

/* Number of decimal digits needed to print a non-negative value. */
static size_t dec_digits(unsigned long v) {
    size_t n = 1;
    while (v >= 10) {
        v /= 10;
        n++;
    }
    return n;
}

/* Write decimal `v` at dst[i..], returning the new index. Caller guarantees
 * room (sized via dec_digits()). */
static size_t put_uint(uint8_t *dst, size_t i, unsigned long v) {
    size_t digits = dec_digits(v);
    size_t end = i + digits;
    for (size_t k = end; k > i; k--) {
        dst[k - 1] = (uint8_t)('0' + (v % 10));
        v /= 10;
    }
    return end;
}

/* --- PNM header parsing (memory) -------------------------------------- */

typedef struct {
    const uint8_t *p;
    size_t len;
    size_t i;
} rbuf_t;

/* Read one PNM header line (excluding newline), skipping '#' comment lines.
 * Advances past the terminating newline. Returns 0 on success, -1 on EOF. */
static int rbuf_line(rbuf_t *r, char *out, size_t cap) {
    for (;;) {
        if (r->i >= r->len) {
            return -1;
        }
        size_t j = 0;
        while (r->i < r->len && r->p[r->i] != '\n') {
            if (j + 1 < cap) {
                out[j++] = (char)r->p[r->i];
            }
            r->i++;
        }
        if (r->i < r->len) {
            r->i++; /* consume '\n' */
        }
        out[j] = '\0';
        if (j > 0 && out[0] == '#') {
            continue;
        }
        return 0;
    }
}

/* Parse one non-negative int from `s`, skipping leading spaces. On success
 * returns 0, sets *out and advances *s past the digits. */
static int parse_uint(const char **s, int *out) {
    const char *p = *s;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p < '0' || *p > '9') {
        return -1;
    }
    long v = 0;
    while (*p >= '0' && *p <= '9') {
        v = v * 10 + (*p - '0');
        p++;
    }
    *out = (int)v;
    *s = p;
    return 0;
}

/* --- probe ------------------------------------------------------------ */

int gfx_image_probe(const uint8_t *data, size_t len, gfx_image_info_t *info) {
    if (!data || len < 3) {
        return -1;
    }

    if (data[0] == 'P' && (data[1] == '4' || data[1] == '5') && data[2] == '\n') {
        rbuf_t r = { data, len, 3 };
        char line[128];
        if (rbuf_line(&r, line, sizeof(line)) < 0) {
            return -1;
        }
        const char *s = line;
        int w, h;
        if (parse_uint(&s, &w) < 0 || parse_uint(&s, &h) < 0 || w <= 0 || h <= 0) {
            return -1;
        }
        if (data[1] == '4') { /* PBM: 1bpp packed rows */
            info->width = w;
            info->height = h;
            info->format = GFX_MHLSB;
            info->buffer_len = (size_t)((w + 7) / 8) * (size_t)h;
            info->src_offset = r.i;
            return 0;
        }
        /* PGM: maxval selects greyscale packing */
        if (rbuf_line(&r, line, sizeof(line)) < 0) {
            return -1;
        }
        s = line;
        int maxval;
        if (parse_uint(&s, &maxval) < 0) {
            return -1;
        }
        int format;
        size_t buf_len;
        if (maxval == 3) {
            format = GFX_GS2_HMSB;
            buf_len = (size_t)((w + 3) / 4) * (size_t)h;
        } else if (maxval == 15) {
            format = GFX_GS4_HMSB;
            buf_len = (size_t)((w + 1) / 2) * (size_t)h;
        } else if (maxval == 255) {
            format = GFX_GS8;
            buf_len = (size_t)w * (size_t)h;
        } else {
            return -1;
        }
        info->width = w;
        info->height = h;
        info->format = format;
        info->buffer_len = buf_len;
        info->src_offset = r.i;
        return 0;
    }

    if (data[0] == 'B' && data[1] == 'M') {
        if (len < 54) {
            return -1;
        }
        uint32_t off = rd32(data + 10);
        uint32_t w = rd32(data + 18);
        uint32_t h = rd32(data + 22);
        uint16_t planes = rd16(data + 26);
        uint16_t bpp = rd16(data + 28);
        if (planes != 1 || bpp != GFX_BMP565_BPP || w == 0 || h == 0) {
            return -1;
        }
        info->width = (int)w;
        info->height = (int)h;
        info->format = GFX_RGB565;
        info->buffer_len = (size_t)w * (size_t)h * GFX_BMP565_BYTES_PER_PIXEL;
        info->src_offset = off;
        return 0;
    }

    return -1;
}

/* --- decode ----------------------------------------------------------- */

int gfx_image_decode(const uint8_t *data, size_t len, const gfx_image_info_t *info,
    uint8_t *dst, size_t dst_len) {
    if (!data || !info || !dst || dst_len < info->buffer_len) {
        return -1;
    }

    if (info->format == GFX_RGB565) {
        /* BMP rows are stored bottom-up; flip into top-down framebuffer order. */
        size_t row_bytes = (size_t)info->width * GFX_BMP565_BYTES_PER_PIXEL;
        for (int row = 0; row < info->height; row++) {
            size_t src = info->src_offset + (size_t)(info->height - row - 1) * row_bytes;
            if (src + row_bytes > len) {
                return -1;
            }
            memcpy(dst + (size_t)row * row_bytes, data + src, row_bytes);
        }
        return 0;
    }

    /* PBM/PGM store rows top-down and contiguous. */
    if (info->src_offset + info->buffer_len > len) {
        return -1;
    }
    memcpy(dst, data + info->src_offset, info->buffer_len);
    return 0;
}

/* --- encode ----------------------------------------------------------- */

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

/* Pixel-data byte length for a framebuffer in a supported format. */
static size_t pixel_bytes(const gfx_fb_t *fb) {
    switch (fb->format) {
        case GFX_MHLSB:
            return (size_t)((fb->width + 7) / 8) * (size_t)fb->height;
        case GFX_GS2_HMSB:
            return (size_t)((fb->width + 3) / 4) * (size_t)fb->height;
        case GFX_GS4_HMSB:
            return (size_t)((fb->width + 1) / 2) * (size_t)fb->height;
        case GFX_GS8:
            return (size_t)fb->width * (size_t)fb->height;
        case GFX_RGB565:
            return (size_t)fb->width * (size_t)fb->height * GFX_BMP565_BYTES_PER_PIXEL;
        default:
            return 0;
    }
}

/* PNM header length: "Pn\n" + "<w> <h>\n" [+ "<maxval>\n" for PGM]. */
static size_t pnm_header_len(const gfx_fb_t *fb, int *maxval_out) {
    size_t hdr = 3 /* "Pn\n" */ + dec_digits((unsigned long)fb->width) + 1 /* space */
        + dec_digits((unsigned long)fb->height) + 1 /* newline */;
    int maxval = 0;
    if (fb->format == GFX_GS2_HMSB) {
        maxval = 3;
    } else if (fb->format == GFX_GS4_HMSB) {
        maxval = 15;
    } else if (fb->format == GFX_GS8) {
        maxval = 255;
    }
    if (maxval) {
        hdr += dec_digits((unsigned long)maxval) + 1;
    }
    if (maxval_out) {
        *maxval_out = maxval;
    }
    return hdr;
}

int gfx_image_encoded_size(const gfx_fb_t *fb, size_t *out_len, const char **out_ext) {
    const char *ext = ext_for_format(fb->format);
    if (!ext) {
        return -1;
    }
    size_t pixels = pixel_bytes(fb);
    size_t total;
    if (fb->format == GFX_RGB565) {
        total = 14 + 40 + pixels; /* BITMAPFILEHEADER + BITMAPINFOHEADER + data */
    } else {
        total = pnm_header_len(fb, NULL) + pixels;
    }
    if (out_len) {
        *out_len = total;
    }
    if (out_ext) {
        *out_ext = ext;
    }
    return 0;
}

int gfx_image_encode(const gfx_fb_t *fb, uint8_t *dst, size_t dst_len, size_t *out_len) {
    size_t total;
    const char *ext;
    if (gfx_image_encoded_size(fb, &total, &ext) < 0 || dst_len < total) {
        return -1;
    }
    size_t pixels = pixel_bytes(fb);

    if (fb->format == GFX_RGB565) {
        size_t data_off = 14 + 40;
        memset(dst, 0, data_off);
        dst[0] = 'B';
        dst[1] = 'M';
        /* file size */
        dst[2] = (uint8_t)(total);
        dst[3] = (uint8_t)(total >> 8);
        dst[4] = (uint8_t)(total >> 16);
        dst[5] = (uint8_t)(total >> 24);
        /* pixel data offset @10 */
        dst[10] = (uint8_t)data_off;
        /* DIB header size @14 = 40 */
        dst[14] = 40;
        /* width @18 */
        dst[18] = (uint8_t)(fb->width);
        dst[19] = (uint8_t)(fb->width >> 8);
        dst[20] = (uint8_t)(fb->width >> 16);
        dst[21] = (uint8_t)((unsigned)fb->width >> 24);
        /* height @22 */
        dst[22] = (uint8_t)(fb->height);
        dst[23] = (uint8_t)(fb->height >> 8);
        dst[24] = (uint8_t)(fb->height >> 16);
        dst[25] = (uint8_t)((unsigned)fb->height >> 24);
        /* planes @26 = 1, bpp @28 = 16 */
        dst[26] = 1;
        dst[28] = GFX_BMP565_BPP;
        /* image size @34 */
        dst[34] = (uint8_t)(pixels);
        dst[35] = (uint8_t)(pixels >> 8);
        dst[36] = (uint8_t)(pixels >> 16);
        dst[37] = (uint8_t)(pixels >> 24);

        size_t row_bytes = (size_t)fb->width * GFX_BMP565_BYTES_PER_PIXEL;
        const uint8_t *src = (const uint8_t *)fb->buf;
        size_t o = data_off;
        for (int row = fb->height - 1; row >= 0; row--) {
            memcpy(dst + o, src + (size_t)row * row_bytes, row_bytes);
            o += row_bytes;
        }
        if (out_len) {
            *out_len = total;
        }
        return 0;
    }

    /* PBM (P4) / PGM (P5) */
    int maxval;
    (void)pnm_header_len(fb, &maxval);
    size_t i = 0;
    dst[i++] = 'P';
    dst[i++] = maxval ? '5' : '4';
    dst[i++] = '\n';
    i = put_uint(dst, i, (unsigned long)fb->width);
    dst[i++] = ' ';
    i = put_uint(dst, i, (unsigned long)fb->height);
    dst[i++] = '\n';
    if (maxval) {
        i = put_uint(dst, i, (unsigned long)maxval);
        dst[i++] = '\n';
    }
    memcpy(dst + i, fb->buf, pixels);
    i += pixels;
    if (out_len) {
        *out_len = i;
    }
    return 0;
}

/* --- host C stdio wrappers (desktop / CPython) ------------------------ */

#if GFX_ENABLE_HOST_STDIO

#include <stdio.h>
#include <stdlib.h>

void gfx_image_fb_free(gfx_image_fb_t *img) {
    if (img->owns_buffer && img->buffer) {
        free(img->buffer);
    }
    img->buffer = NULL;
    img->buffer_len = 0;
    img->owns_buffer = 0;
}

/* Read an entire file into a freshly malloc'd buffer. Caller frees. */
static int read_whole_file(const char *path, uint8_t **out, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long n = ftell(f);
    if (n < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }
    uint8_t *buf = (uint8_t *)malloc((size_t)n ? (size_t)n : 1);
    if (!buf) {
        fclose(f);
        return -1;
    }
    if (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        return -1;
    }
    fclose(f);
    *out = buf;
    *out_len = (size_t)n;
    return 0;
}

/* Load `path`, optionally requiring a specific 2-byte magic (0 = any). */
static int load_image_magic(const char *path, gfx_image_fb_t *out, char m0, char m1) {
    uint8_t *file_buf;
    size_t file_len;
    if (read_whole_file(path, &file_buf, &file_len) < 0) {
        return -1;
    }
    if (m0 && (file_len < 2 || file_buf[0] != (uint8_t)m0 || file_buf[1] != (uint8_t)m1)) {
        free(file_buf);
        return -1;
    }
    gfx_image_info_t info;
    if (gfx_image_probe(file_buf, file_len, &info) < 0) {
        free(file_buf);
        return -1;
    }
    uint8_t *dst = (uint8_t *)malloc(info.buffer_len ? info.buffer_len : 1);
    if (!dst) {
        free(file_buf);
        return -1;
    }
    if (gfx_image_decode(file_buf, file_len, &info, dst, info.buffer_len) < 0) {
        free(dst);
        free(file_buf);
        return -1;
    }
    free(file_buf);
    out->buffer = dst;
    out->buffer_len = info.buffer_len;
    out->width = info.width;
    out->height = info.height;
    out->format = info.format;
    out->owns_buffer = 1;
    return 0;
}

int gfx_files_load_image(const char *path, gfx_image_fb_t *out) {
    return load_image_magic(path, out, 0, 0);
}

int gfx_files_bmp_to_framebuffer(const char *path, gfx_image_fb_t *out) {
    return load_image_magic(path, out, 'B', 'M');
}

int gfx_files_pbm_to_framebuffer(const char *path, gfx_image_fb_t *out) {
    return load_image_magic(path, out, 'P', '4');
}

int gfx_files_pgm_to_framebuffer(const char *path, gfx_image_fb_t *out) {
    return load_image_magic(path, out, 'P', '5');
}

int gfx_files_save_image(const gfx_fb_t *fb, const char *path, char *out_path, size_t out_path_len) {
    size_t enc_len;
    const char *ext;
    if (gfx_image_encoded_size(fb, &enc_len, &ext) < 0) {
        return -1;
    }
    size_t plen = strlen(path);
    size_t elen = strlen(ext);
    int has_ext = (plen > elen + 1 && path[plen - elen - 1] == '.'
        && memcmp(path + plen - elen, ext, elen) == 0);
    size_t need = has_ext ? plen : plen + 1 + elen;
    if (need + 1 > out_path_len) {
        return -1;
    }
    memcpy(out_path, path, plen);
    if (has_ext) {
        out_path[plen] = '\0';
    } else {
        out_path[plen] = '.';
        memcpy(out_path + plen + 1, ext, elen);
        out_path[plen + 1 + elen] = '\0';
    }

    uint8_t *enc = (uint8_t *)malloc(enc_len ? enc_len : 1);
    if (!enc) {
        return -1;
    }
    size_t written = 0;
    if (gfx_image_encode(fb, enc, enc_len, &written) < 0) {
        free(enc);
        return -1;
    }
    FILE *f = fopen(out_path, "wb");
    if (!f) {
        free(enc);
        return -1;
    }
    size_t ok = fwrite(enc, 1, written, f);
    fclose(f);
    free(enc);
    return ok == written ? 0 : -1;
}

#endif /* GFX_ENABLE_HOST_STDIO */
