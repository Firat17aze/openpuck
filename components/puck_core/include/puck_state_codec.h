// Saved-state format. Versioned and CRC-protected, so a corrupt or foreign
// blob is rejected instead of loaded.
//
// Version 1 layout (little-endian, 16 bytes):
//   0  magic "OPCK"
//   4  version (1)
//   5  player_count
//   6  starting_life  (int16)
//   8  life[0..1]     (int16 x PUCK_MAX_PLAYERS)
//   12 crc32 of bytes 0..11 (IEEE 802.3)
// Never change a released layout: add a new version and keep decoding the old.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "puck_game.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUCK_STATE_VERSION   1
#define PUCK_STATE_SIZE      16

typedef enum {
    PUCK_STATE_OK = 0,
    PUCK_STATE_ERR_SIZE,
    PUCK_STATE_ERR_MAGIC,
    PUCK_STATE_ERR_VERSION,
    PUCK_STATE_ERR_CRC,
    PUCK_STATE_ERR_INVALID,
} puck_state_result_t;

// Encode into buf. Returns bytes written, or 0 if cap is too small.
size_t puck_state_encode(const puck_game_t *game, uint8_t *buf, size_t cap);

// Decode and validate. *game is only written on PUCK_STATE_OK.
puck_state_result_t puck_state_decode(const uint8_t *buf, size_t len, puck_game_t *game);

uint32_t puck_crc32(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
