#include "puck_battery.h"
#include "unity.h"

#include <stddef.h>

static const struct {
    uint16_t mv;
    uint8_t percent;
} curve[] = {
    {4200, 100}, {4100, 90}, {4000, 80}, {3900, 65}, {3800, 50}, {3750, 40},
    {3700, 30},  {3650, 20}, {3600, 12}, {3500, 5},  {3400, 2},  {3300, 0},
};

static puck_battery_filter_t filter;

void setUp(void)
{
    puck_battery_filter_init(&filter);
}

void tearDown(void) {}

static void feed(uint16_t mv, int times)
{
    for (int i = 0; i < times; i++) {
        puck_battery_filter_update(&filter, mv);
    }
}

static void test_divider_multiplies_adc_reading(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, puck_battery_mv_from_adc_mv(0));
    TEST_ASSERT_EQUAL_UINT16(4000, puck_battery_mv_from_adc_mv(1000));
    TEST_ASSERT_EQUAL_UINT16(4200, puck_battery_mv_from_adc_mv(1050));
}

static void test_divider_saturates_at_uint16_max(void)
{
    TEST_ASSERT_EQUAL_UINT16(65532, puck_battery_mv_from_adc_mv(16383));
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, puck_battery_mv_from_adc_mv(16384));
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, puck_battery_mv_from_adc_mv(UINT16_MAX));
}

static void test_every_curve_point_maps_exactly(void)
{
    for (size_t i = 0; i < sizeof curve / sizeof curve[0]; i++) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(curve[i].percent, puck_battery_percent(curve[i].mv),
                                        "curve point");
    }
}

static void test_percent_clamps_above_full_and_below_empty(void)
{
    TEST_ASSERT_EQUAL_UINT8(100, puck_battery_percent(4201));
    TEST_ASSERT_EQUAL_UINT8(100, puck_battery_percent(UINT16_MAX));
    TEST_ASSERT_EQUAL_UINT8(0, puck_battery_percent(3299));
    TEST_ASSERT_EQUAL_UINT8(0, puck_battery_percent(0));
}

static void test_percent_interpolates_between_points(void)
{
    TEST_ASSERT_EQUAL_UINT8(95, puck_battery_percent(4150));
    TEST_ASSERT_EQUAL_UINT8(98, puck_battery_percent(4180));
    TEST_ASSERT_EQUAL_UINT8(45, puck_battery_percent(3775));
    TEST_ASSERT_EQUAL_UINT8(9, puck_battery_percent(3550));  // 8.5 -> 9
    TEST_ASSERT_EQUAL_UINT8(0, puck_battery_percent(3301));  // 0.02 -> 0
}

static void test_percent_rounds_to_nearest_with_halves_up(void)
{
    TEST_ASSERT_EQUAL_UINT8(73, puck_battery_percent(3950));  // 72.5 -> 73
    TEST_ASSERT_EQUAL_UINT8(72, puck_battery_percent(3949));  // 72.35 -> 72
    TEST_ASSERT_EQUAL_UINT8(73, puck_battery_percent(3951));  // 72.65 -> 73
    TEST_ASSERT_EQUAL_UINT8(1, puck_battery_percent(3325));   // 0.5 -> 1
    TEST_ASSERT_EQUAL_UINT8(1, puck_battery_percent(3349));   // 0.98 -> 1
    TEST_ASSERT_EQUAL_UINT8(2, puck_battery_percent(3401));   // 2.03 -> 2
}

static void test_percent_never_decreases_as_voltage_rises(void)
{
    uint8_t prev = 0;
    for (uint16_t mv = 3200; mv <= 4300; mv++) {
        uint8_t p = puck_battery_percent(mv);
        TEST_ASSERT_TRUE(p >= prev);
        prev = p;
    }
}

static void test_low_threshold_boundaries(void)
{
    TEST_ASSERT_TRUE(puck_battery_low(0));
    TEST_ASSERT_TRUE(puck_battery_low(PUCK_BATTERY_LOW_PERCENT));
    TEST_ASSERT_FALSE(puck_battery_low(PUCK_BATTERY_LOW_PERCENT + 1));
    TEST_ASSERT_FALSE(puck_battery_low(100));
}

static void test_filter_init_clears_state(void)
{
    puck_battery_filter_t f = {.avg_mv_x8 = 123, .primed = true, .shown_percent = 55};
    puck_battery_filter_init(&f);
    TEST_ASSERT_EQUAL_UINT32(0, f.avg_mv_x8);
    TEST_ASSERT_FALSE(f.primed);
    TEST_ASSERT_EQUAL_UINT8(0, f.shown_percent);
}

static void test_filter_first_reading_is_taken_as_is(void)
{
    TEST_ASSERT_EQUAL_UINT8(73, puck_battery_filter_update(&filter, 3950));
    TEST_ASSERT_TRUE(filter.primed);
    TEST_ASSERT_EQUAL_UINT32(3950u * 8, filter.avg_mv_x8);
}

static void test_filter_averages_with_weight_one_eighth(void)
{
    puck_battery_filter_update(&filter, 4000);
    puck_battery_filter_update(&filter, 3200);
    TEST_ASSERT_EQUAL_UINT32(3900u * 8, filter.avg_mv_x8);  // 4000 * 7/8 + 3200 / 8
    puck_battery_filter_update(&filter, 4100);
    TEST_ASSERT_EQUAL_UINT32(3900u * 8 - 3900 + 4100, filter.avg_mv_x8);
}

static void test_filter_single_spike_barely_moves_display(void)
{
    puck_battery_filter_update(&filter, 3800);
    TEST_ASSERT_EQUAL_UINT8(50, puck_battery_filter_update(&filter, 3750));
    TEST_ASSERT_EQUAL_UINT8(50, puck_battery_filter_update(&filter, 3800));

    puck_battery_filter_init(&filter);
    puck_battery_filter_update(&filter, 3800);
    TEST_ASSERT_EQUAL_UINT8(50, puck_battery_filter_update(&filter, 3850));
    TEST_ASSERT_EQUAL_UINT8(50, puck_battery_filter_update(&filter, 3800));
}

static void test_filter_one_point_drift_does_not_change_display(void)
{
    puck_battery_filter_update(&filter, 3800);  // 50
    feed(3796, 200);                            // settles at 49
    TEST_ASSERT_EQUAL_UINT8(50, filter.shown_percent);
    feed(3804, 200);                            // settles at 51
    TEST_ASSERT_EQUAL_UINT8(50, filter.shown_percent);
}

static void test_filter_two_point_change_updates_display(void)
{
    puck_battery_filter_update(&filter, 3800);  // 50
    feed(3790, 200);                            // settles at 48
    TEST_ASSERT_EQUAL_UINT8(48, filter.shown_percent);
    feed(3810, 200);                            // 52 is 4 away from 48
    TEST_ASSERT_EQUAL_UINT8(52, filter.shown_percent);
}

static void test_filter_sustained_drop_updates_display(void)
{
    puck_battery_filter_update(&filter, 4000);
    feed(3700, 100);
    // Hysteresis may leave it one point off the exact value.
    TEST_ASSERT_UINT8_WITHIN(1, 30, filter.shown_percent);
}

static void test_filter_reaches_zero_and_hundred(void)
{
    puck_battery_filter_update(&filter, 3400);  // 2
    feed(3200, 300);
    TEST_ASSERT_EQUAL_UINT8(0, filter.shown_percent);

    puck_battery_filter_init(&filter);
    puck_battery_filter_update(&filter, 4100);  // 90
    feed(4300, 300);
    TEST_ASSERT_EQUAL_UINT8(100, filter.shown_percent);
}

static void test_filter_shows_extremes_despite_one_point_hysteresis(void)
{
    puck_battery_filter_update(&filter, 3350);  // 1
    feed(3300, 200);
    TEST_ASSERT_EQUAL_UINT8(0, filter.shown_percent);

    puck_battery_filter_init(&filter);
    puck_battery_filter_update(&filter, 4190);  // 99
    feed(4200, 200);
    TEST_ASSERT_EQUAL_UINT8(100, filter.shown_percent);
}

static void test_filter_converges_up_to_new_steady_voltage(void)
{
    puck_battery_filter_update(&filter, 3700);
    feed(4000, 200);
    TEST_ASSERT_EQUAL_UINT8(puck_battery_percent(4000), filter.shown_percent);
}

static void test_filter_converges_down_to_new_steady_voltage(void)
{
    puck_battery_filter_update(&filter, 4000);
    feed(3700, 200);
    TEST_ASSERT_UINT8_WITHIN(1, puck_battery_percent(3700), filter.shown_percent);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_divider_multiplies_adc_reading);
    RUN_TEST(test_divider_saturates_at_uint16_max);
    RUN_TEST(test_every_curve_point_maps_exactly);
    RUN_TEST(test_percent_clamps_above_full_and_below_empty);
    RUN_TEST(test_percent_interpolates_between_points);
    RUN_TEST(test_percent_rounds_to_nearest_with_halves_up);
    RUN_TEST(test_percent_never_decreases_as_voltage_rises);
    RUN_TEST(test_low_threshold_boundaries);
    RUN_TEST(test_filter_init_clears_state);
    RUN_TEST(test_filter_first_reading_is_taken_as_is);
    RUN_TEST(test_filter_averages_with_weight_one_eighth);
    RUN_TEST(test_filter_single_spike_barely_moves_display);
    RUN_TEST(test_filter_one_point_drift_does_not_change_display);
    RUN_TEST(test_filter_two_point_change_updates_display);
    RUN_TEST(test_filter_sustained_drop_updates_display);
    RUN_TEST(test_filter_reaches_zero_and_hundred);
    RUN_TEST(test_filter_shows_extremes_despite_one_point_hysteresis);
    RUN_TEST(test_filter_converges_up_to_new_steady_voltage);
    RUN_TEST(test_filter_converges_down_to_new_steady_voltage);
    return UNITY_END();
}
