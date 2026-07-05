# MicroPython user C module glue for graphics_native.
GRAPHICS_MOD_DIR := $(USERMOD_DIR)

CFLAGS_USERMOD += -I$(GRAPHICS_MOD_DIR) -Wno-unused-function -Wno-sign-compare -Wno-unused-const-variable

SRC_USERMOD_C += \
    $(GRAPHICS_MOD_DIR)/graphics_bundle.c \
    $(GRAPHICS_MOD_DIR)/gfx_area_mp.c
