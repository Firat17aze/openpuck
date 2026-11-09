// Game timer: counts down from a set duration, then into overtime.
//
// Time is passed in as milliseconds (uint32_t, wraparound-safe). The puck has
// no clock that survives power-off, so a timer restored from flash comes back
// PAUSED (if it was running or paused) with the elapsed time it had when saved.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "puck_state_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PUCK_TIMER_IDLE = 0,     // not started since the last reset
    PUCK_TIMER_RUNNING,
    PUCK_TIMER_PAUSED,
} puck_timer_state_t;

typedef struct {
    uint32_t duration_ms;
    uint32_t elapsed_ms;     // time accrued before the current run
    uint32_t run_started_ms; // valid while RUNNING
    puck_timer_state_t state;
} puck_timer_t;

void puck_timer_init(puck_timer_t *timer, uint32_t duration_ms);

// Change the duration. Elapsed time is kept, so remaining time changes.
void puck_timer_set_duration(puck_timer_t *timer, uint32_t duration_ms);

// IDLE/PAUSED -> RUNNING. No effect if already running.
void puck_timer_start(puck_timer_t *timer, uint32_t now_ms);

// RUNNING -> PAUSED, keeping the elapsed time. No effect otherwise.
void puck_timer_pause(puck_timer_t *timer, uint32_t now_ms);

// Back to IDLE with zero elapsed time (duration kept).
void puck_timer_reset(puck_timer_t *timer);

bool puck_timer_running(const puck_timer_t *timer);
uint32_t puck_timer_elapsed_ms(const puck_timer_t *timer, uint32_t now_ms);

// duration - elapsed. Negative values are overtime.
int32_t puck_timer_remaining_ms(const puck_timer_t *timer, uint32_t now_ms);
bool puck_timer_overtime(const puck_timer_t *timer, uint32_t now_ms);

// Format remaining time for display into buf (NUL-terminated); returns length,
// or 0 if cap is too small (needs at most PUCK_TIMER_TEXT_MAX bytes).
//   counting down: "M:SS", minutes not zero-padded, seconds rounded UP so the
//                  display reads 0:00 exactly when time runs out
//                  (e.g. 3300000 ms -> "55:00", 1 ms -> "0:01", 0 -> "0:00")
//   overtime:      "+M:SS", seconds rounded DOWN (e.g. -42999 ms -> "+0:42")
#define PUCK_TIMER_TEXT_MAX 12
size_t puck_timer_format(int32_t remaining_ms, char *buf, size_t cap);

// Saved form, version 1 (little-endian, 20 bytes):
//   0 magic "OPTM", 4 version (1), 5 state (puck_timer_state_t as saved),
//   6 reserved (2 bytes, 0), 8 duration_ms (u32), 12 elapsed_ms (u32, as of
//   now_ms), 16 crc32 of bytes 0..15.
#define PUCK_TIMER_STATE_SIZE 20
size_t puck_timer_encode(const puck_timer_t *timer, uint32_t now_ms, uint8_t *buf, size_t cap);
// On success *timer is IDLE (if saved IDLE) or PAUSED with the saved elapsed
// time. *timer is untouched on error.
puck_state_result_t puck_timer_decode(const uint8_t *buf, size_t len, puck_timer_t *timer);

#ifdef __cplusplus
}
#endif
