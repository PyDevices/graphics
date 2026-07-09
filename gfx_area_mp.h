/*
 * MicroPython Area type bindings.
 * SPDX-License-Identifier: MIT
 */
#ifndef GFX_AREA_MP_H
#define GFX_AREA_MP_H

#include "py/obj.h"
#include "gfx_core.h"

extern const mp_obj_type_t mp_type_area;

mp_obj_t gfx_area_mp_from_gfx(const gfx_area_t *a);

#endif
