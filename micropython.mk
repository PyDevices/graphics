# MicroPython user C module glue for graphics.
GRAPHICS_MOD_DIR := $(USERMOD_DIR)

CFLAGS_USERMOD += -I$(GRAPHICS_MOD_DIR) -Wno-unused-function -Wno-sign-compare -Wno-unused-const-variable
LDFLAGS_USERMOD += -lm

SRC_USERMOD_C += \
    $(GRAPHICS_MOD_DIR)/gfx_module_mp.c \
    $(GRAPHICS_MOD_DIR)/gfx_bindings_mp.c \
    $(GRAPHICS_MOD_DIR)/gfx_canvas_mp.c \
    $(GRAPHICS_MOD_DIR)/gfx_framebuffer.c \
    $(GRAPHICS_MOD_DIR)/gfx_shapes.c \
    $(GRAPHICS_MOD_DIR)/gfx_draw.c \
    $(GRAPHICS_MOD_DIR)/gfx_font.c \
    $(GRAPHICS_MOD_DIR)/gfx_bmp565.c \
    $(GRAPHICS_MOD_DIR)/gfx_files.c \
    $(GRAPHICS_MOD_DIR)/gfx_capabilities.c \
    $(GRAPHICS_MOD_DIR)/gfx_area_mp.c
