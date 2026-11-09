#include "counter.h"
#include "display.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "battery_sense.h"
#include "power.h"
#include "power_stats.h"
#include "storage.h"
#include "ui.h"

static const char *TAG = "openpuck";

static void on_power_off(const char *reason)
{
    if (lvgl_port_lock(200)) {
        counter_flush(lv_tick_get());
        lvgl_port_unlock();
    }
    ui_show_message(reason);
}

// Runs on the LVGL task: go back one UI layer, or toggle the timer (when it is
// enabled) if none is open.
static void power_press_async(void *arg)
{
    if (!ui_back()) {
        if (counter_settings()->timer_enabled) {
            counter_timer_toggle(lv_tick_get());
            ui_refresh_counter();
        }
    }
}

// Runs on the power task: hand the work to the LVGL task.
static void on_power_short_press(void)
{
    if (lvgl_port_lock(100)) {
        lv_async_call(power_press_async, NULL);
        lvgl_port_unlock();
    }
}

static void on_settings_changed(const puck_settings_t *settings)
{
    power_configure_auto_off(settings->auto_off_enabled, settings->auto_off_minutes);
    display_set_brightness(settings->brightness);
}

void app_main(void)
{
    // Must come first: the puck only stays on while the latch is driven high.
    power_hold_on();
    power_sleep_enable();
    ESP_LOGI(TAG, "OpenPuck %s", esp_app_get_description()->version);

    storage_init();
    counter_init();
    const puck_settings_t *settings = counter_settings();
    power_configure_auto_off(settings->auto_off_enabled, settings->auto_off_minutes);

    lv_indev_t *touch = display_init();
    ui_start(touch);
    power_stats_start();
    display_set_brightness(settings->brightness);
    battery_sense_start();

    counter_set_settings_callback(on_settings_changed);
    power_set_auto_off_inhibitor(counter_timer_running);
    power_set_off_callback(on_power_off);
    power_set_short_press_callback(on_power_short_press);
    power_start();
}
