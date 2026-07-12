# SPDX-License-Identifier: MIT
"""Parity probe: kwargs + missing methods vs pydisplay lib/graphics contract."""

try:
    import graphics
except ImportError as e:
    print("FAIL: import graphics:", e)
    raise SystemExit(1)

results = {}


def check(name, fn):
    try:
        fn()
        results[name] = "pass"
        print("PASS:", name)
    except Exception as e:
        results[name] = "fail:" + repr(e)
        print("FAIL:", name, e)


impl = graphics.implementation()
backend = graphics.framebuf_backend()
print("implementation:", impl)
print("framebuf_backend:", backend)
assert impl == "native_cmod", impl
assert backend == "native", backend

W, H = 64, 64
buf = bytearray(W * H * 2)
fb = graphics.FrameBuffer(buf, W, H, graphics.RGB565)
fb.fill(0)


def t_text_module():
    graphics.text8(fb, "A", 0, 0, c=0xFFFF, scale=2, inverted=False)
    graphics.text14(fb, "B", 0, 16, c=0xFFFF, scale=1)
    graphics.text16(fb, "C", 0, 32, scale=1, inverted=False)
    graphics.text(fb, "D", 0, 48, c=1, height=8, scale=1)


def t_text_fb():
    assert hasattr(fb, "text8") and hasattr(fb, "text14") and hasattr(fb, "text16")
    fb.text8("E", 8, 0, c=0xFFFF, scale=1)
    fb.text14("F", 8, 16, scale=1, inverted=False)
    fb.text16("G", 8, 32, c=0xFFFF)
    fb.text("H", 8, 48, height=16, scale=1)


def t_text_draw():
    d = graphics.Draw(fb)
    assert hasattr(d, "text8") and hasattr(d, "text14") and hasattr(d, "text16")
    d.text8("I", 16, 0, c=0xFFFF, scale=1)
    d.text14("J", 16, 16, scale=1)
    d.text16("K", 16, 32, inverted=False)
    d.text("L", 16, 48, height=8)


def t_fill_kwargs():
    graphics.rect(fb, 0, 0, 10, 10, 0xF800, f=True)
    graphics.round_rect(fb, 12, 0, 10, 10, 2, 0x07E0, f=True)
    graphics.ellipse(fb, 30, 5, 4, 3, 0x001F, f=True)
    graphics.triangle(fb, 40, 0, 50, 0, 45, 8, 0xFFFF, f=True)
    fb.rect(0, 12, 8, 8, 0xF800, f=True)
    fb.round_rect(10, 12, 8, 8, 2, 0x07E0, f=True)
    fb.circle(24, 16, 4, 0x001F, f=True)
    d = graphics.Draw(fb)
    d.rect(30, 12, 8, 8, 0xF800, f=True)
    d.circle(44, 16, 3, 0x07E0, f=True)


def t_gradient_polygon_blit():
    graphics.gradient_rect(fb, 0, 24, 20, 8, 0xF800, c2=0x001F, vertical=True)
    fb.gradient_rect(22, 24, 20, 8, 0x07E0, c2=0xF800, vertical=False)
    pts = [(0, 0), (6, 0), (3, 6)]
    graphics.polygon(fb, pts, 50, 24, 0xFFFF, angle=0.5)
    fb.polygon(pts, 50, 36, 0xFFFF, angle=0.0, center_x=0, center_y=0)
    src = graphics.FrameBuffer(bytearray(4 * 4 * 2), 4, 4, graphics.RGB565)
    src.fill(0xF800)
    graphics.blit(fb, src, 0, 40, key=-1)
    fb.blit(src, 8, 40, key=0)


def t_missing_fb_methods():
    for name in (
        "arc",
        "triangle",
        "gradient_rect",
        "polygon",
        "blit_rect",
        "blit_transparent",
        "save",
        "text8",
        "text14",
    ):
        assert hasattr(fb, name), name
    fb.arc(20, 50, 6, 0, 90, 0xFFFF)
    fb.triangle(30, 44, 38, 44, 34, 52, 0xF800, f=False)
    raw = bytearray(4 * 2 * 2)
    fb.blit_rect(raw, 40, 44, 4, 2)
    fb.blit_transparent(raw, 46, 44, 4, 2, 0)


def t_missing_draw_methods():
    d = graphics.Draw(fb)
    for name in (
        "text8",
        "text16",
        "arc",
        "blit",
        "blit_rect",
        "blit_transparent",
        "ellipse",
        "gradient_rect",
        "polygon",
        "triangle",
        "clip",
    ):
        assert hasattr(d, name), name
    d.arc(10, 10, 5, 0, 45, 0xFFFF)
    d.ellipse(20, 10, 4, 3, 0xF800, f=True)
    d.gradient_rect(30, 8, 10, 6, 0xF800, c2=0x001F, vertical=True)
    d.triangle(0, 0, 5, 0, 2, 5, 0x07E0, f=True)
    src = graphics.FrameBuffer(bytearray(2 * 2 * 2), 2, 2, graphics.RGB565)
    src.fill(1)
    d.blit(src, 50, 50, key=-1)


def t_font_kwargs():
    font = graphics.Font(height=8)
    font.text(fb, "Z", 0, 56, 0xFFFF, scale=1, inverted=False)


def t_area_return():
    a = fb.fill_rect(0, 0, 2, 2, 0)
    assert isinstance(a, graphics.Area), type(a)
    a2 = graphics.text8(fb, "x", 0, 0, 1)
    assert isinstance(a2, graphics.Area), type(a2)


check("text_module", t_text_module)
check("text_fb", t_text_fb)
check("text_draw", t_text_draw)
check("fill_kwargs", t_fill_kwargs)
check("gradient_polygon_blit", t_gradient_polygon_blit)
check("missing_fb_methods", t_missing_fb_methods)
check("missing_draw_methods", t_missing_draw_methods)
check("font_kwargs", t_font_kwargs)
check("area_return", t_area_return)

failed = [k for k, v in results.items() if not v.startswith("pass")]
if failed:
    print("FAILED:", ", ".join(failed))
    raise SystemExit(1)
print("test_parity_kwargs: ok")
