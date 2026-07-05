# graphics

Native graphics cmod for **MicroPython**, **CircuitPython**, and **CPython**.

Provides:

- `graphics_native` C module: framebuf-compatible `FrameBuffer`, native `Area`, format constants
- Frozen / pip `graphics` package: `_framebuf`, `_framebuf_plus`, `_area`, capability reporting

## Layout

```
graphics/
  py/graphics/           # frozen MP package (graphics._framebuf, _area, …)
  gfxpy/                 # CPython packaging placeholder
  graphics_bundle.c      # MP/CP FrameBuffer (from micropython framebuf)
  gfx_area_mp.c          # MP/CP native Area
  graphics_native_cpy.c  # CPython extension
  micropython.mk
  circuitpython.mk
  test_graphics.py
  test_area.py
  scripts/benchmark_framebuf.py
```

## Build & test

### MicroPython (from cmods workspace)

```bash
ln -sfn ../graphics graphics   # if not already linked
# manifest.py: package("graphics", base_path="graphics/py", opt=3)
./build_mp.sh --port unix --variant standard
./micropython/ports/unix/build-standard/micropython ../graphics/test_area.py
./micropython/ports/unix/build-standard/micropython ../graphics/test_graphics.py
```

### CPython

```bash
python3 -m venv .venv
.venv/bin/pip install -e .
.venv/bin/python test_area.py      # run from /tmp or after pip install
.venv/bin/python test_graphics.py
.venv/bin/python scripts/benchmark_framebuf.py
```

### CircuitPython

```bash
./apply_cp_unix_graphics_patches.sh
# then build unix/coverage via lv_circuitpython_mod/build_cp.sh
```

## pydisplay integration

`pydisplay` imports `from graphics._framebuf import FrameBuffer, RGB565, …`. With this cmod installed, `graphics.framebuf_backend()` reports `native`.

## License

MIT (framebuf algorithms derived from MicroPython `extmod/modframebuf.c`, Damien P. George).
