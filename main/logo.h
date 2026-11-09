#pragma once

#include "lvgl.h"

// OpenPuck brand colors (see docs/brand/README.md).
#define LOGO_COLOR_RING  lv_color_hex(0xF5F2EA)
#define LOGO_COLOR_BEAD  lv_color_hex(0xFF5A36)

// Draw the OpenPuck mark (open ring + bead) with LVGL primitives, no bitmap.
// `diameter` is the outer diameter of the ring in pixels; 192 matches the
// 240x240 reference in docs/brand/openpuck-device.svg. Returns a
// non-clickable container of that size, so it doesn't block touches beneath.
lv_obj_t *logo_create(lv_obj_t *parent, int32_t diameter);
