#include "battery_sense.h"

#include "board.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "puck_battery.h"

static const char *TAG = "battery";

#define ADC_UNIT_USED      ADC_UNIT_1
#define ADC_CHANNEL_USED   ADC_CHANNEL_4   // GPIO4
#define SAMPLES            16
#define PERIOD_MS          5000
#define LOG_EVERY          12              // once per minute
#define TASK_PRIORITY      1
#define TASK_STACK         3072

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;
static volatile int s_percent = -1;
static volatile uint16_t s_mv;

static adc_cali_handle_t create_calibration(void)
{
    adc_cali_handle_t handle = NULL;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    const adc_cali_curve_fitting_config_t cfg = {
        .unit_id = ADC_UNIT_USED,
        .chan = ADC_CHANNEL_USED,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cfg, &handle) != ESP_OK) {
        handle = NULL;
    }
#endif
    return handle;
}

// Average of several raw samples, as calibrated ADC pin mV; -1 on failure.
static int read_adc_mv(void)
{
    int sum = 0;
    for (int i = 0; i < SAMPLES; i++) {
        int raw;
        if (adc_oneshot_read(s_adc, ADC_CHANNEL_USED, &raw) != ESP_OK) {
            return -1;
        }
        sum += raw;
    }
    const int raw = sum / SAMPLES;
    if (s_cali) {
        int mv;
        return adc_cali_raw_to_voltage(s_cali, raw, &mv) == ESP_OK ? mv : -1;
    }
    return raw * 3300 / 4095;
}

static void battery_task(void *arg)
{
    puck_battery_filter_t filter;
    puck_battery_filter_init(&filter);
    int reads = 0;

    for (;;) {
        const int adc_mv = read_adc_mv();
        if (adc_mv >= 0) {
            const uint16_t battery_mv = puck_battery_mv_from_adc_mv((uint16_t)adc_mv);
            const uint8_t percent = puck_battery_filter_update(&filter, battery_mv);
            s_mv = (uint16_t)(filter.avg_mv_x8 / 8);
            s_percent = percent;
            if (reads++ % LOG_EVERY == 0) {
                ESP_LOGI(TAG, "battery %u mV, %u%%", (unsigned)s_mv, (unsigned)percent);
            }
        } else {
            ESP_LOGW(TAG, "ADC read failed");
        }
        vTaskDelay(pdMS_TO_TICKS(PERIOD_MS));
    }
}

int battery_sense_percent(void)
{
    return s_percent;
}

uint16_t battery_sense_mv(void)
{
    return s_mv;
}

bool battery_sense_charging(void)
{
    return gpio_get_level(BOARD_PIN_CHARGE_STATUS) == BOARD_CHARGING_LEVEL;
}

void battery_sense_start(void)
{
    const gpio_config_t charge_cfg = {
        .pin_bit_mask = BIT64(BOARD_PIN_CHARGE_STATUS),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&charge_cfg));
    const adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_USED };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));
    const adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, ADC_CHANNEL_USED, &chan_cfg));

    s_cali = create_calibration();
    if (!s_cali) {
        ESP_LOGW(TAG, "ADC calibration unavailable; using an uncalibrated estimate");
    }
    xTaskCreate(battery_task, "battery", TASK_STACK, NULL, TASK_PRIORITY, NULL);
}
