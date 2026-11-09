#include "puck_state_codec.h"

#include <string.h>

static const uint8_t MAGIC[4] = { 'O', 'P', 'C', 'K' };
#define CRC_OFFSET 12

uint32_t puck_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put_u32(uint8_t *p, uint32_t v)
{
    put_u16(p, (uint16_t)v);
    put_u16(p + 2, (uint16_t)(v >> 16));
}

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t get_u32(const uint8_t *p)
{
    return get_u16(p) | ((uint32_t)get_u16(p + 2) << 16);
}

size_t puck_state_encode(const puck_game_t *game, uint8_t *buf, size_t cap)
{
    if (cap < PUCK_STATE_SIZE) {
        return 0;
    }
    memcpy(buf, MAGIC, sizeof(MAGIC));
    buf[4] = PUCK_STATE_VERSION;
    buf[5] = game->player_count;
    put_u16(buf + 6, (uint16_t)game->starting_life);
    for (int i = 0; i < PUCK_MAX_PLAYERS; i++) {
        put_u16(buf + 8 + 2 * i, (uint16_t)game->life[i]);
    }
    put_u32(buf + CRC_OFFSET, puck_crc32(buf, CRC_OFFSET));
    return PUCK_STATE_SIZE;
}

puck_state_result_t puck_state_decode(const uint8_t *buf, size_t len, puck_game_t *game)
{
    if (len != PUCK_STATE_SIZE) {
        return PUCK_STATE_ERR_SIZE;
    }
    if (memcmp(buf, MAGIC, sizeof(MAGIC)) != 0) {
        return PUCK_STATE_ERR_MAGIC;
    }
    if (buf[4] != PUCK_STATE_VERSION) {
        return PUCK_STATE_ERR_VERSION;
    }
    if (get_u32(buf + CRC_OFFSET) != puck_crc32(buf, CRC_OFFSET)) {
        return PUCK_STATE_ERR_CRC;
    }
    puck_game_t decoded = {
        .player_count = buf[5],
        .starting_life = (int16_t)get_u16(buf + 6),
    };
    for (int i = 0; i < PUCK_MAX_PLAYERS; i++) {
        decoded.life[i] = (int16_t)get_u16(buf + 8 + 2 * i);
    }
    if (!puck_game_is_valid(&decoded)) {
        return PUCK_STATE_ERR_INVALID;
    }
    *game = decoded;
    return PUCK_STATE_OK;
}
