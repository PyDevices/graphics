# SPDX-License-Identifier: MIT
"""FrameBuffer wrapper that returns native Area bounding boxes."""

from ._area import Area
from ._capabilities import init_capabilities
from ._framebuf import (
    GS2_HMSB,
    GS4_HMSB,
    GS8,
    MONO_HLSB,
    MONO_HMSB,
    MONO_VLSB,
    RGB565,
    _NATIVE,
)
from ._framebuf import FrameBuffer as _FrameBuffer

init_capabilities(
    framebuf_backend="native" if _NATIVE else "pure_python",
    formats=[
        "MONO_VLSB",
        "MONO_HLSB",
        "MONO_HMSB",
        "RGB565",
        "GS2_HMSB",
        "GS4_HMSB",
        "GS8",
    ],
)


class FrameBuffer(_FrameBuffer):
    def __init__(self, buffer, width, height, format, *args, **kwargs):
        super().__init__(buffer, width, height, format, *args, **kwargs)
        self._width = width
        self._height = height

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height

    def fill_rect(self, x, y, w, h, c):
        super().fill_rect(x, y, w, h, c)
        return Area(x, y, w, h)

    def pixel(self, x, y, c=None):
        if c is None:
            return super().pixel(x, y)
        super().pixel(x, y, c)
        return Area(x, y, 1, 1)

    def fill(self, c):
        super().fill(c)
        return Area(0, 0, self.width, self.height)

    def hline(self, x, y, w, c):
        super().hline(x, y, w, c)
        return Area(x, y, w, 1)

    def vline(self, x, y, h, c):
        super().vline(x, y, h, c)
        return Area(x, y, 1, h)

    def line(self, x1, y1, x2, y2, c):
        super().line(x1, y1, x2, y2, c)
        return Area(min(x1, x2), min(y1, y2), abs(x2 - x1) + 1, abs(y2 - y1) + 1)

    def rect(self, x, y, w, h, c, f=False):
        super().rect(x, y, w, h, c, f)
        return Area(x, y, w, h)

    def scroll(self, xstep, ystep):
        super().scroll(xstep, ystep)
        return Area(0, 0, self.width, self.height)
