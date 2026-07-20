---
title: Runtime and pixel formats
description: Runtime capability probes and the exported framebuffer format constants.
---

Source snapshot: [`68194959ad5bc5540aceb6b40807a8c4228af60a`](https://github.com/PyDevices/graphics/tree/68194959ad5bc5540aceb6b40807a8c4228af60a).

Runtime capability probes and the exported framebuffer format constants.

Every entry below is generated from a public binding table or header declaration and links to its immutable source line.

## `graphics.framebuf_backend`

```python
graphics.framebuf_backend()
```

Return the active framebuffer backend identifier.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2950)

## `graphics.implementation`

```python
graphics.implementation()
```

Return the implementation identifier for this graphics module.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2951)

## `graphics.capabilities`

```python
graphics.capabilities()
```

Return structured runtime feature and pixel-format capabilities.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L2952)

## `graphics.MONO_VLSB`

```python
graphics.MONO_VLSB
```

Monochrome vertical least-significant-bit layout.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L3027)

## `graphics.MONO_HLSB`

```python
graphics.MONO_HLSB
```

Monochrome horizontal least-significant-bit layout.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L3028)

## `graphics.MONO_HMSB`

```python
graphics.MONO_HMSB
```

Monochrome horizontal most-significant-bit layout.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L3029)

## `graphics.RGB565`

```python
graphics.RGB565
```

16-bit RGB565 pixel layout.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L3030)

## `graphics.GS2_HMSB`

```python
graphics.GS2_HMSB
```

2-bit grayscale horizontal most-significant-bit layout.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L3031)

## `graphics.GS4_HMSB`

```python
graphics.GS4_HMSB
```

4-bit grayscale horizontal most-significant-bit layout.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L3032)

## `graphics.GS8`

```python
graphics.GS8
```

8-bit grayscale layout.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L3033)

## `graphics.RGB888`

```python
graphics.RGB888
```

24-bit RGB888 pixel layout.

[View the pinned source declaration](https://github.com/PyDevices/graphics/blob/68194959ad5bc5540aceb6b40807a8c4228af60a/gfx_module_cpy.c#L3034)
