#!/usr/bin/env python3
"""Generate a source-linked reference from graphics' public C bindings."""

from __future__ import annotations

import json
import os
import re
import subprocess
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple


REPO_ROOT = Path(__file__).resolve().parents[2]
DOCS_ROOT = REPO_ROOT / "docs"
OUTPUT_ROOT = DOCS_ROOT / "generated"
REPOSITORY_URL = "https://github.com/PyDevices/graphics"
PROJECT_DOCS_URL = "https://pydevices.github.io/graphics/api"
SOURCEY_VERSION = "3.6.5"
CPYTHON_BINDING = "gfx_module_cpy.c"


CLASS_SPECS: Dict[str, Dict[str, object]] = {
    "Area": {
        "methods": "area_methods",
        "getset": "area_getset",
        "summary": "Immutable rectangle geometry used for bounds, clipping, and draw results.",
        "signature": "class Area(x=0, y=0, w=0, h=0)",
    },
    "FrameBuffer": {
        "methods": "framebuffer_methods",
        "getset": "framebuffer_getset",
        "summary": "A framebuf-compatible native drawing surface backed by a writable buffer.",
        "signature": "class FrameBuffer(buffer, width, height, format, stride=None)",
    },
    "Draw": {
        "methods": "draw_methods",
        "getset": None,
        "summary": "Drawing facade that adds a bounded clip stack to any compatible canvas.",
        "signature": "class Draw(canvas)",
    },
    "ClipContext": {
        "methods": "clipctx_methods",
        "getset": None,
        "summary": "Context manager returned by Draw.clip() for scoped clipping.",
        "signature": "class ClipContext(draw, area)",
    },
    "ClippedCanvas": {
        "methods": "clipped_canvas_methods",
        "getset": "clipped_canvas_getset",
        "summary": "Canvas proxy that intersects writes with a fixed Area.",
        "signature": "class ClippedCanvas(canvas, clip)",
    },
    "Font": {
        "methods": "font_methods",
        "getset": "font_getset",
        "summary": "Bitmap font loader and renderer for built-in, file, or buffer-backed glyph data.",
        "signature": "class Font(font_data=None, height=0, cached=True)",
    },
    "BMP565": {
        "methods": "bmp565_methods",
        "getset": "bmp565_getset",
        "summary": "RGB565 bitmap reader supporting buffered and streamed access.",
        "signature": "class BMP565(filename=None, source=None, streamed=False, mirrored=False, width=None, height=None)",
    },
}

COMMON_DRAW_SIGNATURES = {
    "fill": "color",
    "fill_rect": "x, y, width, height, color",
    "pixel": "x, y, color=None",
    "hline": "x, y, width, color",
    "vline": "x, y, height, color",
    "line": "x1, y1, x2, y2, color",
    "rect": "x, y, width, height, color, fill=False",
    "round_rect": "x, y, width, height, radius, color, fill=False",
    "circle": "x, y, radius, color, fill=False",
    "ellipse": "x, y, x_radius, y_radius, color, fill=False, mask=0x0F",
    "arc": "x, y, radius, start_angle, end_angle, color",
    "triangle": "x0, y0, x1, y1, x2, y2, color, fill=False",
    "gradient_rect": "x, y, width, height, color1, color2=None, vertical=True",
    "poly": "x, y, coordinates, color, fill=False",
    "polygon": "points, x, y, color, angle=0, center_x=0, center_y=0",
    "blit": "source, x, y, key=-1, palette=None",
    "blit_rect": "buffer, x, y, width, height",
    "blit_transparent": "buffer, x, y, width, height, key",
    "text": "text, x, y, color=1, scale=1, inverted=False, font_data=None, height=8",
    "text8": "text, x, y, color=1, scale=1, inverted=False, font_data=None",
    "text14": "text, x, y, color=1, scale=1, inverted=False, font_data=None",
    "text16": "text, x, y, color=1, scale=1, inverted=False, font_data=None",
}

AREA_SIGNATURES = {
    "contains": "point_or_x, y=None",
    "contains_area": "other",
    "intersects": "other",
    "touches_or_intersects": "other",
    "shift": "dx=0, dy=0",
    "clip": "other",
    "offset": "left, top=None, right=None, bottom=None",
    "inset": "left, top=None, right=None, bottom=None",
}

SPECIAL_SIGNATURES = {
    ("FrameBuffer", "from_file"): "path",
    ("FrameBuffer", "scroll"): "x_step, y_step",
    ("FrameBuffer", "save"): "path='screenshot'",
    ("Draw", "clip"): "area_or_x, y=None, width=None, height=None",
    ("ClipContext", "__enter__"): "",
    ("ClipContext", "__exit__"): "exc_type, exc_value, traceback",
    ("Font", "text"): "canvas, text, x, y, color, scale=1, inverted=False",
    ("Font", "draw_char"): "char, x, y, canvas, color, scale=1, inverted=False",
    ("Font", "text_width"): "text, scale=1",
    ("Font", "deinit"): "",
    ("Font", "export"): "filename",
    ("BMP565", "save"): "filename=None",
    ("BMP565", "deinit"): "",
}

MODULE_SPECIAL_SIGNATURES = {
    "framebuf_backend": "",
    "implementation": "",
    "capabilities": "",
    "load_image": "path",
    "save_image": "framebuffer, path='screenshot'",
    "bmp_to_framebuffer": "path",
    "pbm_to_framebuffer": "path",
    "pgm_to_framebuffer": "path",
}

DESCRIPTIONS = {
    "fill": "Fill the target canvas with one color and return the affected Area.",
    "fill_rect": "Fill a rectangular region and return the clipped affected Area.",
    "pixel": "Read or write one pixel, depending on whether a color is supplied.",
    "hline": "Draw a horizontal line.",
    "vline": "Draw a vertical line.",
    "line": "Draw a line between two points.",
    "rect": "Draw an outlined or filled rectangle.",
    "round_rect": "Draw an outlined or filled rounded rectangle.",
    "circle": "Draw an outlined or filled circle.",
    "ellipse": "Draw selected quadrants of an outlined or filled ellipse.",
    "arc": "Draw a circular arc between two angles.",
    "triangle": "Draw an outlined or filled triangle.",
    "gradient_rect": "Fill a rectangle with a horizontal or vertical color gradient.",
    "poly": "Draw a polygon from a packed coordinate buffer.",
    "polygon": "Draw a translated and optionally rotated sequence of points.",
    "blit": "Copy another framebuffer onto the target with optional keying and palette lookup.",
    "blit_rect": "Copy a packed RGB565 rectangle onto the target.",
    "blit_transparent": "Copy a packed RGB565 rectangle while skipping the transparent key.",
    "text": "Render bitmap text with selectable height, scale, and inversion.",
    "text8": "Render text with the built-in 8-pixel-high font path.",
    "text14": "Render text with the built-in 14-pixel-high font path.",
    "text16": "Render text with the built-in 16-pixel-high font path.",
    "contains": "Test whether the area contains a point.",
    "contains_area": "Test whether another Area is fully contained.",
    "intersects": "Test whether two areas overlap.",
    "touches_or_intersects": "Test whether two areas overlap or share an edge.",
    "shift": "Return a copy translated by the requested offset.",
    "clip": "Return the intersection with another Area.",
    "offset": "Return an Area expanded independently on each edge.",
    "inset": "Return an Area reduced independently on each edge.",
    "from_file": "Load a supported image file into a new FrameBuffer.",
    "scroll": "Move framebuffer contents by the requested x and y offsets.",
    "save": "Write the current image or bitmap to a versioned output path.",
    "framebuf_backend": "Return the active framebuffer backend identifier.",
    "implementation": "Return the implementation identifier for this graphics module.",
    "capabilities": "Return structured runtime feature and pixel-format capabilities.",
    "load_image": "Load a supported image file as a FrameBuffer.",
    "save_image": "Save a FrameBuffer to a versioned image path.",
    "bmp_to_framebuffer": "Decode a BMP file into a FrameBuffer.",
    "pbm_to_framebuffer": "Decode a PBM file into a FrameBuffer.",
    "pgm_to_framebuffer": "Decode a PGM file into a FrameBuffer.",
    "draw_char": "Render one glyph through this Font onto a canvas.",
    "text_width": "Measure rendered text width at the selected scale.",
    "deinit": "Release resources owned by the object.",
    "export": "Write cached font bytes to a file.",
    "__enter__": "Push the requested clipping area and return the effective clip.",
    "__exit__": "Pop the clipping area when leaving the context.",
}

PROPERTY_DESCRIPTIONS = {
    "x": "Left coordinate.",
    "y": "Top coordinate.",
    "w": "Width in pixels.",
    "h": "Height in pixels.",
    "width": "Width in pixels.",
    "height": "Height in pixels.",
    "buffer": "Backing pixel buffer when the object owns or exposes one.",
    "format": "Framebuffer pixel-format identifier.",
    "color_depth": "Bits per pixel for the framebuffer format.",
    "font_name": "Font source name or the in-memory/default identifier.",
    "bpp": "Bitmap color depth in bits per pixel.",
    "BPP": "Bitmap storage width in bytes per pixel.",
}

CONSTANT_DESCRIPTIONS = {
    "MONO_VLSB": "Monochrome vertical least-significant-bit layout.",
    "MONO_HLSB": "Monochrome horizontal least-significant-bit layout.",
    "MONO_HMSB": "Monochrome horizontal most-significant-bit layout.",
    "RGB565": "16-bit RGB565 pixel layout.",
    "GS2_HMSB": "2-bit grayscale horizontal most-significant-bit layout.",
    "GS4_HMSB": "4-bit grayscale horizontal most-significant-bit layout.",
    "GS8": "8-bit grayscale layout.",
    "RGB888": "24-bit RGB888 pixel layout.",
}

DRAW_FUNCTIONS = set(COMMON_DRAW_SIGNATURES)
IMAGE_FUNCTIONS = {
    "load_image",
    "save_image",
    "bmp_to_framebuffer",
    "pbm_to_framebuffer",
    "pgm_to_framebuffer",
}
RUNTIME_FUNCTIONS = {"framebuf_backend", "implementation", "capabilities"}

HEADER_GROUPS: Sequence[Tuple[str, str, str, Sequence[str]]] = (
    (
        "native-drawing",
        "Native drawing and framebuffer API",
        "Public C declarations for geometry, framebuffer operations, shapes, and clipping.",
        ("gfx_core.h", "gfx_framebuffer.h", "gfx_shapes.h", "gfx_draw.h"),
    ),
    (
        "native-assets-runtime",
        "Native fonts, images, and runtime API",
        "Public C declarations for fonts, BMP565 data, image conversion, and capability reporting.",
        ("gfx_font.h", "gfx_bmp565.h", "gfx_files.h", "gfx_capabilities.h"),
    ),
)


def git_commit() -> str:
    override = os.environ.get("SOURCE_COMMIT", "").strip()
    if override:
        if not re.fullmatch(r"[0-9a-fA-F]{40}", override):
            raise ValueError("SOURCE_COMMIT must be a full 40-character Git SHA")
        return override.lower()
    return subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=REPO_ROOT, text=True
    ).strip()


def release_tag() -> str:
    override = os.environ.get("SOURCE_RELEASE", "").strip()
    if override:
        return override
    return subprocess.check_output(
        ["git", "describe", "--tags", "--abbrev=0"],
        cwd=REPO_ROOT,
        text=True,
    ).strip()


def tracked_native_source_count() -> int:
    output = subprocess.check_output(
        ["git", "ls-files", "*.c", "*.h"], cwd=REPO_ROOT, text=True
    )
    return len([line for line in output.splitlines() if line.strip()])


def source_url(commit: str, path: str, line: int) -> str:
    return f"{REPOSITORY_URL}/blob/{commit}/{path}#L{line}"


def find_table(lines: Sequence[str], declaration: str) -> Tuple[int, List[str]]:
    start = next(
        index for index, line in enumerate(lines) if declaration in line
    )
    block: List[str] = []
    for index in range(start + 1, len(lines)):
        if lines[index].strip() == "};":
            break
        block.append(lines[index])
    return start + 1, block


def method_entries(table: str, commit: str) -> List[Dict[str, object]]:
    lines = (REPO_ROOT / CPYTHON_BINDING).read_text(encoding="utf-8").splitlines()
    start, block = find_table(lines, f"static PyMethodDef {table}[]")
    records: List[Dict[str, object]] = []
    pattern = re.compile(
        r'^\s*\{"([^"]+)",\s*(?:\(PyCFunction\))?([A-Za-z_][A-Za-z0-9_]*)'
    )
    for offset, line in enumerate(block, start=1):
        match = pattern.search(line)
        if not match:
            continue
        records.append(
            {
                "name": match.group(1),
                "binding": match.group(2),
                "path": CPYTHON_BINDING,
                "line": start + offset,
                "source_url": source_url(commit, CPYTHON_BINDING, start + offset),
            }
        )
    return records


def property_entries(table: str, commit: str) -> List[Dict[str, object]]:
    lines = (REPO_ROOT / CPYTHON_BINDING).read_text(encoding="utf-8").splitlines()
    start, block = find_table(lines, f"static PyGetSetDef {table}[]")
    records: List[Dict[str, object]] = []
    pattern = re.compile(r'^\s*\{"([^"]+)",\s*\(getter\)([A-Za-z_][A-Za-z0-9_]*)')
    for offset, line in enumerate(block, start=1):
        match = pattern.search(line)
        if not match:
            continue
        records.append(
            {
                "name": match.group(1),
                "binding": match.group(2),
                "path": CPYTHON_BINDING,
                "line": start + offset,
                "source_url": source_url(commit, CPYTHON_BINDING, start + offset),
            }
        )
    return records


def type_line(name: str) -> int:
    lines = (REPO_ROOT / CPYTHON_BINDING).read_text(encoding="utf-8").splitlines()
    needle = f'.tp_name = "graphics.{name}"'
    return next(index for index, line in enumerate(lines, start=1) if needle in line)


def python_signature(owner: str, name: str) -> str:
    if owner == "Area" and name in AREA_SIGNATURES:
        args = AREA_SIGNATURES[name]
    elif (owner, name) in SPECIAL_SIGNATURES:
        args = SPECIAL_SIGNATURES[(owner, name)]
    elif name in COMMON_DRAW_SIGNATURES:
        args = COMMON_DRAW_SIGNATURES[name]
    else:
        args = "*args, **kwargs"
    return f"{owner}.{name}({args})"


def module_signature(name: str) -> str:
    if name in MODULE_SPECIAL_SIGNATURES:
        args = MODULE_SPECIAL_SIGNATURES[name]
    elif name in COMMON_DRAW_SIGNATURES:
        args = f"canvas, {COMMON_DRAW_SIGNATURES[name]}"
    else:
        args = "*args, **kwargs"
    return f"graphics.{name}({args})"


def class_records(name: str, commit: str) -> List[Dict[str, object]]:
    spec = CLASS_SPECS[name]
    line = type_line(name)
    records: List[Dict[str, object]] = [
        {
            "name": name,
            "qualified_name": f"graphics.{name}",
            "kind": "class",
            "signature": str(spec["signature"]),
            "summary": str(spec["summary"]),
            "path": CPYTHON_BINDING,
            "line": line,
            "source_url": source_url(commit, CPYTHON_BINDING, line),
        }
    ]
    for item in method_entries(str(spec["methods"]), commit):
        method_name = str(item["name"])
        records.append(
            {
                **item,
                "qualified_name": f"graphics.{name}.{method_name}",
                "kind": "method",
                "signature": python_signature(name, method_name),
                "summary": DESCRIPTIONS.get(
                    method_name,
                    f"Public {name} operation implemented by the native binding.",
                ),
            }
        )
    getset = spec.get("getset")
    if getset:
        for item in property_entries(str(getset), commit):
            property_name = str(item["name"])
            records.append(
                {
                    **item,
                    "qualified_name": f"graphics.{name}.{property_name}",
                    "kind": "property",
                    "signature": f"{name}.{property_name}",
                    "summary": PROPERTY_DESCRIPTIONS.get(
                        property_name, f"Read-only {name} property."
                    ),
                }
            )
    return records


def module_records(commit: str) -> List[Dict[str, object]]:
    records: List[Dict[str, object]] = []
    for item in method_entries("module_methods", commit):
        name = str(item["name"])
        records.append(
            {
                **item,
                "qualified_name": f"graphics.{name}",
                "kind": "function",
                "signature": module_signature(name),
                "summary": DESCRIPTIONS.get(
                    name, "Public module function implemented by the native binding."
                ),
            }
        )
    return records


def constant_records(commit: str) -> List[Dict[str, object]]:
    lines = (REPO_ROOT / CPYTHON_BINDING).read_text(encoding="utf-8").splitlines()
    pattern = re.compile(r'^\s*ADD_INT\("([^"]+)",\s*([^\)]+)\);')
    records: List[Dict[str, object]] = []
    for line_number, line in enumerate(lines, start=1):
        match = pattern.search(line)
        if not match:
            continue
        name = match.group(1)
        records.append(
            {
                "name": name,
                "qualified_name": f"graphics.{name}",
                "kind": "constant",
                "signature": f"graphics.{name}",
                "summary": CONSTANT_DESCRIPTIONS.get(
                    name, "Native graphics module constant."
                ),
                "binding": match.group(2).strip(),
                "path": CPYTHON_BINDING,
                "line": line_number,
                "source_url": source_url(commit, CPYTHON_BINDING, line_number),
            }
        )
    return records


def normalize_c_declaration(lines: Iterable[str]) -> str:
    return " ".join(" ".join(lines).split())


def header_prototypes(path: str, commit: str) -> List[Dict[str, object]]:
    lines = (REPO_ROOT / path).read_text(encoding="utf-8").splitlines()
    records: List[Dict[str, object]] = []
    buffer: List[str] = []
    start_line = 0
    for line_number, raw in enumerate(lines, start=1):
        stripped = re.sub(r"/\*.*?\*/", "", raw).strip()
        if not buffer:
            if (
                not stripped
                or stripped.startswith(("#", "typedef", "//"))
                or "gfx_" not in stripped
                or "(" not in stripped
            ):
                continue
            start_line = line_number
        buffer.append(stripped)
        if ";" not in stripped:
            continue
        declaration = normalize_c_declaration(buffer)
        buffer = []
        match = re.match(
            r"^(?!static\b)(?:const\s+)?[A-Za-z_][A-Za-z0-9_\s\*]*\s+(gfx_[A-Za-z0-9_]+)\s*\((.*)\)\s*;$",
            declaration,
        )
        if not match:
            continue
        name = match.group(1)
        family = name.removeprefix("gfx_").split("_", maxsplit=1)[0]
        records.append(
            {
                "name": name,
                "qualified_name": name,
                "kind": "c_function",
                "signature": declaration,
                "summary": (
                    f"Native {family} entry point declared for firmware and extension integration."
                ),
                "path": path,
                "line": start_line,
                "source_url": source_url(commit, path, start_line),
            }
        )
    return records


def frontmatter(title: str, description: str) -> str:
    return (
        "---\n"
        f"title: {title}\n"
        f"description: {description}\n"
        "---\n"
    )


def render_record(record: Dict[str, object]) -> str:
    language = "c" if record["kind"] == "c_function" else "python"
    lines = [
        f"## `{record['qualified_name']}`",
        "",
        f"```{language}",
        str(record["signature"]),
        "```",
        "",
        str(record["summary"]),
        "",
        f"[View the pinned source declaration]({record['source_url']})",
    ]
    return "\n".join(lines)


def write_page(
    slug: str,
    title: str,
    description: str,
    records: Sequence[Dict[str, object]],
    commit: str,
) -> Dict[str, object]:
    body = [
        frontmatter(title, description),
        f"Source snapshot: [`{commit}`]({REPOSITORY_URL}/tree/{commit}).",
        "",
        description,
        "",
        "Every entry below is generated from a public binding table or header declaration and links to its immutable source line.",
        "",
        "\n\n".join(render_record(record) for record in records),
        "",
    ]
    (OUTPUT_ROOT / f"{slug}.md").write_text("\n".join(body), encoding="utf-8")
    return {
        "slug": slug,
        "title": title,
        "symbol_count": len(records),
        "source_files": sorted({str(record["path"]) for record in records}),
    }


def write_reference(commit: str, release: str) -> Dict[str, object]:
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    all_python: List[Dict[str, object]] = []
    pages: List[Dict[str, object]] = []

    area = class_records("Area", commit)
    clip_context = class_records("ClipContext", commit)
    clipped_canvas = class_records("ClippedCanvas", commit)
    area_page = area + clip_context + clipped_canvas
    all_python.extend(area_page)
    pages.append(
        write_page(
            "area-clipping",
            "Area and clipping",
            "Rectangle geometry, scoped clip contexts, and clipped canvas operations.",
            area_page,
            commit,
        )
    )

    framebuffer = class_records("FrameBuffer", commit)
    all_python.extend(framebuffer)
    pages.append(
        write_page(
            "framebuffer",
            "FrameBuffer",
            "Native framebuffer construction, drawing, text, blitting, scrolling, and file output.",
            framebuffer,
            commit,
        )
    )

    module = module_records(commit)
    draw = class_records("Draw", commit)
    drawing = draw + [record for record in module if record["name"] in DRAW_FUNCTIONS]
    all_python.extend(draw)
    all_python.extend(record for record in module if record["name"] in DRAW_FUNCTIONS)
    pages.append(
        write_page(
            "drawing",
            "Drawing operations",
            "Object-oriented and module-level shape, text, polygon, and blit operations.",
            drawing,
            commit,
        )
    )

    fonts_images = (
        class_records("Font", commit)
        + class_records("BMP565", commit)
        + [record for record in module if record["name"] in IMAGE_FUNCTIONS]
    )
    all_python.extend(fonts_images)
    pages.append(
        write_page(
            "fonts-images",
            "Fonts and images",
            "Bitmap font rendering, RGB565 bitmap access, and image conversion helpers.",
            fonts_images,
            commit,
        )
    )

    module_formats = [record for record in module if record["name"] in RUNTIME_FUNCTIONS]
    module_formats += constant_records(commit)
    all_python.extend(module_formats)
    pages.append(
        write_page(
            "module-formats",
            "Runtime and pixel formats",
            "Runtime capability probes and the exported framebuffer format constants.",
            module_formats,
            commit,
        )
    )

    native_records: List[Dict[str, object]] = []
    for slug, title, description, paths in HEADER_GROUPS:
        records = [
            record
            for path in paths
            for record in header_prototypes(path, commit)
        ]
        native_records.extend(records)
        pages.append(write_page(slug, title, description, records, commit))

    all_records = all_python + native_records
    source_files = sorted({str(record["path"]) for record in all_records})
    introduction = [
        frontmatter(
            "graphics API reference",
            "Source-linked Python and native C reference for the PyDevices graphics module.",
        ),
        "`graphics` is the all-C drawing module shared by CPython, MicroPython, and CircuitPython builds in the PyDevices stack.",
        "",
        "## Quick start",
        "",
        "```python",
        "from graphics import Area, FrameBuffer, RGB565",
        "",
        "buffer = bytearray(160 * 128 * 2)",
        "canvas = FrameBuffer(buffer, 160, 128, RGB565)",
        "changed = canvas.fill_rect(10, 10, 40, 24, 0xF800)",
        "assert changed == Area(10, 10, 40, 24)",
        "```",
        "",
        "## Reference snapshot",
        "",
        f"- Release line: **{release}**",
        f"- Source commit: [`{commit}`]({REPOSITORY_URL}/tree/{commit})",
        "- License: **MIT**",
        "- Adapter: **CPython binding tables plus public C headers**",
        f"- Python API entries indexed: **{len(all_python)}**",
        f"- Native C functions indexed: **{len(native_records)}**",
        f"- Source files represented: **{len(source_files)}**",
        f"- Tracked C and header files in the repository: **{tracked_native_source_count()}**",
        "",
        "The generated pages expose the user-facing Python names and the lower-level C entry points used by firmware and extension integrations. Every entry links to the exact source snapshot above.",
        "",
        "## Browse the reference",
        "",
    ]
    introduction.extend(
        f"- [{page['title']}]({PROJECT_DOCS_URL}/generated/{page['slug']}.html) — {page['symbol_count']} entries"
        for page in pages
    )
    introduction.append("")
    (OUTPUT_ROOT / "introduction.md").write_text(
        "\n".join(introduction), encoding="utf-8"
    )

    manifest: Dict[str, object] = {
        "schema": "pydevices-graphics-sourcey-reference.v1",
        "repository": REPOSITORY_URL,
        "commit": commit,
        "release": release,
        "license": "MIT",
        "adapter": "cpython-binding-tables+public-c-headers",
        "generated_with": f"sourcey@{SOURCEY_VERSION}",
        "repository_native_source_file_count": tracked_native_source_count(),
        "source_file_count": len(source_files),
        "source_files": source_files,
        "symbol_count": len(all_records),
        "python_symbol_count": len(all_python),
        "native_c_symbol_count": len(native_records),
        "pages": pages,
        "symbols": all_records,
    }
    (OUTPUT_ROOT / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return manifest


def main() -> None:
    manifest = write_reference(git_commit(), release_tag())
    print(
        f"Indexed {manifest['symbol_count']} entries from "
        f"{manifest['source_file_count']} source files at "
        f"{str(manifest['commit'])[:12]}."
    )


if __name__ == "__main__":
    main()
