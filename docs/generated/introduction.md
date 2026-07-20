---
title: graphics API reference
description: Source-linked Python and native C reference for the PyDevices graphics module.
---

`graphics` is the all-C drawing module shared by CPython, MicroPython, and CircuitPython builds in the PyDevices stack.

## Quick start

```python
from graphics import Area, FrameBuffer, RGB565

buffer = bytearray(160 * 128 * 2)
canvas = FrameBuffer(buffer, 160, 128, RGB565)
changed = canvas.fill_rect(10, 10, 40, 24, 0xF800)
assert changed == Area(10, 10, 40, 24)
```

## Reference snapshot

- Release line: **v0.0.8**
- Source commit: [`68194959ad5bc5540aceb6b40807a8c4228af60a`](https://github.com/PyDevices/graphics/tree/68194959ad5bc5540aceb6b40807a8c4228af60a)
- License: **MIT**
- Adapter: **CPython binding tables plus public C headers**
- Python API entries indexed: **136**
- Native C functions indexed: **59**
- Source files represented: **7**
- Tracked C and header files in the repository: **31**

The generated pages expose the user-facing Python names and the lower-level C entry points used by firmware and extension integrations. Every entry links to the exact source snapshot above.

## Browse the reference

- [Area and clipping](https://pydevices.github.io/graphics/api/generated/area-clipping.html) — 26 entries
- [FrameBuffer](https://pydevices.github.io/graphics/api/generated/framebuffer.html) — 31 entries
- [Drawing operations](https://pydevices.github.io/graphics/api/generated/drawing.html) — 46 entries
- [Fonts and images](https://pydevices.github.io/graphics/api/generated/fonts-images.html) — 22 entries
- [Runtime and pixel formats](https://pydevices.github.io/graphics/api/generated/module-formats.html) — 11 entries
- [Native drawing and framebuffer API](https://pydevices.github.io/graphics/api/generated/native-drawing.html) — 34 entries
- [Native fonts, images, and runtime API](https://pydevices.github.io/graphics/api/generated/native-assets-runtime.html) — 25 entries
