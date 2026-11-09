#include "power.h"

#include "board.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "power";

#define POLL_MS            10    // button polling while it is pressed
#define IDLE_WAIT_MS       1000  // button released: wait for a press, or this long
#define PM_MAX_MHZ         160
#define PM_MIN_MHZ         40
#define DEBOUNCE_SAMPLES   3     // state must be stable for 30 ms to count
#define TASK_PRIORITY      10    // above the LVGL task
#define TASK_STACK         4096  // the off callback saves to NVS on this task

// 32-bit so reads and writes from different tasks are single instructions;
// differences are wraparound-safe.
static volatile uint32_t s_last_activity_ms;
static volatile bool s_auto_off_enabled;
static volatile uint32_t s_auto_off_minutes;
static bool (*volatile s_inhibit)(void);
static power_off_cb_t s_off_cb;
static power_press_cb_t s_short_press_cb;
static TaskHandle_t s_power_task;

void power_hold_on(void)
{
    gpio_set_level(BOARD_PIN_POWER_LATCH, 1);
    const gpio_config_t cfg = {
        .pin_bit_mask = BIT64(BOARD_PIN_POWER_LATCH),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    gpio_set_level(BOARD_PIN_POWER_LATCH, 1);
    // Light sleep must not isolate the latch pad (that would switch the puck
    // off). Do this before the hold, which freezes the pad.
    ESP_ERROR_CHECK(gpio_sleep_sel_dis(BOARD_PIN_POWER_LATCH));
    // Keep the pad driven through chip resets, so a crash restarts the puck
    // instead of switching it off (verified with a USB-triggered chip reset).
    gpio_hold_en(BOARD_PIN_POWER_LATCH);
}

// By default light sleep switches every pad to an isolated (disabled) state.
// Every pin we use must keep its normal configuration instead: the LCD has no
// chip select, so a floating SCLK/MOSI would clock garbage into it.
static void keep_pins_in_sleep(void)
{
    static const gpio_num_t pins[] = {
        BOARD_PIN_POWER_LATCH, BOARD_PIN_POWER_BUTTON,
        BOARD_PIN_LCD_MOSI, BOARD_PIN_LCD_SCLK, BOARD_PIN_LCD_DC,
        BOARD_PIN_CHARGE_STATUS, BOARD_PIN_BACKLIGHT,
        BOARD_PIN_TOUCH_SDA, BOARD_PIN_TOUCH_SCL, BOARD_PIN_TOUCH_INT,
        BOARD_PIN_BATTERY_ADC,
    };
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        ESP_ERROR_CHECK(gpio_sleep_sel_dis(pins[i]));
    }
}

void power_sleep_enable(void)
{
    keep_pins_in_sleep();
    const esp_pm_config_t cfg = {
        .max_freq_mhz = PM_MAX_MHZ,
        .min_freq_mhz = PM_MIN_MHZ,
        .light_sleep_enable = true,
    };
    ESP_ERROR_CHECK(esp_pm_configure(&cfg));
}

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void power_note_activity(void)
{
    s_last_activity_ms = now_ms();
}

void power_configure_auto_off(bool enabled, uint32_t minutes)
{
    s_auto_off_minutes = minutes;
    s_auto_off_enabled = enabled;
    power_note_activity();
}

void power_set_auto_off_inhibitor(bool (*inhibit)(void))
{
    s_inhibit = inhibit;
}

void power_set_off_callback(power_off_cb_t cb)
{
    s_off_cb = cb;
}

void power_set_short_press_callback(power_press_cb_t cb)
{
    s_short_press_cb = cb;
}

static void power_off(const char *reason)
{
    ESP_LOGW(TAG, "powering off: %s", reason);
    if (s_off_cb) {
        s_off_cb(reason);
    }
    gpio_hold_dis(BOARD_PIN_POWER_LATCH);
    gpio_set_level(BOARD_PIN_POWER_LATCH, 0);
    // While the button is still held, the hardware keeps the board powered;
    // power drops when it is released.
    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}

static bool button_pressed_raw(void)
{
    return gpio_get_level(BOARD_PIN_POWER_BUTTON) == BOARD_BUTTON_PRESSED_LEVEL;
}

// Button pressed (level high): mask the interrupt and wake the power task,
// which polls while the button is down and re-enables the interrupt after.
static void IRAM_ATTR button_isr(void *arg)
{
    gpio_intr_disable(BOARD_PIN_POWER_BUTTON);
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_power_task, &woken);
    portYIELD_FROM_ISR(woken);
}

static void power_task(void *arg)
{
    s_power_task = xTaskGetCurrentTaskHandle();
    bool stable = button_pressed_raw();
    bool armed = !stable;  // ignore the press that turned the puck on, if still held
    int same_count = 0;
    int64_t pressed_at_us = 0;
    bool idle = !stable;  // released and settled: nothing to poll

    for (;;) {
        if (idle) {
            // Sleep until a press (or the periodic auto-off check).
            ESP_ERROR_CHECK(gpio_intr_enable(BOARD_PIN_POWER_BUTTON));
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(IDLE_WAIT_MS));
        } else {
            vTaskDelay(pdMS_TO_TICKS(POLL_MS));
        }
        const int64_t now = esp_timer_get_time();
        const bool raw = button_pressed_raw();

        same_count = (raw == stable) ? 0 : same_count + 1;
        if (same_count >= DEBOUNCE_SAMPLES) {
            stable = raw;
            same_count = 0;
            if (!stable) {
                if (armed && pressed_at_us) {
                    ESP_LOGI(TAG, "short press (%lld ms)", (now - pressed_at_us) / 1000);
                    if (s_short_press_cb) {
                        s_short_press_cb();
                    }
                }
                armed = true;
                pressed_at_us = 0;
            } else if (armed) {
                pressed_at_us = now;
            }
            power_note_activity();
        }

        if (armed && stable && pressed_at_us &&
                now - pressed_at_us >= (int64_t)CONFIG_OPENPUCK_POWER_OFF_HOLD_MS * 1000) {
            power_off("Release to power off");
        }
        idle = !stable && !raw && same_count == 0;
        bool (*inhibit)(void) = s_inhibit;
        if (inhibit && inhibit()) {
            power_note_activity();
        }
        const uint32_t auto_off_ms = s_auto_off_minutes * 60000u;
        if (s_auto_off_enabled && auto_off_ms && now_ms() - s_last_activity_ms >= auto_off_ms) {
            power_off("Idle - powering off");
        }
    }
}

void power_start(void)
{
    const gpio_config_t cfg = {
        .pin_bit_mask = BIT64(BOARD_PIN_POWER_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        // Level (not edge) so the same setting wakes light sleep; see button_isr.
        .intr_type = GPIO_INTR_HIGH_LEVEL,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    gpio_intr_disable(BOARD_PIN_POWER_BUTTON);  // the task enables it when idle
    // The touch driver normally installs the GPIO ISR service first; only
    // install it here if it isn't there yet (installing twice logs an error).
    esp_err_t err = gpio_isr_handler_add(BOARD_PIN_POWER_BUTTON, button_isr, NULL);
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(gpio_install_isr_service(0));
        err = gpio_isr_handler_add(BOARD_PIN_POWER_BUTTON, button_isr, NULL);
    }
    ESP_ERROR_CHECK(err);
    gpio_intr_disable(BOARD_PIN_POWER_BUTTON);  // handler_add enables it
    // Wake light sleep when the button is pressed (high).
    ESP_ERROR_CHECK(gpio_wakeup_enable(BOARD_PIN_POWER_BUTTON, GPIO_INTR_HIGH_LEVEL));
    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
    power_note_activity();
    xTaskCreate(power_task, "power", TASK_STACK, NULL, TASK_PRIORITY, NULL);
}
