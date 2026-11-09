// Controls for the stubbed hardware (host renderer only).
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_system.h"

// Battery as battery_sense.h would report it.
void host_battery_set(int percent, uint16_t mv, bool charging);
void host_set_reset_reason(esp_reset_reason_t reason);
// Forget everything the fake NVS holds.
void host_storage_clear(void);
