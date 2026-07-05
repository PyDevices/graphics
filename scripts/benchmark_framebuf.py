#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import os
import sys

_repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _repo in sys.path:
    sys.path.remove(_repo)

import time

from graphics import RGB565, framebuf_backend
from graphics._framebuf import FrameBuffer as NativeFB
from graphics._framebuf_pure import FrameBuffer as PureFB

W, H = 128, 128
ITERS = 200


def bench(cls, label):
    buf = bytearray(W * H * 2)
    fb = cls(buf, W, H, RGB565)
    t0 = time.perf_counter()
    for _ in range(ITERS):
        fb.fill(0)
        fb.fill_rect(10, 10, 40, 40, 0xF800)
        fb.line(0, 0, W - 1, H - 1, 0x07E0)
        fb.ellipse(64, 64, 30, 20, 0x001F)
    elapsed = time.perf_counter() - t0
    print(f"{label}: {elapsed:.3f}s ({ITERS} iters)")
    return elapsed


def main():
    print("framebuf backend:", framebuf_backend())
    native_t = bench(NativeFB, "native")
    pure_t = bench(PureFB, "pure_python")
    if pure_t > 0:
        print(f"speedup: {pure_t / native_t:.1f}x")


if __name__ == "__main__":
    main()
