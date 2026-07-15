#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify graphics cmod exports match pydisplay __all__."""

import graphics

ALL = [
    "BMP565",
    "GS2_HMSB",
    "GS4_HMSB",
    "GS8",
    "MONO_HLSB",
    "MONO_HMSB",
    "MONO_VLSB",
    "RGB565",
    "RGB888",
    "Area",
    "ClipContext",
    "ClippedCanvas",
    "Draw",
    "Font",
    "FrameBuffer",
    "arc",
    "blit",
    "blit_rect",
    "blit_transparent",
    "bmp_to_framebuffer",
    "capabilities",
    "circle",
    "ellipse",
    "fill",
    "fill_rect",
    "framebuf_backend",
    "gradient_rect",
    "hline",
    "implementation",
    "line",
    "load_image",
    "pbm_to_framebuffer",
    "pgm_to_framebuffer",
    "pixel",
    "poly",
    "polygon",
    "rect",
    "round_rect",
    "save_image",
    "text",
    "text8",
    "text14",
    "text16",
    "triangle",
    "vline",
]

missing = [name for name in ALL if not hasattr(graphics, name)]
if missing:
    raise SystemExit("missing exports: " + ", ".join(missing))

assert graphics.framebuf_backend() == "native", graphics.capabilities()
assert graphics.implementation() == "native_cmod", graphics.capabilities()

buf = bytearray(32 * 32 * 2)
fb = graphics.FrameBuffer(buf, 32, 32, graphics.RGB565)
assert fb.buffer is buf or bytes(fb.buffer) == bytes(buf)
fb.fill(0)
graphics.fill_rect(fb, 1, 1, 4, 4, 0xF800)
graphics.text8(fb, "Hi", 0, 0, 0xFFFF)
graphics.text16(fb, "C", 0, 32, scale=1)
d = graphics.Draw(fb)
d.fill_rect(0, 0, 2, 2, 1)
clip = graphics.Area(0, 0, 8, 8)
cc = graphics.ClippedCanvas(fb, clip)
assert cc.width == 32
ctx = graphics.ClipContext(d, clip)
with ctx as eff:
    assert isinstance(eff, graphics.Area)
print("test_parity: ok")
