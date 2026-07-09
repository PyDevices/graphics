# SPDX-License-Identifier: MIT
"""Build native graphics C extension."""

import sys

from setuptools import Extension, setup

if sys.platform == "win32":
    extra_compile_args = ["/wd4996"]
else:
    extra_compile_args = [
        "-Wno-unused-function",
        "-Wno-sign-compare",
    ]

GFX_SOURCES = [
    "gfx_module_cpy.c",
    "gfx_framebuffer.c",
    "gfx_shapes.c",
    "gfx_draw.c",
    "gfx_font.c",
    "gfx_capabilities.c",
]

setup(
    name="graphics-cmod",
    packages=[],
    py_modules=[],
    ext_modules=[
        Extension(
            "graphics",
            sources=GFX_SOURCES,
            include_dirs=["."],
            extra_compile_args=extra_compile_args,
        ),
    ],
)
