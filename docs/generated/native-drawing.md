---
title: Native drawing and framebuffer API
description: Public C declarations for geometry, framebuffer operations, shapes, and clipping.
---

Source snapshot: [`68194959ad5bc5540aceb6b40807a8c4228af60a`](https://github.com/PyDevices/graphics/tree/68194959ad5bc5540aceb6b40807a8c4228af60a).

Public C declarations for geometry, framebuffer operations, shapes, and clipping.

Every entry below is generated from a public binding table or header declaration and links to its immutable source line.

## `gfx_fb_validate_buffer`

```c
int gfx_fb_validate_buffer(size_t buf_len, int width, int height, int format, int stride);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L21)

## `gfx_fb_color_depth`

```c
int gfx_fb_color_depth(int format);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L22)

## `gfx_fb_setpixel`

```c
void gfx_fb_setpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y, uint32_t col);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L24)

## `gfx_fb_getpixel`

```c
uint32_t gfx_fb_getpixel(const gfx_fb_t *fb, unsigned int x, unsigned int y);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L25)

## `gfx_fb_setpixel_checked`

```c
void gfx_fb_setpixel_checked(const gfx_fb_t *fb, int x, int y, int col, int mask);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L26)

## `gfx_fb_fill_rect_raw`

```c
void gfx_fb_fill_rect_raw(const gfx_fb_t *fb, int x, int y, int w, int h, uint32_t col);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L27)

## `gfx_fb_fill_rect`

```c
gfx_area_t gfx_fb_fill_rect(const gfx_fb_t *fb, int x, int y, int w, int h, uint32_t col);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L28)

## `gfx_fb_fill`

```c
gfx_area_t gfx_fb_fill(const gfx_fb_t *fb, uint32_t col);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L29)

## `gfx_fb_canvas_init`

```c
void gfx_fb_canvas_init(gfx_canvas_t *canvas, const gfx_fb_t *fb);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L31)

## `gfx_fb_scroll`

```c
void gfx_fb_scroll(gfx_fb_t *fb, int xstep, int ystep);
```

Native fb entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_framebuffer.h#L32)

## `gfx_clipped_canvas_init`

```c
void gfx_clipped_canvas_init(gfx_clipped_canvas_t *cc, const gfx_canvas_t *parent, const gfx_area_t *clip);
```

Native clipped entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L20)

## `gfx_shapes_pixel`

```c
gfx_area_t gfx_shapes_pixel(const gfx_canvas_t *canvas, int x, int y, int c);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L22)

## `gfx_shapes_hline`

```c
gfx_area_t gfx_shapes_hline(const gfx_canvas_t *canvas, int x, int y, int w, int c);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L23)

## `gfx_shapes_vline`

```c
gfx_area_t gfx_shapes_vline(const gfx_canvas_t *canvas, int x, int y, int h, int c);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L24)

## `gfx_shapes_fill_rect`

```c
gfx_area_t gfx_shapes_fill_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L25)

## `gfx_shapes_rect`

```c
gfx_area_t gfx_shapes_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c, int fill);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L26)

## `gfx_shapes_line`

```c
gfx_area_t gfx_shapes_line(const gfx_canvas_t *canvas, int x1, int y1, int x2, int y2, int c);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L27)

## `gfx_shapes_ellipse`

```c
gfx_area_t gfx_shapes_ellipse(const gfx_canvas_t *canvas, int cx, int cy, int xradius, int yradius, int col, int fill, int mask_part);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L28)

## `gfx_shapes_fill`

```c
gfx_area_t gfx_shapes_fill(const gfx_canvas_t *canvas, int c);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L29)

## `gfx_shapes_poly_int_from_buffer`

```c
int gfx_shapes_poly_int_from_buffer(const void *buf, size_t len, size_t itemsize, const char *fmt, size_t index, int *out);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L31)

## `gfx_shapes_poly`

```c
gfx_area_t gfx_shapes_poly(const gfx_canvas_t *canvas, int x, int y, const void *coords, size_t coords_len, size_t itemsize, const char *fmt, int col, int fill);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L32)

## `gfx_shapes_blit`

```c
gfx_area_t gfx_shapes_blit(const gfx_canvas_t *canvas, const gfx_fb_t *source, int x, int y, int key, const gfx_fb_t *palette);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L33)

## `gfx_shapes_circle`

```c
gfx_area_t gfx_shapes_circle(const gfx_canvas_t *canvas, int x0, int y0, int r, int c, int fill);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L34)

## `gfx_shapes_round_rect`

```c
gfx_area_t gfx_shapes_round_rect(const gfx_canvas_t *canvas, int x0, int y0, int w, int h, int r, int c, int fill);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L35)

## `gfx_shapes_triangle`

```c
gfx_area_t gfx_shapes_triangle(const gfx_canvas_t *canvas, int x0, int y0, int x1, int y1, int x2, int y2, int c, int fill);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L36)

## `gfx_shapes_arc`

```c
gfx_area_t gfx_shapes_arc(const gfx_canvas_t *canvas, int x, int y, int r, float a0, float a1, int c);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L37)

## `gfx_shapes_gradient_rect`

```c
gfx_area_t gfx_shapes_gradient_rect(const gfx_canvas_t *canvas, int x, int y, int w, int h, int c1, int c2, int vertical);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L38)

## `gfx_shapes_blit_rect`

```c
gfx_area_t gfx_shapes_blit_rect(const gfx_canvas_t *canvas, const void *buf, int x, int y, int w, int h, int bpp);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L39)

## `gfx_shapes_blit_transparent`

```c
gfx_area_t gfx_shapes_blit_transparent(const gfx_canvas_t *canvas, const void *buf, int x, int y, int w, int h, int key, int bpp);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L40)

## `gfx_shapes_polygon`

```c
gfx_area_t gfx_shapes_polygon(const gfx_canvas_t *canvas, const int *points, size_t n_points, int x, int y, int color, float angle, int center_x, int center_y);
```

Native shapes entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_shapes.h#L41)

## `gfx_draw_init`

```c
void gfx_draw_init(gfx_draw_t *draw, const gfx_canvas_t *canvas);
```

Native draw entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_draw.h#L21)

## `gfx_draw_effective_clip`

```c
gfx_area_t gfx_draw_effective_clip(const gfx_draw_t *draw);
```

Native draw entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_draw.h#L23)

## `gfx_draw_push_clip`

```c
void gfx_draw_push_clip(gfx_draw_t *draw, const gfx_area_t *clip);
```

Native draw entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_draw.h#L24)

## `gfx_draw_pop_clip`

```c
void gfx_draw_pop_clip(gfx_draw_t *draw);
```

Native draw entry point declared for firmware and extension integration.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_draw.h#L25)
