# SPDX-License-Identifier: MIT
"""Build graphics_native CPython extension and graphics package."""

from setuptools import Extension, setup

setup(
    name="graphics-cmod",
    version="0.1.0",
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
        ),
    ],
)
