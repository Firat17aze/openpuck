# First install log (2026-11-08)

How OpenPuck was first installed on a Life Puck running its factory firmware,
over USB-C, without opening the case. This is the reference for the end-user
installer (see [`roadmap.md`](roadmap.md)). Day-to-day instructions are in
[`flashing.md`](flashing.md).

## Setup

- Mac mini (Apple Silicon, macOS), USB-C data cable straight to the puck.
- esptool 5.6 via Python. ESP-IDF v6.1 was used only to build the firmware.
- A full 4 MB backup of the factory flash, made beforehand and checked by
  SHA-256.

## What makes this device different

1. **Power is held by firmware.** GPIO7 must stay high or the puck switches
   off, and every chip reset releases it. esptool's normal "reset into download
   mode" therefore turns a factory-firmware puck off mid-connection.
2. **The power button keeps the board powered in hardware while held.** This is
   what makes the first install possible.
3. **On macOS, opening the serial port toggles DTR and resets the chip.** Every
   step after connecting has to happen in **one session**. Reconnecting means
   another reset.

## Procedure that worked

| # | Step | Detail |
|---|---|---|
| 1 | Start with the puck **off** | The script waits for the USB port to disappear first, so it never resets a puck that's already running before the user is ready |
| 2 | User **presses and holds** the power button | Factory firmware boots and the USB port appears |
| 3 | Wait 2 s, then esptool's default reset into ROM download mode | The button keeps the board powered through the reset |
| 4 | Take over the power latch from the ROM loader | IO_MUX `0x60009020` MCU_SEL = 1; `GPIO_FUNC7_OUT_SEL` (`0x60004570`) = `0x80`; set bit 7 in `GPIO_OUT_W1TS` and `GPIO_ENABLE_W1TS` |
| 5 | Prompt **"Release the button now"** (spoken on macOS) | Poll GPIO10 (button) via `GPIO_IN` until it reads released |
| 6 | Upload the flasher stub, then **attach flash** | Without attaching, the flash ID reads `0xffffff` |
| 7 | Safety gates, nothing written unless all pass | Backup present with matching SHA-256; flash ID `0x164020` (XMC, 4 MB); first 4 KB of flash equal the backup |
| 8 | Erase `0x9000`–`0xFFFF` | The new partition table places NVS/PHY data there |
| 9 | Write bootloader `0x0`, partition table `0x8000`, app `0x10000` | 5 s for 612 KB |
| 10 | Verify all three (MD5) | All matched |
| 11 | Hard reset | Releases the latch, so the puck powers off; the user presses the button to start OpenPuck |

Total time with the user involved: about 25 s from button press to done.

## Mistakes to design out of the installer

- **Resetting before the user is ready.** An early script acted on a port that
  was already present and reset a running puck before the user had pressed
  anything, switching it off. The installer must wait for a *fresh* power-on
  (port absent, then present) before the guided hold.
- **Text prompts the user can't see.** Background scripts can't show "release
  now" to someone looking at the device. Use spoken or on-screen cues tied to
  each step.
- **Reading the flash ID without attaching flash** gives `0xffffff`, which
  looks like a dead chip.

## After the first install

OpenPuck holds the latch with `gpio_hold_en`, which survives resets,
including the reset into download mode. Later updates, backups and restores
connect directly, with no button hold. Verified: `flash.py info`, `backup` (4 MB
in 21 s) and `flash` (7 s), all with nobody touching the device.
