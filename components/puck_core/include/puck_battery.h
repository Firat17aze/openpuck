// Battery level from a voltage reading.
//
// Hardware: the battery reaches the ADC through a 1:4 divider, so
// battery mV = ADC mV * PUCK_BATTERY_DIVIDER. Single-cell lithium (4.2 V full).
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PUCK_BATTERY_DIVIDER     4
#define PUCK_BATTERY_LOW_PERCENT 10

uint16_t puck_battery_mv_from_adc_mv(uint16_t adc_mv);

// Percent (0-100) along a typical single-cell lithium discharge curve,
// piecewise-linear between these points (mV -> %):
//   4200 100, 4100 90, 4000 80, 3900 65, 3800 50, 3750 40, 3700 30,
//   3650 20, 3600 12, 3500 5, 3400 2, 3300 0
// Clamped to 0 below 3300 mV and 100 above 4200 mV.
uint8_t puck_battery_percent(uint16_t battery_mv);

bool puck_battery_low(uint8_t percent);

// Smooths readings so the shown level doesn't jitter: an exponential moving
// average of the voltage (weight 1/8 for each new reading; the first reading
// is taken as-is), then hysteresis: the shown percent only changes once the
// smoothed percent differs from it by at least 2 points, or reaches 0 or 100.
typedef struct {
    uint32_t avg_mv_x8;  // average * 8
    bool primed;
    uint8_t shown_percent;
} puck_battery_filter_t;

void puck_battery_filter_init(puck_battery_filter_t *filter);
// Feed a battery reading (mV); returns the percent to show.
uint8_t puck_battery_filter_update(puck_battery_filter_t *filter, uint16_t battery_mv);

#ifdef __cplusplus
}
#endif
