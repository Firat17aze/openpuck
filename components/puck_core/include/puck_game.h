// Game state: players and their life totals.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PUCK_MAX_PLAYERS        2
#define PUCK_LIFE_MIN           0       // totals never go negative
#define PUCK_LIFE_MAX           99      // two digits always fit the screen
#define PUCK_STARTING_LIFE_MIN  0
#define PUCK_STARTING_LIFE_MAX  99

typedef struct {
    uint8_t player_count;
    int16_t starting_life;
    int16_t life[PUCK_MAX_PLAYERS];
} puck_game_t;

// Start a new game. Returns false (and leaves *game untouched) if the player
// count or starting life is out of range.
bool puck_game_new(puck_game_t *game, uint8_t player_count, int16_t starting_life);

// Add delta to a player's life, clamped to [PUCK_LIFE_MIN, PUCK_LIFE_MAX].
// Returns true if the life total changed.
bool puck_game_adjust(puck_game_t *game, uint8_t player, int16_t delta);

// Restart with the same player count and starting life.
void puck_game_restart(puck_game_t *game);

// True if every field is within range (used to vet loaded state).
bool puck_game_is_valid(const puck_game_t *game);

#ifdef __cplusplus
}
#endif
