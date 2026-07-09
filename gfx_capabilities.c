/*
 * Platform capability registry.
 * SPDX-License-Identifier: MIT
 */

#include "gfx_capabilities.h"

const char *gfx_framebuf_backend(void) {
    return "native";
}

const char *gfx_implementation(void) {
    return "native_cmod";
}
