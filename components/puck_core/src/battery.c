#include "puck_battery.h"

#include <stddef.h>

#define HYSTERESIS_PERCENT 2

typedef struct {
    uint16_t mv;
    uint8_t percent;
} curve_point_t;

// Descending in mV; see the table in puck_battery.h.
static const curve_point_t curve[] = {
    {4200, 100}, {4100, 90}, {4000, 80}, {3900, 65}, {3800, 50}, {3750, 40},
    {3700, 30},  {3650, 20}, {3600, 12}, {3500, 5},  {3400, 2},  {3300, 0},
};
#define CURVE_LEN (sizeof curve / sizeof curve[0])

uint16_t puck_battery_mv_from_adc_mv(uint16_t adc_mv)
{
    uint32_t mv = (uint32_t)adc_mv * PUCK_BATTERY_DIVIDER;
    return mv > UINT16_MAX ? UINT16_MAX : (uint16_t)mv;
}

uint8_t puck_battery_percent(uint16_t battery_mv)
{
    if (battery_mv >= curve[0].mv) {
        return curve[0].percent;
    }
    if (battery_mv <= curve[CURVE_LEN - 1].mv) {
        return curve[CURVE_LEN - 1].percent;
    }

    size_t i = 1;
    while (battery_mv < curve[i].mv) {
        i++;
    }
    // Now curve[i].mv <= battery_mv < curve[i - 1].mv.
    uint32_t span_mv = (uint32_t)(curve[i - 1].mv - curve[i].mv);
    uint32_t span_percent = (uint32_t)(curve[i - 1].percent - curve[i].percent);
    uint32_t above_mv = (uint32_t)(battery_mv - curve[i].mv);
    // Adding half the divisor before dividing rounds to nearest, with exact
    // halves (e.g. 72.5) going up.
    uint32_t offset = (above_mv * span_percent + span_mv / 2) / span_mv;
    return (uint8_t)(curve[i].percent + offset);
}

bool puck_battery_low(uint8_t percent)
{
    return percent <= PUCK_BATTERY_LOW_PERCENT;
}

void puck_battery_filter_init(puck_battery_filter_t *filter)
{
    *filter = (puck_battery_filter_t) {0};
}

uint8_t puck_battery_filter_update(puck_battery_filter_t *filter, uint16_t battery_mv)
{
    if (!filter->primed) {
        filter->avg_mv_x8 = (uint32_t)battery_mv * 8;
        filter->primed = true;
        filter->shown_percent = puck_battery_percent(battery_mv);
        return filter->shown_percent;
    }

    // Signed so a reading below the average can pull it down.
    int32_t avg_x8 = (int32_t)filter->avg_mv_x8;
    avg_x8 += (int32_t)battery_mv - avg_x8 / 8;
    filter->avg_mv_x8 = (uint32_t)avg_x8;

    uint8_t percent = puck_battery_percent((uint16_t)((filter->avg_mv_x8 + 4) / 8));
    int diff = percent - filter->shown_percent;
    bool at_extreme = percent == 0 || percent == 100;
    if (diff >= HYSTERESIS_PERCENT || diff <= -HYSTERESIS_PERCENT || (at_extreme && diff != 0)) {
        filter->shown_percent = percent;
    }
    return filter->shown_percent;
}
