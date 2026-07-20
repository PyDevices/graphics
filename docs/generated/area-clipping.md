---
title: Area and clipping
description: Rectangle geometry, scoped clip contexts, and clipped canvas operations.
---

Source snapshot: [`68194959ad5bc5540aceb6b40807a8c4228af60a`](https://github.com/PyDevices/graphics/tree/68194959ad5bc5540aceb6b40807a8c4228af60a).

Rectangle geometry, scoped clip contexts, and clipped canvas operations.

Every entry below is generated from a public binding table or header declaration and links to its immutable source line.

## `graphics.Area`

```python
class Area(x=0, y=0, w=0, h=0)
```

Immutable rectangle geometry used for bounds, clipping, and draw results.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L395)

## `graphics.Area.contains`

```python
Area.contains(point_or_x, y=None)
```

Test whether the area contains a point.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L377)

## `graphics.Area.contains_area`

```python
Area.contains_area(other)
```

Test whether another Area is fully contained.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L378)

## `graphics.Area.intersects`

```python
Area.intersects(other)
```

Test whether two areas overlap.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L379)

## `graphics.Area.touches_or_intersects`

```python
Area.touches_or_intersects(other)
```

Test whether two areas overlap or share an edge.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L380)

## `graphics.Area.shift`

```python
Area.shift(dx=0, dy=0)
```

Return a copy translated by the requested offset.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L381)

## `graphics.Area.clip`

```python
Area.clip(other)
```

Return the intersection with another Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L382)

## `graphics.Area.offset`

```python
Area.offset(left, top=None, right=None, bottom=None)
```

Return an Area expanded independently on each edge.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L383)

## `graphics.Area.inset`

```python
Area.inset(left, top=None, right=None, bottom=None)
```

Return an Area reduced independently on each edge.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L384)

## `graphics.Area.x`

```python
Area.x
```

Left coordinate.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L229)

## `graphics.Area.y`

```python
Area.y
```

Top coordinate.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L230)

## `graphics.Area.w`

```python
Area.w
```

Width in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L231)

## `graphics.Area.h`

```python
Area.h
```

Height in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L232)

## `graphics.ClipContext`

```python
class ClipContext(draw, area)
```

Context manager returned by Draw.clip() for scoped clipping.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2147)

## `graphics.ClipContext.__enter__`

```python
ClipContext.__enter__()
```

Push the requested clipping area and return the effective clip.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2140)

## `graphics.ClipContext.__exit__`

```python
ClipContext.__exit__(exc_type, exc_value, traceback)
```

Pop the clipping area when leaving the context.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2141)

## `graphics.ClippedCanvas`

```python
class ClippedCanvas(canvas, clip)
```

Canvas proxy that intersects writes with a fixed Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2414)

## `graphics.ClippedCanvas.pixel`

```python
ClippedCanvas.pixel(x, y, color=None)
```

Read or write one pixel, depending on whether a color is supplied.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2402)

## `graphics.ClippedCanvas.fill`

```python
ClippedCanvas.fill(color)
```

Fill the target canvas with one color and return the affected Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2403)

## `graphics.ClippedCanvas.fill_rect`

```python
ClippedCanvas.fill_rect(x, y, width, height, color)
```

Fill a rectangular region and return the clipped affected Area.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2404)

## `graphics.ClippedCanvas.hline`

```python
ClippedCanvas.hline(x, y, width, color)
```

Draw a horizontal line.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2405)

## `graphics.ClippedCanvas.vline`

```python
ClippedCanvas.vline(x, y, height, color)
```

Draw a vertical line.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2406)

## `graphics.ClippedCanvas.blit_rect`

```python
ClippedCanvas.blit_rect(buffer, x, y, width, height)
```

Copy a packed RGB565 rectangle onto the target.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2407)

## `graphics.ClippedCanvas.blit_transparent`

```python
ClippedCanvas.blit_transparent(buffer, x, y, width, height, key)
```

Copy a packed RGB565 rectangle while skipping the transparent key.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2408)

## `graphics.ClippedCanvas.width`

```python
ClippedCanvas.width
```

Width in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2223)

## `graphics.ClippedCanvas.height`

```python
ClippedCanvas.height
```

Height in pixels.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2224)
