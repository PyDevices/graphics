#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import sys

for _p in list(sys.path):
    if _p.endswith("/graphics") and "repos" in _p:
        sys.path.remove(_p)

import array

import graphics
from graphics import Area, FrameBuffer, RGB565, capabilities, framebuf_backend

print("backend:", framebuf_backend())
assert framebuf_backend() == "native", capabilities()

# Area geometry
a = Area(0, 0, 10, 10)
assert a.contains(5, 5)
assert not a.contains(10, 10)
assert Area(0, 0, 5, 5).intersects(Area(4, 4, 5, 5))
assert Area(0, 0, 2, 2) + Area(4, 4, 2, 2) == Area(0, 0, 6, 6)
x, y, w, h = Area(1, 2, 3, 4)
assert (x, y, w, h) == (1, 2, 3, 4)
assert repr(Area(1, 2, 3, 4)) == "Area(1, 2, 3, 4)"

# FrameBuffer RGB565
buf = bytearray(128 * 128 * 2)
fb = FrameBuffer(buf, 128, 128, RGB565)
fb.fill(0)
area = fb.fill_rect(10, 10, 20, 20, 0xF800)
assert area == Area(10, 10, 20, 20)
fb.line(0, 0, 64, 64, 0x07E0)
fb.hline(0, 0, 32, 0x001F)
fb.vline(0, 0, 32, 0xFFFF)
fb.rect(5, 5, 10, 10, 0x1234)
fb.ellipse(64, 64, 20, 10, 0xABCD)
coords = array.array("h", [0, 0, 10, 0, 5, 10])
fb.poly(20, 20, coords, 0x5555, True)

print("graphics smoke: ok")
