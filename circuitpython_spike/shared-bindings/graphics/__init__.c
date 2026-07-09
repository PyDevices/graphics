// CircuitPython shared-bindings registration for graphics.
#include "shared-bindings/graphics/__init__.h"
#include "py/obj.h"
#include "py/runtime.h"

extern const mp_obj_module_t mp_module_graphics;

MP_REGISTER_MODULE(MP_QSTR_graphics, mp_module_graphics);
