---
title: Native fonts, images, and runtime API
description: Public C declarations for fonts, BMP565 data, image conversion, and capability reporting.
---

Source snapshot: [`aa6041e9226bdf5172a656eef722b852ec02f270`](https://github.com/PyDevices/graphics/tree/aa6041e9226bdf5172a656eef722b852ec02f270).

Public C declarations for fonts, BMP565 data, image conversion, and capability reporting.

Every entry below is generated from a public binding table or header declaration and links to its immutable source line.

## `gfx_font_init_default`

```c
void gfx_font_init_default(gfx_font_t *font, int height);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L22)

## `gfx_font_init_from_data`

```c
void gfx_font_init_from_data(gfx_font_t *font, const uint8_t *data, size_t len, int height);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L23)

## `gfx_font_deinit`

```c
void gfx_font_deinit(gfx_font_t *font);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L24)

## `gfx_font_export`

```c
int gfx_font_export(const gfx_font_t *font, const char *filename);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L26)

## `gfx_font_text`

```c
gfx_area_t gfx_font_text(const gfx_canvas_t *canvas, const gfx_font_t *font, const char *str, int x0, int y0, int col, int scale, int inverted);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L29)

## `gfx_font_draw_char`

```c
gfx_area_t gfx_font_draw_char(const gfx_canvas_t *canvas, const gfx_font_t *font, unsigned char ch, int x, int y, int color, int scale, int inverted);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L30)

## `gfx_font_text_width`

```c
int gfx_font_text_width(const gfx_font_t *font, const char *str, int scale);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L31)

## `gfx_font_text8`

```c
gfx_area_t gfx_font_text8(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L32)

## `gfx_font_text14`

```c
gfx_area_t gfx_font_text14(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L33)

## `gfx_font_text16`

```c
gfx_area_t gfx_font_text16(const gfx_canvas_t *canvas, const char *str, int x0, int y0, int col);
```

Native font entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_font.h#L34)

## `gfx_bmp565_read_header_from_file`

```c
int gfx_bmp565_read_header_from_file(const char *path, int *width, int *height, unsigned int *data_offset);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L27)

## `gfx_bmp565_open_stream`

```c
int gfx_bmp565_open_stream(const char *path, gfx_bmp565_t *out);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L28)

## `gfx_bmp565_load_from_file`

```c
int gfx_bmp565_load_from_file(const char *path, gfx_bmp565_t *out);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L29)

## `gfx_bmp565_init_from_buffer`

```c
int gfx_bmp565_init_from_buffer(gfx_bmp565_t *out, const uint8_t *buf, size_t len, int width, int height);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L30)

## `gfx_bmp565_deinit`

```c
void gfx_bmp565_deinit(gfx_bmp565_t *bmp);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L31)

## `gfx_bmp565_save`

```c
int gfx_bmp565_save(const gfx_bmp565_t *bmp, const char *path);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L32)

## `gfx_bmp565_save_versioned`

```c
int gfx_bmp565_save_versioned(const gfx_bmp565_t *bmp, const char *path, char *out_path, size_t out_path_len);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L33)

## `gfx_bmp565_read_bytes`

```c
int gfx_bmp565_read_bytes(const gfx_bmp565_t *bmp, int start, int stop, uint8_t *out, size_t out_cap, size_t *out_len);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L34)

## `gfx_bmp565_read_region`

```c
int gfx_bmp565_read_region(const gfx_bmp565_t *bmp, int x0, int x1, int y0, int y1, uint8_t *out, size_t out_cap, size_t *out_len);
```

Native bmp565 entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_bmp565.h#L35)

## `gfx_image_probe`

```c
int gfx_image_probe(const uint8_t *data, size_t len, gfx_image_info_t *info);
```

Native image entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_files.h#L43)

## `gfx_image_decode`

```c
int gfx_image_decode(const uint8_t *data, size_t len, const gfx_image_info_t *info, uint8_t *dst, size_t dst_len);
```

Native image entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_files.h#L47)

## `gfx_image_fb_free`

```c
void gfx_image_fb_free(gfx_image_fb_t *img);
```

Native image entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_files.h#L64)

## `gfx_files_load_image`

```c
int gfx_files_load_image(const char *path, gfx_image_fb_t *out);
```

Native files entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_files.h#L65)

## `gfx_files_save_image`

```c
int gfx_files_save_image(const gfx_fb_t *fb, const char *path, char *out_path, size_t out_path_len);
```

Native files entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_files.h#L66)

## `gfx_files_pbm_to_framebuffer`

```c
int gfx_files_pbm_to_framebuffer(const char *path, gfx_image_fb_t *out);
```

Native files entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_files.h#L67)

## `gfx_files_pgm_to_framebuffer`

```c
int gfx_files_pgm_to_framebuffer(const char *path, gfx_image_fb_t *out);
```

Native files entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_files.h#L68)

## `gfx_files_bmp_to_framebuffer`

```c
int gfx_files_bmp_to_framebuffer(const char *path, gfx_image_fb_t *out);
```

Native files entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/aa6041e9226bdf5172a656eef722b852ec02f270/gfx_files.h#L69)
