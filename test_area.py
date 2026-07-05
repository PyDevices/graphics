#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Area tests for MicroPython and CPython."""

import sys

for _p in list(sys.path):
    if _p.endswith("/graphics") and "repos" in _p:
        sys.path.remove(_p)

from graphics import Area

assert Area((5, 6, 7, 8)) == Area(5, 6, 7, 8)
a = Area(0, 0, 10, 10)
assert a.contains(5, 5)
assert not a.contains(10, 10)
assert Area(0, 0, 5, 5).intersects(Area(4, 4, 5, 5))
assert Area(0, 0, 10, 10).clip(Area(5, 5, 10, 10)) == Area(5, 5, 5, 5)
assert Area(1, 2, 3, 4).shift(10, 20) == Area(11, 22, 3, 4)
assert Area(0, 0, 2, 2) + Area(4, 4, 2, 2) == Area(0, 0, 6, 6)
x, y, w, h = Area(1, 2, 3, 4)
assert (x, y, w, h) == (1, 2, 3, 4)
assert repr(Area(1, 2, 3, 4)) == "Area(1, 2, 3, 4)"
try:
    hash(Area(1, 2, 3, 4))
    raise AssertionError("Area should be unhashable")
except TypeError:
    pass
print("test_area: ok")
