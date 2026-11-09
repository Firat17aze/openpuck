#include "puck_history.h"

#include <string.h>

#include "puck_game.h"

static const uint8_t MAGIC[4] = { 'O', 'P', 'H', 'S' };
#define HISTORY_VERSION 1
#define HEADER_SIZE     7
#define ENTRY_SIZE      5
#define CRC_SIZE        4

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

// Ring index of the entry `offset` places after the oldest.
static uint16_t slot(const puck_history_t *history, uint16_t offset)
{
    return (uint16_t)((history->start + offset) % PUCK_HISTORY_CAPACITY);
}

void puck_history_clear(puck_history_t *history)
{
    history->start = 0;
    history->count = 0;
    history->can_combine = false;
}

void puck_history_init(puck_history_t *history, uint32_t combine_ms)
{
    puck_history_clear(history);
    history->combine_ms = combine_ms;
    history->last_change_ms = 0;
}

void puck_history_record(puck_history_t *history, uint8_t player, int16_t delta,
                         int16_t total, uint32_t now_ms)
{
    if (delta == 0) {
        return;
    }
    if (history->can_combine && history->count > 0 &&
        now_ms - history->last_change_ms < history->combine_ms) {
        puck_history_entry_t *newest =
            &history->entries[slot(history, (uint16_t)(history->count - 1))];
        if (newest->player == player) {
            int32_t sum = (int32_t)newest->delta + delta;
            if (sum > INT16_MAX) {
                sum = INT16_MAX;
            } else if (sum < INT16_MIN) {
                sum = INT16_MIN;
            }
            if (sum == 0) {
                history->count--;
                history->can_combine = false;
                return;
            }
            newest->delta = (int16_t)sum;
            newest->total = total;
            history->last_change_ms = now_ms;
            return;
        }
    }

    const puck_history_entry_t entry = { .player = player, .delta = delta, .total = total };
    if (history->count == PUCK_HISTORY_CAPACITY) {
        history->entries[history->start] = entry;
        history->start = slot(history, 1);
    } else {
        history->entries[slot(history, history->count)] = entry;
        history->count++;
    }
    history->last_change_ms = now_ms;
    history->can_combine = true;
}

uint16_t puck_history_count(const puck_history_t *history)
{
    return history->count;
}

const puck_history_entry_t *puck_history_get(const puck_history_t *history, uint16_t index)
{
    if (index >= history->count) {
        return NULL;
    }
    return &history->entries[slot(history, (uint16_t)(history->count - 1 - index))];
}

size_t puck_history_encode(const puck_history_t *history, uint8_t *buf, size_t cap)
{
    const size_t size = HEADER_SIZE + ENTRY_SIZE * (size_t)history->count + CRC_SIZE;
    if (cap < size) {
        return 0;
    }
    memcpy(buf, MAGIC, sizeof(MAGIC));
    buf[4] = HISTORY_VERSION;
    put_u16(buf + 5, history->count);
    uint8_t *p = buf + HEADER_SIZE;
    for (uint16_t i = 0; i < history->count; i++) {
        const puck_history_entry_t *e = &history->entries[slot(history, i)];
        p[0] = e->player;
        put_u16(p + 1, (uint16_t)e->delta);
        put_u16(p + 3, (uint16_t)e->total);
        p += ENTRY_SIZE;
    }
    put_u32(p, puck_crc32(buf, size - CRC_SIZE));
    return size;
}

puck_state_result_t puck_history_decode(const uint8_t *buf, size_t len,
                                        puck_history_t *history, uint32_t combine_ms)
{
    if (len < HEADER_SIZE + CRC_SIZE) {
        return PUCK_STATE_ERR_SIZE;
    }
    const uint16_t count = get_u16(buf + 5);
    if (len != HEADER_SIZE + ENTRY_SIZE * (size_t)count + CRC_SIZE) {
        return PUCK_STATE_ERR_SIZE;
    }
    if (memcmp(buf, MAGIC, sizeof(MAGIC)) != 0) {
        return PUCK_STATE_ERR_MAGIC;
    }
    if (buf[4] != HISTORY_VERSION) {
        return PUCK_STATE_ERR_VERSION;
    }
    if (get_u32(buf + len - CRC_SIZE) != puck_crc32(buf, len - CRC_SIZE)) {
        return PUCK_STATE_ERR_CRC;
    }
    if (count > PUCK_HISTORY_CAPACITY) {
        return PUCK_STATE_ERR_INVALID;
    }

    puck_history_t decoded;
    puck_history_init(&decoded, combine_ms);
    const uint8_t *p = buf + HEADER_SIZE;
    for (uint16_t i = 0; i < count; i++) {
        puck_history_entry_t *e = &decoded.entries[i];
        e->player = p[0];
        e->delta = (int16_t)get_u16(p + 1);
        e->total = (int16_t)get_u16(p + 3);
        if (e->player >= PUCK_MAX_PLAYERS || e->delta == 0 ||
            e->total < PUCK_LIFE_MIN || e->total > PUCK_LIFE_MAX) {
            return PUCK_STATE_ERR_INVALID;
        }
        p += ENTRY_SIZE;
    }
    decoded.count = count;
    *history = decoded;
    return PUCK_STATE_OK;
}
