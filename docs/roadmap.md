# Roadmap: distribution, testing and releases

## 1. Easy install for non-technical owners

**Goal:** someone with a Life Puck, a USB-C cable and a computer can back up
their puck and install OpenPuck in a few minutes, **without installing
anything**.

### Primary: browser installer on GitHub Pages

A web page using Espressif's [esptool-js](https://github.com/espressif/esptool-js)
over the Web Serial API:

- Works in **Chrome, Edge and Opera on Windows, macOS, Linux and ChromeOS**.
  Not Safari, Firefox or phones; the page must detect this and say so plainly.
- No drivers needed: the puck's USB-Serial-JTAG is a standard CDC device
  (built into Windows 10+, macOS and Linux).
- Guided flow, one step per screen, with pictures:
  1. **Connect**: plug in, click Connect, pick the puck in the browser's list.
  2. **Hold the button**, only if the puck switched off on connect (factory
     firmware): "Press and hold the power button… keep holding…", then
     "Release now". Same logic as `tools/flash.py` (see
     [`first-install-log.md`](first-install-log.md)). Show the cue on screen
     and play a sound.
  3. **Back up**: read all 4 MB and download `life-puck-backup-<mac>.bin`
     automatically, before anything is written. Required; can't be skipped.
  4. **Install**: write and verify OpenPuck, with a progress bar and clear
     "don't unplug" messaging.
  5. **Done**: "Press the power button to start OpenPuck."
  - **Restore** page: pick your backup file, and it writes it back.
- Release builds publish the bootloader, partition table and app (or one merged
  image) plus a manifest, so the page always offers the latest release.

**To prove early (spike):**

- esptool-js can drive the latch registers (`writeReg`) and read GPIO10 in the
  same session.
- Browser behavior when the puck re-enumerates after the power-on: is the
  already-granted port reusable via `navigator.serial.getPorts()` and the
  `connect` event, or must the user pick it again?
- DTR/RTS handling on port open in each OS, which is what caused the macOS
  resets.

### Fallback: `tools/flash.py`

For unsupported browsers and developers. Needs Python + esptool. Optionally
later: packaged single-file executables (e.g. PyInstaller) for macOS and
Windows, built in CI.

## 2. Testing and code coverage

**Architecture rule:** everything that isn't hardware lives in a **pure C
component with no ESP-IDF dependencies** (`components/puck_core`): life
totals, players, history, timer, settings, the save scheduler (decides *when*
to save) and the state serialization format. Hardware code (`main/`) stays a
thin layer. That makes most of the firmware testable on a normal computer.

| Layer | What | How | Runs |
|---|---|---|---|
| Unit | `puck_core` logic | Unity + CMake on the host (gcc/clang), sanitizers on | Every push |
| Coverage | `puck_core` | gcov + gcovr, report published, threshold enforced (target ≥ 90 % lines) | Every push |
| Serialization | Saved-state format | Round-trip and compatibility tests with stored fixtures from every released version | Every push |
| UI | Screens render without errors | LVGL on the host (headless), screenshot comparison | Every push (later) |
| Firmware build | Full ESP-IDF build for esp32c3, size report, warnings as errors | `espressif/idf:v6.1` container | Every push |
| Installer | Web installer logic | JS unit tests with a mocked esptool-js/serial port | Every push (once it exists) |
| Hardware-in-the-loop | Flash a real puck, boot, check the log | Self-hosted runner with a puck attached | Optional, before releases |

## 3. GitHub Actions

- **`ci.yml`** (push / PR): host unit tests + coverage gate, ESP-IDF build,
  formatting and static checks, artifacts for the firmware binaries.
- **`release.yml`** (on a version tag `vX.Y.Z`): build, run all tests, attach
  firmware binaries + manifest + checksums to a GitHub Release, and deploy the
  web installer (with that release) to GitHub Pages.
- Versioning: semantic versions; `PROJECT_VER` comes from the git tag.

## Before the first public release

- Choose a license.
- Web installer tested by someone non-technical on Windows and macOS.
- Restore path tested end to end on a real device.
