#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""FrameBuffer subclass test for CPython tp_new/tp_init split."""

import sys

for _p in list(sys.path):
    if _p.endswith("/graphics") and "repos" in _p:
        sys.path.remove(_p)

import graphics
from graphics import FrameBuffer, RGB565, Area


class SubBuffer(FrameBuffer):
    def __init__(self, buf, w, h, fmt):
        super().__init__(buf, w, h, fmt)


buf = bytearray(32 * 32 * 2)
fb = SubBuffer(buf, 32, 32, RGB565)
fb.fill(0)
area = fb.fill_rect(4, 4, 8, 8, 0xF800)
assert area == Area(4, 4, 8, 8)
print("test_subclass: ok")
