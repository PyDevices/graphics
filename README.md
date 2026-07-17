# graphics-cmod

Native all-C **graphics** module for MicroPython, CircuitPython, and CPython. Import as `graphics`.

> **Pip name:** `graphics-cmod` · **Import:** `import graphics`

Prefer this wheel on desktop/Android when available. The pure-Python alternative is [`pydisplay-graphics`](https://test.pypi.org/project/pydisplay-graphics/) (same import name).

## Install

### CPython (TestPyPI)

```bash
pip install \
  -i https://test.pypi.org/simple/ \
  --extra-index-url https://pypi.org/simple/ \
  graphics-cmod
```

### Quick start

```python
import graphics
from graphics import FrameBuffer, RGB565

fb = FrameBuffer(bytearray(160 * 128 * 2), 160, 128, RGB565)
fb.fill(0)
fb.fill_rect(10, 10, 40, 40, 0xF800)
assert graphics.framebuf_backend() == "native"
print(graphics.implementation())
```

## What you get

- `Area` — rectangle geometry helper
- `FrameBuffer` — framebuf-compatible drawing surface (returns `Area` bounds)
- Format constants: `MONO_VLSB`, `MONO_HLSB`, `MONO_HMSB`, `RGB565`, `GS2_HMSB`, `GS4_HMSB`, `GS8`, `RGB888`
- `framebuf_backend()`, `capabilities()`, `implementation()`

Firmware / editable builds: see **Build from source** below.

## Links

- [Source](https://github.com/PyDevices/graphics)
- [Issues](https://github.com/PyDevices/graphics/issues)
- Related: [pydisplay-graphics](https://test.pypi.org/project/pydisplay-graphics/), [pydisplay](https://github.com/PyDevices/pydisplay)

## License

MIT (framebuf algorithms derived from MicroPython `extmod/modframebuf.c`, Damien P. George).

---

## Build from source

### Layout

```
graphics/
  gfx_core.h           # types, format IDs, Area geometry
  gfx_framebuffer.c/h  # format pixel ops
  gfx_shapes.c/h       # drawing algorithms via gfx_canvas_t
  gfx_draw.c/h         # Draw clip stack (C core)
  gfx_font.c/h         # text8
  gfx_capabilities.c/h # capabilities reporting
  gfx_area_mp.c        # MicroPython Area bindings
  gfx_module_mp.c      # MicroPython module registration
  gfx_module_cpy.c     # CPython module registration
  micropython.mk
  circuitpython.mk
  test_area.py
  test_graphics.py
  test_subclass.py
```

### CPython (editable)

```bash
python3 -m venv .venv
.venv/bin/pip install -e .
.venv/bin/python test_area.py
.venv/bin/python test_graphics.py
.venv/bin/python test_subclass.py
```

### MicroPython

Clone as a sibling of `micropython/`:

```
workspace/
  graphics/       ← this repo
  micropython/
```

There is no `graphics/manifest.py` — the C usermod is the only graphics path when linked in.

```bash
cd micropython/ports/unix
make submodules
make USER_C_MODULES=../../..
cd ../../..
./micropython/ports/unix/build-standard/micropython graphics/test_area.py
./micropython/ports/unix/build-standard/micropython graphics/test_graphics.py
./micropython/ports/unix/build-standard/micropython graphics/test_subclass.py
```

([cmods](https://github.com/PyDevices/cmods) is an optional convenience workspace with `./build_mp.sh`; it is not required.)

### pydisplay integration

pydisplay's Python `src/lib/graphics` remains the pure-Python implementation. When this cmod is installed or linked, `graphics.framebuf_backend()` reports `native` and `graphics.implementation()` reports `native_cmod`.
