// Host stand-in for esp_lvgl_port.h: there is no second task on the host, so
// the LVGL lock is a no-op.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

static inline bool lvgl_port_lock(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return true;
}

static inline void lvgl_port_unlock(void) {}
