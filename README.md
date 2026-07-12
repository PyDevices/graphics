# graphics

Native all-C graphics cmod for **MicroPython**, **CircuitPython**, and **CPython**.

`import graphics` loads a single C module — no Python package inside this repo.

Provides:

- `Area` — rectangle geometry helper
- `FrameBuffer` — framebuf-compatible drawing surface (returns `Area` bounds)
- Format constants: `MONO_VLSB`, `MONO_HLSB`, `MONO_HMSB`, `RGB565`, `GS2_HMSB`, `GS4_HMSB`, `GS8`, `RGB888`
- `framebuf_backend()`, `capabilities()`, `implementation()`

## Layout

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

## 🚀 Build & test

### CPython

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

## pydisplay integration

pydisplay's Python `src/lib/graphics` remains the pure-Python implementation. When this cmod is installed or linked, `graphics.framebuf_backend()` reports `native` and `graphics.implementation()` reports `native_cmod`.

## License

MIT (framebuf algorithms derived from MicroPython `extmod/modframebuf.c`, Damien P. George).
