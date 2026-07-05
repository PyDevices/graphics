# SPDX-License-Identifier: MIT
"""Framebuf backend selection: native extension, built-in framebuf, or pure Python."""

_NATIVE = False

try:
    from graphics_native import (  # type: ignore
        GS2_HMSB,
        GS4_HMSB,
        GS8,
        MONO_HLSB,
        MONO_HMSB,
        MONO_VLSB,
        RGB565,
        FrameBuffer,
    )

    _NATIVE = True
except ImportError:
    try:
        from framebuf import (  # type: ignore
            GS2_HMSB,
            GS4_HMSB,
            GS8,
            MONO_HLSB,
            MONO_HMSB,
            MONO_VLSB,
            RGB565,
            FrameBuffer,
        )

        _NATIVE = True
    except ImportError:
        from ._framebuf_pure import (  # noqa: F401
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
    "GS2_HMSB",
    "GS4_HMSB",
    "GS8",
    "MONO_HLSB",
    "MONO_HMSB",
    "MONO_VLSB",
    "RGB565",
    "FrameBuffer",
    "_NATIVE",
]
