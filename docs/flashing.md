# Backing up, flashing and restoring

`tools/flash.py` does everything over the puck's USB-C port. No need to open
the case.

## Requirements

- An ESP-IDF environment (it provides `esptool` and `pyserial`). Activate it
  first, for example `source ~/.espressif/tools/activate_idf_v6.1.sh`.
- A USB-C **data** cable. Many charging cables have no data wires.
- For flashing: a build (`idf.py build`).

## 1. Back up your puck (do this first)

```sh
python tools/flash.py backup my-puck-backup.bin
```

This saves the whole 4 MB flash (about 20 seconds) plus a `.sha256` file.
Keep it safe: it's the only way back to the firmware your puck shipped with.
It contains that firmware, so keep it to yourself and don't share it.

## 2. Flash OpenPuck

```sh
idf.py build
python tools/flash.py flash --backup my-puck-backup.bin
```

`--backup` is required. The tool checks that the file looks like a real full
backup before it writes anything.

## 3. Restore the original firmware

```sh
python tools/flash.py restore my-puck-backup.bin
```

## Holding the power button (factory firmware only)

The puck stays powered only while its firmware holds the power latch on, and
connecting resets the chip:

- **OpenPuck** keeps the latch held through the reset, so the commands above
  just work while it's running.
- **Factory firmware** lets go, and the puck switches off. The tool notices and
  says **"Press and hold the power button"**. Turn the puck on and keep
  holding; the tool connects, takes over holding the power, and says
  **"Release the button now"**. On macOS the prompts are also spoken aloud.

So you'll only need the button for your first backup and first flash, or after
a restore.

## Checking the connection

```sh
python tools/flash.py info
```

Read-only: shows the chip, its MAC address and the flash size, then resets the
puck.

## Troubleshooting

- **"The puck never appeared on USB"**: try another cable (data, not
  charge-only) or another port.
- **The puck stays off after a command**: press the power button.
- **Opening a serial monitor**: with OpenPuck running, opening the USB serial
  port restarts it (the log starts from boot); it stays on. With factory
  firmware it switches the puck off.
