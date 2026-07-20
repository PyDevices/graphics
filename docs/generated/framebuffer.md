---
title: FrameBuffer
description: Native framebuffer construction, drawing, text, blitting, scrolling, and file output.
---

Source snapshot: [`68194959ad5bc5540aceb6b40807a8c4228af60a`](https://github.com/PyDevices/graphics/tree/68194959ad5bc5540aceb6b40807a8c4228af60a).

Native framebuffer construction, drawing, text, blitting, scrolling, and file output.

Every entry below is generated from a public binding table or header declaration and links to its immutable source line.

## `graphics.FrameBuffer`

```python
class FrameBuffer(buffer, width, height, format, stride=None)
```

A framebuf-compatible native drawing surface backed by a writable buffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1112)

## `graphics.FrameBuffer.from_file`

```python
FrameBuffer.from_file(path)
```

Load a supported image file into a new FrameBuffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1082)

## `graphics.FrameBuffer.fill`

```python
FrameBuffer.fill(color)
```

Fill the target canvas with one color and return the affected Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1083)

## `graphics.FrameBuffer.fill_rect`

```python
FrameBuffer.fill_rect(x, y, width, height, color)
```

Fill a rectangular region and return the clipped affected Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1084)

## `graphics.FrameBuffer.pixel`

```python
FrameBuffer.pixel(x, y, color=None)
```

Read or write one pixel, depending on whether a color is supplied.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1085)

## `graphics.FrameBuffer.hline`

```python
FrameBuffer.hline(x, y, width, color)
```

Draw a horizontal line.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1086)

## `graphics.FrameBuffer.vline`

```python
FrameBuffer.vline(x, y, height, color)
```

Draw a vertical line.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1087)

## `graphics.FrameBuffer.rect`

```python
FrameBuffer.rect(x, y, width, height, color, fill=False)
```

Draw an outlined or filled rectangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1088)

## `graphics.FrameBuffer.round_rect`

```python
FrameBuffer.round_rect(x, y, width, height, radius, color, fill=False)
```

Draw an outlined or filled rounded rectangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1089)

## `graphics.FrameBuffer.circle`

```python
FrameBuffer.circle(x, y, radius, color, fill=False)
```

Draw an outlined or filled circle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1090)

## `graphics.FrameBuffer.line`

```python
FrameBuffer.line(x1, y1, x2, y2, color)
```

Draw a line between two points.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1091)

## `graphics.FrameBuffer.ellipse`

```python
FrameBuffer.ellipse(x, y, x_radius, y_radius, color, fill=False, mask=0x0F)
```

Draw selected quadrants of an outlined or filled ellipse.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1092)

## `graphics.FrameBuffer.poly`

```python
FrameBuffer.poly(x, y, coordinates, color, fill=False)
```

Draw a polygon from a packed coordinate buffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1093)

## `graphics.FrameBuffer.arc`

```python
FrameBuffer.arc(x, y, radius, start_angle, end_angle, color)
```

Draw a circular arc between two angles.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1094)

## `graphics.FrameBuffer.triangle`

```python
FrameBuffer.triangle(x0, y0, x1, y1, x2, y2, color, fill=False)
```

Draw an outlined or filled triangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1095)

## `graphics.FrameBuffer.gradient_rect`

```python
FrameBuffer.gradient_rect(x, y, width, height, color1, color2=None, vertical=True)
```

Fill a rectangle with a horizontal or vertical color gradient.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1096)

## `graphics.FrameBuffer.polygon`

```python
FrameBuffer.polygon(points, x, y, color, angle=0, center_x=0, center_y=0)
```

Draw a translated and optionally rotated sequence of points.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1097)

## `graphics.FrameBuffer.blit`

```python
FrameBuffer.blit(source, x, y, key=-1, palette=None)
```

Copy another framebuffer onto the target with optional keying and palette lookup.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1098)

## `graphics.FrameBuffer.blit_rect`

```python
FrameBuffer.blit_rect(buffer, x, y, width, height)
```

Copy a packed RGB565 rectangle onto the target.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1099)

## `graphics.FrameBuffer.blit_transparent`

```python
FrameBuffer.blit_transparent(buffer, x, y, width, height, key)
```

Copy a packed RGB565 rectangle while skipping the transparent key.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1100)

## `graphics.FrameBuffer.text`

```python
FrameBuffer.text(text, x, y, color=1, scale=1, inverted=False, font_data=None, height=8)
```

Render bitmap text with selectable height, scale, and inversion.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1101)

## `graphics.FrameBuffer.text8`

```python
FrameBuffer.text8(text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 8-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1102)

## `graphics.FrameBuffer.text14`

```python
FrameBuffer.text14(text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 14-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1103)

## `graphics.FrameBuffer.text16`

```python
FrameBuffer.text16(text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 16-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1104)

## `graphics.FrameBuffer.scroll`

```python
FrameBuffer.scroll(x_step, y_step)
```

Move framebuffer contents by the requested x and y offsets.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1105)

## `graphics.FrameBuffer.save`

```python
FrameBuffer.save(path='screenshot')
```

Write the current image or bitmap to a versioned output path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1106)

## `graphics.FrameBuffer.width`

```python
FrameBuffer.width
```

Width in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1073)

## `graphics.FrameBuffer.height`

```python
FrameBuffer.height
```

Height in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1074)

## `graphics.FrameBuffer.buffer`

```python
FrameBuffer.buffer
```

Backing pixel buffer when the object owns or exposes one.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1075)

## `graphics.FrameBuffer.format`

```python
FrameBuffer.format
```

Framebuffer pixel-format identifier.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1076)

## `graphics.FrameBuffer.color_depth`

```python
FrameBuffer.color_depth
```

Bits per pixel for the framebuffer format.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L1077)
