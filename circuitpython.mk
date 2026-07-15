# CircuitPython build glue for graphics (unix coverage).
GRAPHICS_MOD_DIR ?= $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

CFLAGS += -I$(GRAPHICS_MOD_DIR) -DCIRCUITPY_GRAPHICS=1
QSTR_DEFS += $(GRAPHICS_MOD_DIR)/graphics_qstrdefs.h

GRAPHICS_SOURCES := \
    $(GRAPHICS_MOD_DIR)/gfx_module_mp.c \
    $(GRAPHICS_MOD_DIR)/gfx_bindings_mp.c \
    $(GRAPHICS_MOD_DIR)/gfx_canvas_mp.c \
    $(GRAPHICS_MOD_DIR)/gfx_clip_mp.c \
    $(GRAPHICS_MOD_DIR)/gfx_framebuffer.c \
    $(GRAPHICS_MOD_DIR)/gfx_shapes.c \
    $(GRAPHICS_MOD_DIR)/gfx_draw.c \
    $(GRAPHICS_MOD_DIR)/gfx_font.c \
    $(GRAPHICS_MOD_DIR)/gfx_bmp565.c \
    $(GRAPHICS_MOD_DIR)/gfx_files.c \
    $(GRAPHICS_MOD_DIR)/gfx_capabilities.c \
    $(GRAPHICS_MOD_DIR)/gfx_area_mp.c

GRAPHICS_SUPPRESS_CFLAGS := -Wno-unused-function -Wno-sign-compare
$(foreach _gfx,$(GRAPHICS_SOURCES),$(eval $(BUILD)/$(_gfx:.c=.o): CFLAGS += $(GRAPHICS_SUPPRESS_CFLAGS)))

SRC_C += $(GRAPHICS_SOURCES)
