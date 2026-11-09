#!/bin/sh
# Render the documentation screenshots (docs/images/*.png) on the host: the real
# UI code with the hardware stubbed out. Needs cmake and a C compiler; the
# network is used only to fetch LVGL and the PNG writer the first time.
#   tools/screenshots.sh                  all scenes
#   tools/screenshots.sh menu history     only these scenes
#   tools/screenshots.sh --no-bezel       plain 240x240, round-masked (for tests)
set -e
cd "$(dirname "$0")/.."
BUILD=build-screenshots

cmake -S tools/screenshots -B "$BUILD" > /dev/null
cmake --build "$BUILD" --parallel > /dev/null
mkdir -p docs/images
"$BUILD/screenshots" "$@"
