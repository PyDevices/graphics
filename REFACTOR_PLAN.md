# graphics cmod refactor plan

**Goal:** One coherent all-C `graphics` module. No Python package inside the cmod. Public API matches `pydisplay/src/lib/graphics` exactly. Build/install chooses either the Python package OR this C module — no runtime fallbacks inside either product.

**Python reference (public contract):** [pydisplay `src/lib/graphics/`](https://github.com/PyDevices/pydisplay/tree/main/src/lib/graphics)

**Do not modify:** upstream `micropython/` / `circuitpython/` clones (leave uncommitted).

---

## Architecture principles

1. **Two products, one import name:** User code always `import graphics`. Never `graphics_native`, never internal `_framebuf*`.
2. **No MP built-in `framebuf`:** Cmod embeds modframebuf-style ops; do not `import framebuf` or delegate to firmware framebuf.
3. **Layer order (dependencies):**
   ```
   gfx_core.h          types, format IDs, shared macros
   gfx_area.c/h        Area type + geometry (clip, intersect, shift, union, …)
   gfx_shapes.c/h      all drawing algorithms; take gfx_canvas_t; return gfx_area_t
   gfx_framebuffer.c/h format pixel ops; FrameBuffer methods call gfx_shapes
   gfx_draw.c/h        Draw type: canvas ref + clip stack; calls same gfx_shapes
   gfx_font.c/h        Font, text8/14/16 (after framebuffer + shapes)
   gfx_bmp565.c/h      BMP565 (port from Python when shapes/fb stable)
   gfx_files.c/h       load_image, save_image, pbm/pgm/bmp (optional phase)
   gfx_capabilities.c/h  capabilities(), framebuf_backend(), implementation()
   gfx_module_mp.c     MicroPython / CircuitPython bindings → MP_REGISTER_MODULE(graphics)
   gfx_module_cpy.c    CPython extension → PyInit_graphics
   ```
4. **Shapes before framebuffer:** Line, ellipse, poly, arc, circle, etc. live in `gfx_shapes.c`. Framebuffer only supplies the draw target (pixel/fill_rect/hline fast paths per format).
5. **Draw peer to FrameBuffer:** Both consume `gfx_shapes`; Draw adds clip wrapping only.
6. **Subclassable FrameBuffer on CPython:** `tp_new` allocates; `tp_init` parses `(buffer, width, height, format[, stride])` — required for pydisplay `displaybuf.DisplayBuffer` without changing displaybuf.

---

## Draw-target abstraction (`gfx_canvas_t`)

Shapes must work on FrameBuffer, display drivers (future), and clipped Draw targets:

```c
typedef struct gfx_canvas {
    void *ctx;
    int width, height;
    int (*pixel)(void *ctx, int x, int y, int c, int set);  /* set=0 get */
    void (*hline)(void *ctx, int x, int y, int w, int c);
    void (*vline)(void *ctx, int x, int y, int h, int c);
    void (*fill_rect)(void *ctx, int x, int y, int w, int h, int c);
} gfx_canvas_t;
```

Optional: blit hooks on canvas for fast paths. Clipped canvas wraps base + `gfx_area_t` clip.

---

## Public API (`graphics.__all__`)

Must match pydisplay `src/lib/graphics/__init__.py`:

- Constants: `MONO_VLSB`, `MONO_HLSB`, `MONO_HMSB`, `RGB565`, `GS2_HMSB`, `GS4_HMSB`, `GS8`, `RGB888`
- Types: `Area`, `FrameBuffer`, `Font`, `BMP565`, `Draw`
- Module functions: shape fns, `text`/`text8`/`text14`/`text16`, I/O fns, `capabilities`, `framebuf_backend`, `implementation`
- FrameBuffer methods return `Area` where Python `_framebuf_plus` does
- `capabilities()` reports `implementation: "native_cmod"`, `framebuf: "native"`

Sync gaps with Python tree: add `RGB888` and `ellipse` to both `__all__` lists when binding exports are wired.

---

## Implementation phases

### Phase 0 — Scaffold & delete old layout

- [ ] Add `REFACTOR_PLAN.md` (this file) — done
- [ ] Create `gfx_core.h`, stub headers for each layer
- [ ] Remove `py/graphics/` entire tree from cmod (after Phase 4 exports work — or move to `_deprecated_py/` until cutover)
- [ ] Remove `gfxpy/` placeholder if unused
- [ ] Stop freezing Python graphics in `workspace manifest.py` when usermod linked (comment or conditional — document in README)
- [ ] Update `setup.py`: extension module name `graphics`, not `graphics_native`; no `packages=["graphics"]` from py/

### Phase 1 — Area (`gfx_area.c`)

- [ ] Port/consolidate from `gfx_core.h` + `gfx_area_mp.c` + `graphics_native_cpy.c` Area + pydisplay `_area.py` behavior
- [ ] MP, CP, CPython types all expose `graphics.Area`
- [ ] Methods: contains, contains_area, intersects, touches_or_intersects, shift, clip, offset, inset, `__add__`, `__eq__`, `__iter__`, `__repr__`, `__str__`
- [ ] Tests: extend `test_area.py`

### Phase 2 — Shapes (`gfx_shapes.c`)

- [ ] Implement draw-target vtable + clipped canvas
- [ ] Port algorithms from pydisplay `_shapes.py` and/or `modframebuf.c` (line, rect, fill_rect, hline, vline, ellipse, poly, arc, circle, triangle, round_rect, gradient_rect, polygon, blit, blit_rect, blit_transparent)
- [ ] All return `gfx_area_t`; module-level C functions mirror `graphics.line`, etc.
- [ ] Unit tests with a mock canvas (memory buffer + simple RGB565 callback)

### Phase 3 — Framebuffer (`gfx_framebuffer.c`)

- [ ] Port format ops from `graphics_bundle.c` / `graphics_native_cpy.c` / `modframebuf.c` into ONE file
- [ ] FrameBuffer methods delegate to `gfx_shapes` where appropriate; keep format-specific fill_rect fast paths
- [ ] `tp_new`/`tp_init` split on CPython; MP `make_new`/`__init__` parity
- [ ] Properties: width, height, buffer, format, color_depth
- [ ] RGB888: implement in C or document as phase 5 (Python lib has RGB888=7 extension)
- [ ] Tests: extend `test_graphics.py`; add `test_subclass.py` for super().__init__

### Phase 4 — Draw (`gfx_draw.c`)

- [ ] `Draw(canvas)` constructor; clip stack; context manager clip on MP/CP if feasible else clip push/pop methods
- [ ] Every Draw method → same gfx_shapes entry as module-level + FrameBuffer
- [ ] Tests: basic clip + rect

### Phase 5 — Module registration & bindings

- [ ] Replace `graphics_bundle.c` + `graphics_native_cpy.c` with `gfx_module_mp.c` + `gfx_module_cpy.c` (thin)
- [ ] Register module as `graphics` (MP_QSTR_graphics), not `graphics_native`
- [ ] Export all public symbols on module dict
- [ ] Update `micropython.mk`, `circuitpython.mk`, CP spike bindings under `circuitpython_spike/`
- [ ] Update `workspace manifest.py`: remove `package("graphics", base_path="graphics/py")` — use C module only when building with cmod

### Phase 6 — Font, BMP565, files, capabilities

- [ ] `gfx_font.c` — Font class, text/text8/text14/text16
- [ ] `gfx_bmp565.c` — BMP565 type
- [ ] `gfx_files.c` — load_image, save_image, pbm/pgm/bmp helpers (port from `_files.py`)
- [ ] `gfx_capabilities.c` — capabilities(), framebuf_backend(), implementation()

### Phase 7 — Integration & cleanup

- [ ] Delete obsolete: `graphics_bundle.c`, `graphics_native_cpy.c`, `gfx_area_mp.c` (merged), entire `py/` tree
- [ ] Update README.md, PUBLISHING.md
- [ ] `pip install -e .` → `import graphics` is C only
- [ ] Rebuild MP unix: `build_mp.sh (optional workspace wrapper) --port unix --variant standard`
- [ ] Run `test_area.py`, `test_graphics.py`, `test_subclass.py`
- [ ] pydisplay smoke: `font_simpletest3`, `displaybuf_simpletest` with cmod wheel (optional — parent handles matrix)

---

## Success criteria (definition of done)

1. Zero `.py` files shipped as the graphics module from this repo (tests/scripts OK).
2. `import graphics` works on CPython after `pip install -e .` with no Python graphics package.
3. MP unix build with cmod: `import graphics` → C module; `graphics.framebuf_backend() == "native"`.
4. `test_graphics.py` and `test_area.py` pass.
5. `test_subclass.py`: subclass FrameBuffer, `super().__init__(buf,w,h,fmt)`, draw succeeds.
6. Public symbol set matches pydisplay `graphics.__all__` (document any intentional deferrals).
7. No references to `graphics_native` in user-facing docs (internal CP spike paths may rename gradually).

---

## Risks & notes

- **Manifest conflict:** pydisplay MP builds currently freeze `graphics/py` AND link usermod. After cutover, manifest must choose one. Coordinate with `workspace manifest.py` line `package("graphics", base_path="graphics/py")`.
- **CPython pip name:** May remain `graphics-cmod` on PyPI but import name `graphics`.
- **CircuitPython:** Update spike bindings to match module rename.
- **pydisplay `src/lib/graphics`:** Unchanged by this work; remains the Python implementation.

---

## Agent handoff

Implement phases 0→7 in order. Commit to the graphics repo on meaningful phase boundaries. Do not commit micropython/circuitpython upstream. Report back with: files added/removed, test results, remaining public API gaps vs pydisplay.
