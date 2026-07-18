# MicroPython CMake glue for graphics (ESP32, RP2, …).
#
# Make-based ports use micropython.mk in this directory. CMake-based ports
# (esp32, rp2, …) discover this file via USER_C_MODULES / an aggregator
# micropython.cmake that includes sibling modules.
#
# Point USER_C_MODULES at this repo (or this file) directly, e.g.:
#   idf.py build -DUSER_C_MODULES=<path to graphics>
# Or a semicolon-separated list with other modules (no aggregator required):
#   -DUSER_C_MODULES="<path to graphics>;<path to displayif>"

set(GRAPHICS_MOD_DIR ${CMAKE_CURRENT_LIST_DIR})

add_library(graphics INTERFACE)

target_sources(graphics INTERFACE
    ${GRAPHICS_MOD_DIR}/gfx_module_mp.c
    ${GRAPHICS_MOD_DIR}/gfx_bindings_mp.c
    ${GRAPHICS_MOD_DIR}/gfx_canvas_mp.c
    ${GRAPHICS_MOD_DIR}/gfx_framebuffer.c
    ${GRAPHICS_MOD_DIR}/gfx_shapes.c
    ${GRAPHICS_MOD_DIR}/gfx_draw.c
    ${GRAPHICS_MOD_DIR}/gfx_font.c
    ${GRAPHICS_MOD_DIR}/gfx_bmp565.c
    ${GRAPHICS_MOD_DIR}/gfx_files.c
    ${GRAPHICS_MOD_DIR}/gfx_capabilities.c
    ${GRAPHICS_MOD_DIR}/gfx_area_mp.c
)

target_include_directories(graphics INTERFACE ${GRAPHICS_MOD_DIR})
target_compile_options(graphics INTERFACE
    -Wno-unused-function
    -Wno-sign-compare
    -Wno-unused-const-variable
)

# libm: required on some desktop ports; ESP-IDF already links libm into the app.
if(NOT ESP_PLATFORM)
    target_link_libraries(graphics INTERFACE m)
endif()

target_link_libraries(usermod INTERFACE graphics)
