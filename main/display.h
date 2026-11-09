#pragma once

#include "lvgl.h"

// Bring up backlight, GC9A01 panel, CST816S touch and LVGL.
// Returns the LVGL touch input device.
lv_indev_t *display_init(void);

// Backlight brightness, 0-100 %.
void display_set_brightness(int percent);
