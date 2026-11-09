// Battery voltage sensing. A low-priority task samples the ADC every few
// seconds; the readers below are safe to call from any task.
#pragma once

#include <stdbool.h>
#include <stdint.h>

// Set up the ADC and start the sampling task.
void battery_sense_start(void);

// Smoothed charge level 0-100, or -1 until the first reading.
int battery_sense_percent(void);

// Smoothed battery voltage in mV, or 0 until the first reading.
uint16_t battery_sense_mv(void);

// True while the charger reports it is charging (not when unplugged or full).
bool battery_sense_charging(void);
