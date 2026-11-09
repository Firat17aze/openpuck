#include "puck_game.h"

#include <string.h>

static bool valid_setup(uint8_t player_count, int16_t starting_life)
{
    return player_count >= 1 && player_count <= PUCK_MAX_PLAYERS &&
           starting_life >= PUCK_STARTING_LIFE_MIN && starting_life <= PUCK_STARTING_LIFE_MAX;
}

bool puck_game_new(puck_game_t *game, uint8_t player_count, int16_t starting_life)
{
    if (!valid_setup(player_count, starting_life)) {
        return false;
    }
    memset(game, 0, sizeof(*game));
    game->player_count = player_count;
    game->starting_life = starting_life;
    for (uint8_t i = 0; i < player_count; i++) {
        game->life[i] = starting_life;
    }
    return true;
}

bool puck_game_adjust(puck_game_t *game, uint8_t player, int16_t delta)
{
    if (player >= game->player_count) {
        return false;
    }
    int32_t life = (int32_t)game->life[player] + delta;
    if (life < PUCK_LIFE_MIN) {
        life = PUCK_LIFE_MIN;
    } else if (life > PUCK_LIFE_MAX) {
        life = PUCK_LIFE_MAX;
    }
    if (life == game->life[player]) {
        return false;
    }
    game->life[player] = (int16_t)life;
    return true;
}

void puck_game_restart(puck_game_t *game)
{
    puck_game_new(game, game->player_count, game->starting_life);
}

bool puck_game_is_valid(const puck_game_t *game)
{
    if (!valid_setup(game->player_count, game->starting_life)) {
        return false;
    }
    for (uint8_t i = 0; i < PUCK_MAX_PLAYERS; i++) {
        int16_t life = game->life[i];
        if (i >= game->player_count ? life != 0 : (life < PUCK_LIFE_MIN || life > PUCK_LIFE_MAX)) {
            return false;
        }
    }
    return true;
}
