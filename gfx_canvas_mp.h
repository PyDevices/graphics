/*
 * MicroPython canvas resolution (native FrameBuffer + Python duck typing).
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_CANVAS_MP_H
#define GFX_CANVAS_MP_H

#include "py/obj.h"
#include "gfx_bindings_mp.h"

bool mp_canvas_resolve(mp_obj_t target, mp_canvas_slot_t *slot);

#endif
