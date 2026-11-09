#pragma once

#include <stdbool.h>
#include <stdint.h>

// Drive the power latch high. Call first thing in app_main.
void power_hold_on(void);

// Turn on automatic light sleep (CPU 160 MHz max, 40 MHz min) and keep every
// pin we use in its normal configuration while asleep. Call right after
// power_hold_on, before anything else is set up.
void power_sleep_enable(void);

// Start the power task: debounced button handling, long-press power off, and
// the inactivity auto-off. Runs independently of the UI task.
void power_start(void);

// Enable or disable the idle auto-off and set its delay. Callable from any
// task; takes effect immediately and counts as activity.
void power_configure_auto_off(bool enabled, uint32_t minutes);

// While inhibit() returns true the puck counts as active (e.g. a running
// timer). Called on the power task, so it must be quick and task-safe.
void power_set_auto_off_inhibitor(bool (*inhibit)(void));

// Record user activity (touch, button) to postpone the inactivity auto-off.
void power_note_activity(void);

// Called just before the latch is released, with a short human-readable
// reason. Runs on the power task, so it must not block for long.
typedef void (*power_off_cb_t)(const char *reason);
void power_set_off_callback(power_off_cb_t cb);

// Called on a short press of the power button. Runs on the power task.
typedef void (*power_press_cb_t)(void);
void power_set_short_press_callback(power_press_cb_t cb);
