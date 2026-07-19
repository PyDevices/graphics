---
title: Fonts and images
description: Bitmap font rendering, RGB565 bitmap access, and image conversion helpers.
---

Source snapshot: [`68194959ad5bc5540aceb6b40807a8c4228af60a`](https://github.com/PyDevices/graphics/tree/68194959ad5bc5540aceb6b40807a8c4228af60a).

Bitmap font rendering, RGB565 bitmap access, and image conversion helpers.

Every entry below is generated from a public binding table or header declaration and links to its immutable source line.

## `graphics.Font`

```python
class Font(font_data=None, height=0, cached=True)
```

Bitmap font loader and renderer for built-in, file, or buffer-backed glyph data.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2643)

## `graphics.Font.text`

```python
Font.text(canvas, text, x, y, color, scale=1, inverted=False)
```

Render bitmap text with selectable height, scale, and inversion.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2633)

## `graphics.Font.draw_char`

```python
Font.draw_char(char, x, y, canvas, color, scale=1, inverted=False)
```

Render one glyph through this Font onto a canvas.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2634)

## `graphics.Font.text_width`

```python
Font.text_width(text, scale=1)
```

Measure rendered text width at the selected scale.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2635)

## `graphics.Font.deinit`

```python
Font.deinit()
```

Release resources owned by the object.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2636)

## `graphics.Font.export`

```python
Font.export(filename)
```

Write cached font bytes to a file.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2637)

## `graphics.Font.width`

```python
Font.width
```

Width in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2626)

## `graphics.Font.height`

```python
Font.height
```

Height in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2627)

## `graphics.Font.font_name`

```python
Font.font_name
```

Font source name or the in-memory/default identifier.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2628)

## `graphics.BMP565`

```python
class BMP565(filename=None, source=None, streamed=False, mirrored=False, width=None, height=None)
```

RGB565 bitmap reader supporting buffered and streamed access.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2934)

## `graphics.BMP565.save`

```python
BMP565.save(filename=None)
```

Write the current image or bitmap to a versioned output path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2923)

## `graphics.BMP565.deinit`

```python
BMP565.deinit()
```

Release resources owned by the object.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2924)

## `graphics.BMP565.width`

```python
BMP565.width
```

Width in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2914)

## `graphics.BMP565.height`

```python
BMP565.height
```

Height in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2915)

## `graphics.BMP565.buffer`

```python
BMP565.buffer
```

Backing pixel buffer when the object owns or exposes one.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2916)

## `graphics.BMP565.bpp`

```python
BMP565.bpp
```

Bitmap color depth in bits per pixel.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2917)

## `graphics.BMP565.BPP`

```python
BMP565.BPP
```

Bitmap storage width in bytes per pixel.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2918)

## `graphics.load_image`

```python
graphics.load_image(path)
```

Load a supported image file as a FrameBuffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2971)

## `graphics.save_image`

```python
graphics.save_image(framebuffer, path='screenshot')
```

Save a FrameBuffer to a versioned image path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2972)

## `graphics.bmp_to_framebuffer`

```python
graphics.bmp_to_framebuffer(path)
```

Decode a BMP file into a FrameBuffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2973)

## `graphics.pbm_to_framebuffer`

```python
graphics.pbm_to_framebuffer(path)
```

Decode a PBM file into a FrameBuffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2974)

## `graphics.pgm_to_framebuffer`

```python
graphics.pgm_to_framebuffer(path)
```

Decode a PGM file into a FrameBuffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2975)
