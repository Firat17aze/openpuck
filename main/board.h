// Pin map and hardware constants for the MetalFab Tokens Life Puck.
// See docs/hardware.md for how each was identified.
#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

// Power
#define BOARD_PIN_POWER_LATCH       GPIO_NUM_7   // high = stay on, low = power off
#define BOARD_PIN_POWER_BUTTON      GPIO_NUM_10  // reads BOARD_BUTTON_PRESSED_LEVEL when pressed
#define BOARD_BUTTON_PRESSED_LEVEL  1            // external pull resistor; internal pulls off

// Display: GC9A01, 240x240, SPI
#define BOARD_LCD_SPI_HOST          SPI2_HOST
#define BOARD_PIN_LCD_MOSI          GPIO_NUM_3
#define BOARD_PIN_LCD_SCLK          GPIO_NUM_5
#define BOARD_PIN_LCD_DC            GPIO_NUM_6
#define BOARD_PIN_LCD_CS            GPIO_NUM_NC  // not connected
#define BOARD_PIN_LCD_RST           GPIO_NUM_NC  // not connected
#define BOARD_LCD_PCLK_HZ           (80 * 1000 * 1000)
#define BOARD_LCD_H_RES             240
#define BOARD_LCD_V_RES             240
#define BOARD_LCD_INVERT_COLOR      true
#define BOARD_LCD_MIRROR_X          true
#define BOARD_LCD_MIRROR_Y          false

// Charger status: the charger pulls this low while charging (open drain;
// use the internal pull-up). High when unplugged or fully charged.
#define BOARD_PIN_CHARGE_STATUS     GPIO_NUM_20
#define BOARD_CHARGING_LEVEL        0

// Battery voltage through a 1:4 divider (ADC1 channel 4)
#define BOARD_PIN_BATTERY_ADC       GPIO_NUM_4

// Backlight: PWM, active high
#define BOARD_PIN_BACKLIGHT         GPIO_NUM_2

// Touch: CST816S, I2C
#define BOARD_PIN_TOUCH_SDA         GPIO_NUM_1
#define BOARD_PIN_TOUCH_SCL         GPIO_NUM_0
// The touch INT line (active low) is on GPIO9, which is also the BOOT
// strapping pin: use it as an input only, never drive it or add a pull-down.
#define BOARD_PIN_TOUCH_INT         GPIO_NUM_9
#define BOARD_PIN_TOUCH_RST         GPIO_NUM_NC  // not connected
