#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate C font headers from pydisplay ``lib/graphics/_font_8x*.py`` romfont data.

Canonical font bytes live in pydisplay; this script writes ``font_8x8.h``,
``font_8x14.h``, and ``font_8x16.h`` next to ``gfx_font.c``.

Usage (from repo root)::

    python3 scripts/sync_fonts.py
    python3 scripts/sync_fonts.py --check

Default pydisplay path: ``../../pydisplay`` relative to this graphics repo.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

_GRAPHICS_DIR = Path(__file__).resolve().parents[1]
_DEFAULT_PYDISPLAY = _GRAPHICS_DIR.parent.parent / "pydisplay"


def _load_font_bytes(py_path: Path) -> bytes:
    source = py_path.read_text(encoding="utf-8")
    match = re.search(r"_FONT\s*=\s*\(", source, re.MULTILINE)
    if not match:
        raise SystemExit(f"no _FONT tuple in {py_path}")
    ns: dict = {}
    exec(compile(source, str(py_path), "exec"), ns)
    data = bytes(ns["_FONT"])
    return data


def _format_c_array(data: bytes, *, cols: int = 16) -> str:
    lines = []
    row = []
    for i, byte in enumerate(data):
        row.append(f"0x{byte:02x}")
        if len(row) == cols:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row))
    return "\n".join(lines)


def _render_header(name: str, array_name: str, data: bytes, height: int) -> str:
    guard = f"FONT_{name}_H"
    return f"""#ifndef {guard}
#define {guard}
#include <stdint.h>

/* romfont from pydisplay src/lib/graphics/_font_{name.lower()}.py ({height}px, 256 glyphs) */
static const uint8_t {array_name}[] = {{
{_format_c_array(data)}
}};

#define FONT_{name}_LEN {len(data)}
#define FONT_{name}_HEIGHT {height}

#endif
"""


def sync_fonts(*, pydisplay: Path, check: bool = False) -> None:
    fonts = (
        ("8X8", "font_8x8", 8, pydisplay / "src/lib/graphics/_font_8x8.py"),
        ("8X14", "font_8x14", 14, pydisplay / "src/lib/graphics/_font_8x14.py"),
        ("8X16", "font_8x16", 16, pydisplay / "src/lib/graphics/_font_8x16.py"),
    )
    errors = []
    for c_name, array_name, height, py_path in fonts:
        if not py_path.is_file():
            raise SystemExit(f"missing font source: {py_path}")
        data = _load_font_bytes(py_path)
        expected = 256 * height
        if len(data) != expected:
            raise SystemExit(f"{py_path.name}: expected {expected} bytes, got {len(data)}")
        out_path = _GRAPHICS_DIR / f"{array_name}.h"
        content = _render_header(c_name, array_name, data, height)
        if check:
            if not out_path.is_file():
                errors.append(f"missing {out_path.name}")
            elif out_path.read_text(encoding="utf-8") != content:
                errors.append(f"stale {out_path.name}")
            continue
        out_path.write_text(content, encoding="utf-8")
        print(f"wrote {out_path.name} ({len(data)} bytes)")

    if check:
        if errors:
            for err in errors:
                print(err, file=sys.stderr)
            raise SystemExit(1)
        print("font headers up to date")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--pydisplay",
        type=Path,
        default=_DEFAULT_PYDISPLAY,
        help="path to pydisplay repo (default: ../../pydisplay)",
    )
    parser.add_argument("--check", action="store_true", help="fail if headers are missing or stale")
    args = parser.parse_args()
    sync_fonts(pydisplay=args.pydisplay.resolve(), check=args.check)


if __name__ == "__main__":
    main()
