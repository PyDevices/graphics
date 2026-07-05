# SPDX-License-Identifier: MIT
"""Platform capability registry for graphics."""

import sys

_DIALECT = sys.implementation.name

_FORMAT_NAMES = (
    "MONO_VLSB",
    "MONO_HLSB",
    "MONO_HMSB",
    "RGB565",
    "GS2_HMSB",
    "GS4_HMSB",
    "GS8",
)

_CAPS = {
    "framebuf": "pure_python",
    "dialect": _DIALECT,
    "formats": list(_FORMAT_NAMES),
}


def init_capabilities(*, framebuf_backend, formats=None):
    _CAPS["framebuf"] = framebuf_backend
    _CAPS["dialect"] = _DIALECT
    if formats is not None:
        _CAPS["formats"] = list(formats)


def capabilities():
    return dict(_CAPS)


def framebuf_backend():
    return _CAPS["framebuf"]
