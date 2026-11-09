// Host rendering harness: a 240x240 LVGL display drawn into memory, a simulated
// clock and a scripted touch device. Scenes (scenes.c) use it to drive the real
// UI code and save PNGs.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

#define SCREEN_SIZE 240

// Start from a clean slate: LVGL initialized, the clock at a fixed time, the
// fake battery and storage reset. Call once per process (scenes run in their
// own process).
void render_begin(void);

lv_indev_t *render_indev(void);

// Simulated time, in ms (what lv_tick_get returns).
uint32_t render_now(void);
// Move the clock forward in 10 ms steps, running LVGL (timers, input,
// animations, drawing) at each step.
void render_advance(uint32_t ms);
// Move the clock forward in one step (a long wait, e.g. a running timer).
void render_jump(uint32_t ms);

// Touch input. Positions are screen pixels; the touch is read on the next
// LVGL step, so follow each call with render_advance.
void touch_down(int32_t x, int32_t y);
void touch_move(int32_t x, int32_t y);
void touch_up(void);
// Press, hold for ms, release, then let LVGL settle for settle_ms.
void touch_hold(int32_t x, int32_t y, uint32_t ms, uint32_t settle_ms);
// A quick tap (60 ms press), then settle_ms.
void touch_tap(int32_t x, int32_t y, uint32_t settle_ms);

// Render now and write dir/name.png: true colors, round mask, and (when
// bezel is set) a dark ring around the circle, which makes the image 6 px wider.
void render_set_output(const char *dir, bool bezel);
bool render_save(const char *name);
