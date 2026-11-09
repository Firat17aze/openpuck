# Hardware: MetalFab Tokens Life Puck

Facts about the hardware, measured or observed on a retail unit.

## Chip

- ESP32-C3FH4, revision v0.4. 4 MB XMC flash in-package, DIO mode, 80 MHz.
- Security eFuses unset: no flash encryption, no secure boot, JTAG and
  download mode enabled.
- USB-C goes straight to the chip's built-in USB-Serial-JTAG (`303a:1001`).
  There is no separate USB-to-UART adapter chip.

## Pin map

| GPIO | Function | Notes |
|---|---|---|
| 0 | Touch I2C SCL | |
| 1 | Touch I2C SDA | |
| 2 | Backlight | PWM (LEDC), active high. 5 kHz, 8-bit works |
| 3 | LCD SPI MOSI | |
| 5 | LCD SPI SCLK | 80 MHz works |
| 6 | LCD D/C | |
| 7 | **Power latch** | Drive high early in boot to stay on, low to power off |
| 9 | Touch interrupt (CST816S INT) | **Also the BOOT strapping pin** |
| 10 | **Power button** | Reads 1 when pressed. Relies on an external pull resistor |
| 20 | **Charger status** | Pulled low by the charger while charging (open drain: enable the internal pull-up). High when unplugged or full. Verified 2026-11-08 |

Not connected: LCD CS, LCD RESET and touch RESET.

## Display: GC9A01

240×240 round panel, RGB565, BGR element order. Needs color inversion and
X mirroring to display correctly. With LVGL 9 via `esp_lvgl_port`, set
`swap_bytes`. Verified on a device: colors correct, and touch coordinates
line up with the display with no touch mirroring.

## Touch: CST816S

- I2C address `0x15`. 100 kHz works.
- Register `0xA7` holds the chip ID and `0xA9` the firmware version.
- **Auto-sleep:** by default the chip drops to standby after a few seconds idle,
  and the first touch may only wake it. Writing 1 to register `0xFE`
  (DisAutoSleep) turns this off. Verified on a device: taps register
  immediately after 10+ s idle.
- The INT pin is shared with the BOOT strapping pin (GPIO9). If INT is held low
  during a reset (for example a finger on the screen), the chip may start in
  download mode. To be verified.

## Power behavior

- Power is held on by the firmware driving GPIO7 high.
- **Any chip reset turns the puck off**, because GPIO7 is released. This
  includes crashes, watchdog resets, esptool/OpenOCD resets and even opening
  the USB serial port on macOS (DTR toggles on open). USB power does not keep
  it on.
- **`gpio_hold_en(GPIO7)` keeps the latch held through a chip reset.**
  Verified: a USB-triggered reset (`rst:0x15 USB_UART_CHIP_RESET`) restarted
  the firmware without losing power, and USB stayed connected. Firmware that
  holds the latch this way restarts on a crash instead of switching off, and
  its serial log can be read over USB (each port open restarts it).
- **Holding the power button keeps the board powered in hardware.** Verified
  2026-11-08. This allows flashing over USB without opening the case:
  1. With the puck off, press and hold the power button.
  2. Within a few seconds, reset into ROM download mode over USB-Serial-JTAG
     (esptool's default reset).
  3. From the ROM loader, set GPIO7 as a GPIO output and drive it high:
     - IO_MUX `0x60009020`: set MCU_SEL = 1
     - `GPIO_FUNC7_OUT_SEL` `0x60004570` = `0x80`
     - `GPIO_OUT_W1TS` / `GPIO_ENABLE_W1TS` bit 7
  4. Release the button. The chip stays powered in download mode, and closing
     the port does not drop power. **Reopening the port does**: on macOS the
     open toggles DTR, the chip resets, and the latch is released (verified
     2026-11-08). Do all flash work in the same esptool session.
  - Note: the firmware isn't running in download mode, so nothing will power the
    puck off. Reset it when you're done, or the battery will drain.

## Battery

Battery voltage is on GPIO4 (ADC1 channel 4) through a 1:4 divider (battery
mV = ADC mV x 4). Charging status is on GPIO20 (see the pin map).

## Debug access

- JTAG over USB works (`openocd-esp32`, `board/esp32c3-builtin.cfg`). Attaching
  does not reset the chip.
- Briefly halting the CPU and reading memory is safe while the device runs.
- Running OpenOCD's flash stub while the factory firmware runs hung the CPU.
