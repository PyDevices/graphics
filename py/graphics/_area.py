# SPDX-License-Identifier: MIT
"""Native Area when available, else pure-Python fallback."""

try:
    from graphics_native import Area as _NativeArea
except ImportError:
    _NativeArea = None


if _NativeArea is not None:

    class Area(_NativeArea):
        __hash__ = None

else:
    from ._area_pure import Area  # noqa: F401
