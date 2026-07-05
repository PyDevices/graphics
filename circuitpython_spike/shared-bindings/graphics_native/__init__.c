// CircuitPython shared-bindings registration for graphics_native.
#include "shared-bindings/graphics_native/__init__.h"
#include "py/obj.h"
#include "py/runtime.h"

extern const mp_obj_module_t mp_module_graphics_native;

MP_REGISTER_MODULE(MP_QSTR_graphics_native, mp_module_graphics_native);
