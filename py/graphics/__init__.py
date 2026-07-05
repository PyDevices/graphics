# SPDX-License-Identifier: MIT
"""graphics — native framebuf + Area for MicroPython, CircuitPython, and CPython."""

from ._area import Area
from ._capabilities import capabilities, framebuf_backend
from ._framebuf_plus import (
    GS2_HMSB,
    GS4_HMSB,
    GS8,
    MONO_HLSB,
    MONO_HMSB,
    MONO_VLSB,
    RGB565,
    FrameBuffer,
)

__all__ = [
    "Area",
    "FrameBuffer",
    "GS2_HMSB",
    "GS4_HMSB",
    "GS8",
    "MONO_HLSB",
    "MONO_HMSB",
    "MONO_VLSB",
    "RGB565",
    "capabilities",
    "framebuf_backend",
]
