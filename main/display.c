#include "display.h"

#include "board.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_cst816s.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "power.h"

static const char *TAG = "display";

#define BACKLIGHT_LEDC_TIMER    LEDC_TIMER_0
#define BACKLIGHT_LEDC_CHANNEL  LEDC_CHANNEL_0
#define LVGL_BUFFER_LINES       40

// CST816S register 0xFE (DisAutoSleep): non-zero keeps the controller awake,
// so the first tap after an idle pause isn't swallowed waking it up.
#define CST816S_REG_DIS_AUTO_SLEEP 0xFE

static void backlight_init(void)
{
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = BACKLIGHT_LEDC_TIMER,
        .freq_hz = 5000,
        // RC_FAST keeps running in light sleep (the default clocks stop).
        .clk_cfg = LEDC_USE_RC_FAST_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
    const ledc_channel_config_t channel = {
        .gpio_num = BOARD_PIN_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BACKLIGHT_LEDC_CHANNEL,
        .timer_sel = BACKLIGHT_LEDC_TIMER,
        .duty = 0,
        // Keep the PWM output running while the chip light-sleeps.
        .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

void display_set_brightness(int percent)
{
    percent = percent < 0 ? 0 : percent > 100 ? 100 : percent;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_LEDC_CHANNEL, percent * 255 / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_LEDC_CHANNEL);
}

static void panel_init(esp_lcd_panel_io_handle_t *io, esp_lcd_panel_handle_t *panel)
{
    const spi_bus_config_t bus = GC9A01_PANEL_BUS_SPI_CONFIG(
        BOARD_PIN_LCD_SCLK, BOARD_PIN_LCD_MOSI,
        BOARD_LCD_H_RES * LVGL_BUFFER_LINES * sizeof(uint16_t));
    ESP_ERROR_CHECK(spi_bus_initialize(BOARD_LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = GC9A01_PANEL_IO_SPI_CONFIG(
        BOARD_PIN_LCD_CS, BOARD_PIN_LCD_DC, NULL, NULL);
    io_cfg.pclk_hz = BOARD_LCD_PCLK_HZ;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BOARD_LCD_SPI_HOST,
                                             &io_cfg, io));

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BOARD_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(*io, &panel_cfg, panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*panel, BOARD_LCD_INVERT_COLOR));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(*panel, BOARD_LCD_MIRROR_X, BOARD_LCD_MIRROR_Y));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*panel, true));
}

// Runs on every touch read; counts touches as activity for the auto-off.
static void touch_seen(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                       uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    if (*point_num > 0) {
        power_note_activity();
    }
}

static esp_lcd_touch_handle_t touch_init(void)
{
    const i2c_master_bus_config_t bus_cfg = {
        .i2c_port = -1,
        .sda_io_num = BOARD_PIN_TOUCH_SDA,
        .scl_io_num = BOARD_PIN_TOUCH_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    esp_lcd_panel_io_handle_t io;
    const esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &io_cfg, &io));

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BOARD_LCD_H_RES,
        .y_max = BOARD_LCD_V_RES,
        .rst_gpio_num = BOARD_PIN_TOUCH_RST,
        .int_gpio_num = BOARD_PIN_TOUCH_INT,  // esp_lvgl_port then reads on interrupts only
        .levels.interrupt = 0,                // active low
        .process_coordinates = touch_seen,
    };
    esp_lcd_touch_handle_t tp;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(io, &tp_cfg, &tp));

    const uint8_t keep_awake = 1;
    esp_err_t err = esp_lcd_panel_io_tx_param(io, CST816S_REG_DIS_AUTO_SLEEP, &keep_awake, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "could not disable touch auto-sleep: %s", esp_err_to_name(err));
    }
    return tp;
}

// LVGL reads time from the microsecond timer, so the port's periodic tick
// timer (which would wake the chip every few ms) is not needed.
static uint32_t tick_get_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// Light-sleep wakeup on the touch INT line needs the pin in LOW LEVEL mode
// (gpio_wakeup_enable switches it), and a level interrupt would retrigger
// continuously while INT is low. So the interrupt handler masks itself, and
// it is re-enabled after the touch controller has been read.
static esp_lcd_touch_interrupt_callback_t s_touch_isr_cb;
static lv_indev_read_cb_t s_touch_read_cb;

static void IRAM_ATTR touch_int_isr(void *arg)
{
    gpio_intr_disable(BOARD_PIN_TOUCH_INT);
    s_touch_isr_cb((esp_lcd_touch_handle_t)arg);
}

static void touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    s_touch_read_cb(indev, data);
    gpio_intr_enable(BOARD_PIN_TOUCH_INT);
}

static void touch_wakeup_init(esp_lcd_touch_handle_t tp, lv_indev_t *indev)
{
    // Swap the driver's edge handler for the self-masking one above.
    s_touch_isr_cb = tp->config.interrupt_callback;
    ESP_ERROR_CHECK(gpio_isr_handler_remove(BOARD_PIN_TOUCH_INT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BOARD_PIN_TOUCH_INT, touch_int_isr, tp));
    s_touch_read_cb = lv_indev_get_read_cb(indev);
    lv_indev_set_read_cb(indev, touch_read);
    // GPIO9 is the strapping pin: input only, no pulls (the controller drives it).
    ESP_ERROR_CHECK(gpio_wakeup_enable(BOARD_PIN_TOUCH_INT, GPIO_INTR_LOW_LEVEL));
    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
}

lv_indev_t *display_init(void)
{
    backlight_init();

    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_handle_t panel;
    panel_init(&io, &panel);
    esp_lcd_touch_handle_t tp = touch_init();

    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.timer_period_ms = 1000;      // the port's tick is unused (see tick_get_ms)
    port_cfg.task_max_sleep_ms = 2000;
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));
    lv_tick_set_cb(tick_get_ms);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io,
        .panel_handle = panel,
        .buffer_size = BOARD_LCD_H_RES * LVGL_BUFFER_LINES,
        .double_buffer = true,
        .hres = BOARD_LCD_H_RES,
        .vres = BOARD_LCD_V_RES,
        .rotation = {
            .mirror_x = BOARD_LCD_MIRROR_X,
            .mirror_y = BOARD_LCD_MIRROR_Y,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        },
    };
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };
    lv_indev_t *indev = lvgl_port_add_touch(&touch_cfg);
    lvgl_port_lock(0);
    touch_wakeup_init(tp, indev);
    lvgl_port_unlock();
    return indev;
}
