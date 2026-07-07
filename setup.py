# SPDX-License-Identifier: MIT
"""Build graphics_native CPython extension and graphics package."""

import sys

from setuptools import Extension, setup

RELEASE_VERSION = "0.0.1"  # baseline when no git tags (see scripts/next_release_version.sh)

if sys.platform == "win32":
    extra_compile_args = ["/wd4996"]
else:
    extra_compile_args = [
        "-Wno-unused-function",
        "-Wno-sign-compare",
    ]

setup(
    name="graphics-cmod",
    packages=["graphics", "gfxpy"],
    package_dir={
        "graphics": "py/graphics",
        "gfxpy": "gfxpy",
    },
    ext_modules=[
        Extension(
            "graphics_native",
            sources=["graphics_native_cpy.c"],
            include_dirs=["."],
            extra_compile_args=extra_compile_args,
        ),
    ],
)
