// History of changes since the last new game, newest first.
//
// Quick bursts on the same player are combined into one entry: a change made
// within combine_ms of the previous change to the same player adds to the
// newest entry instead of creating a new one. If a combined entry's delta
// becomes 0 it is removed (net no change). When full, the oldest entry is
// dropped. Entries have no timestamps (display order only).
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "puck_state_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUCK_HISTORY_CAPACITY 100

typedef struct {
    uint8_t player;
    int16_t delta;   // net change of this entry
    int16_t total;   // player's total after the change
} puck_history_entry_t;

typedef struct {
    puck_history_entry_t entries[PUCK_HISTORY_CAPACITY];  // ring buffer
    uint16_t start;  // index of the oldest entry
    uint16_t count;
    uint32_t combine_ms;
    uint32_t last_change_ms;
    bool can_combine;  // newest entry may still absorb changes
} puck_history_t;

void puck_history_init(puck_history_t *history, uint32_t combine_ms);
void puck_history_clear(puck_history_t *history);

// Record an applied change (pass the actual delta after clamping, and the new
// total). delta == 0 is ignored.
void puck_history_record(puck_history_t *history, uint8_t player, int16_t delta,
                         int16_t total, uint32_t now_ms);

uint16_t puck_history_count(const puck_history_t *history);

// index 0 = newest. Returns NULL if index >= count.
const puck_history_entry_t *puck_history_get(const puck_history_t *history, uint16_t index);

// Saved form (versioned, CRC-protected, magic "OPHS"): header, count, then 5
// bytes per entry (player, delta, total), oldest first, then CRC32.
#define PUCK_HISTORY_ENCODED_MAX (4 + 1 + 2 + PUCK_HISTORY_CAPACITY * 5 + 4)
size_t puck_history_encode(const puck_history_t *history, uint8_t *buf, size_t cap);
// On success replaces *history (combine state reset: the next change starts a
// new entry). *history is untouched on error.
puck_state_result_t puck_history_decode(const uint8_t *buf, size_t len,
                                        puck_history_t *history, uint32_t combine_ms);

#ifdef __cplusplus
}
#endif
