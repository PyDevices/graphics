---
title: Drawing operations
description: Object-oriented and module-level shape, text, polygon, and blit operations.
---

Source snapshot: [`68194959ad5bc5540aceb6b40807a8c4228af60a`](https://github.com/PyDevices/graphics/tree/68194959ad5bc5540aceb6b40807a8c4228af60a).

Object-oriented and module-level shape, text, polygon, and blit operations.

Every entry below is generated from a public binding table or header declaration and links to its immutable source line.

## `graphics.Draw`

```python
class Draw(canvas)
```

Drawing facade that adds a bounded clip stack to any compatible canvas.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2085)

## `graphics.Draw.fill`

```python
Draw.fill(color)
```

Fill the target canvas with one color and return the affected Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2057)

## `graphics.Draw.fill_rect`

```python
Draw.fill_rect(x, y, width, height, color)
```

Fill a rectangular region and return the clipped affected Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2058)

## `graphics.Draw.pixel`

```python
Draw.pixel(x, y, color=None)
```

Read or write one pixel, depending on whether a color is supplied.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2059)

## `graphics.Draw.hline`

```python
Draw.hline(x, y, width, color)
```

Draw a horizontal line.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2060)

## `graphics.Draw.vline`

```python
Draw.vline(x, y, height, color)
```

Draw a vertical line.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2061)

## `graphics.Draw.line`

```python
Draw.line(x1, y1, x2, y2, color)
```

Draw a line between two points.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2062)

## `graphics.Draw.rect`

```python
Draw.rect(x, y, width, height, color, fill=False)
```

Draw an outlined or filled rectangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2063)

## `graphics.Draw.round_rect`

```python
Draw.round_rect(x, y, width, height, radius, color, fill=False)
```

Draw an outlined or filled rounded rectangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2064)

## `graphics.Draw.circle`

```python
Draw.circle(x, y, radius, color, fill=False)
```

Draw an outlined or filled circle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2065)

## `graphics.Draw.ellipse`

```python
Draw.ellipse(x, y, x_radius, y_radius, color, fill=False, mask=0x0F)
```

Draw selected quadrants of an outlined or filled ellipse.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2066)

## `graphics.Draw.arc`

```python
Draw.arc(x, y, radius, start_angle, end_angle, color)
```

Draw a circular arc between two angles.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2067)

## `graphics.Draw.triangle`

```python
Draw.triangle(x0, y0, x1, y1, x2, y2, color, fill=False)
```

Draw an outlined or filled triangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2068)

## `graphics.Draw.gradient_rect`

```python
Draw.gradient_rect(x, y, width, height, color1, color2=None, vertical=True)
```

Fill a rectangle with a horizontal or vertical color gradient.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2069)

## `graphics.Draw.poly`

```python
Draw.poly(x, y, coordinates, color, fill=False)
```

Draw a polygon from a packed coordinate buffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2070)

## `graphics.Draw.polygon`

```python
Draw.polygon(points, x, y, color, angle=0, center_x=0, center_y=0)
```

Draw a translated and optionally rotated sequence of points.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2071)

## `graphics.Draw.blit`

```python
Draw.blit(source, x, y, key=-1, palette=None)
```

Copy another framebuffer onto the target with optional keying and palette lookup.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2072)

## `graphics.Draw.blit_rect`

```python
Draw.blit_rect(buffer, x, y, width, height)
```

Copy a packed RGB565 rectangle onto the target.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2073)

## `graphics.Draw.blit_transparent`

```python
Draw.blit_transparent(buffer, x, y, width, height, key)
```

Copy a packed RGB565 rectangle while skipping the transparent key.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2074)

## `graphics.Draw.text`

```python
Draw.text(text, x, y, color=1, scale=1, inverted=False, font_data=None, height=8)
```

Render bitmap text with selectable height, scale, and inversion.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2075)

## `graphics.Draw.text8`

```python
Draw.text8(text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 8-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2076)

## `graphics.Draw.text14`

```python
Draw.text14(text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 14-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2077)

## `graphics.Draw.text16`

```python
Draw.text16(text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 16-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2078)

## `graphics.Draw.clip`

```python
Draw.clip(area_or_x, y=None, width=None, height=None)
```

Return the intersection with another Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2079)

## `graphics.fill`

```python
graphics.fill(canvas, color)
```

Fill the target canvas with one color and return the affected Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2953)

## `graphics.fill_rect`

```python
graphics.fill_rect(canvas, x, y, width, height, color)
```

Fill a rectangular region and return the clipped affected Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2954)

## `graphics.pixel`

```python
graphics.pixel(canvas, x, y, color=None)
```

Read or write one pixel, depending on whether a color is supplied.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2955)

## `graphics.hline`

```python
graphics.hline(canvas, x, y, width, color)
```

Draw a horizontal line.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2956)

## `graphics.vline`

```python
graphics.vline(canvas, x, y, height, color)
```

Draw a vertical line.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2957)

## `graphics.line`

```python
graphics.line(canvas, x1, y1, x2, y2, color)
```

Draw a line between two points.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2958)

## `graphics.rect`

```python
graphics.rect(canvas, x, y, width, height, color, fill=False)
```

Draw an outlined or filled rectangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2959)

## `graphics.round_rect`

```python
graphics.round_rect(canvas, x, y, width, height, radius, color, fill=False)
```

Draw an outlined or filled rounded rectangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2960)

## `graphics.circle`

```python
graphics.circle(canvas, x, y, radius, color, fill=False)
```

Draw an outlined or filled circle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2961)

## `graphics.ellipse`

```python
graphics.ellipse(canvas, x, y, x_radius, y_radius, color, fill=False, mask=0x0F)
```

Draw selected quadrants of an outlined or filled ellipse.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2962)

## `graphics.arc`

```python
graphics.arc(canvas, x, y, radius, start_angle, end_angle, color)
```

Draw a circular arc between two angles.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2963)

## `graphics.triangle`

```python
graphics.triangle(canvas, x0, y0, x1, y1, x2, y2, color, fill=False)
```

Draw an outlined or filled triangle.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2964)

## `graphics.gradient_rect`

```python
graphics.gradient_rect(canvas, x, y, width, height, color1, color2=None, vertical=True)
```

Fill a rectangle with a horizontal or vertical color gradient.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2965)

## `graphics.poly`

```python
graphics.poly(canvas, x, y, coordinates, color, fill=False)
```

Draw a polygon from a packed coordinate buffer.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2966)

## `graphics.blit`

```python
graphics.blit(canvas, source, x, y, key=-1, palette=None)
```

Copy another framebuffer onto the target with optional keying and palette lookup.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2967)

## `graphics.blit_rect`

```python
graphics.blit_rect(canvas, buffer, x, y, width, height)
```

Copy a packed RGB565 rectangle onto the target.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2968)

## `graphics.blit_transparent`

```python
graphics.blit_transparent(canvas, buffer, x, y, width, height, key)
```

Copy a packed RGB565 rectangle while skipping the transparent key.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2969)

## `graphics.polygon`

```python
graphics.polygon(canvas, points, x, y, color, angle=0, center_x=0, center_y=0)
```

Draw a translated and optionally rotated sequence of points.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2970)

## `graphics.text`

```python
graphics.text(canvas, text, x, y, color=1, scale=1, inverted=False, font_data=None, height=8)
```

Render bitmap text with selectable height, scale, and inversion.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2976)

## `graphics.text8`

```python
graphics.text8(canvas, text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 8-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2977)

## `graphics.text14`

```python
graphics.text14(canvas, text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 14-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2978)

## `graphics.text16`

```python
graphics.text16(canvas, text, x, y, color=1, scale=1, inverted=False, font_data=None)
```

Render text with the built-in 16-pixel-high font path.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2979)
