#include "puck_timer.h"

#include <stdio.h>
#include <string.h>

static const uint8_t MAGIC[4] = { 'O', 'P', 'T', 'M' };
#define TIMER_VERSION 1
#define CRC_OFFSET    16

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

void puck_timer_init(puck_timer_t *timer, uint32_t duration_ms)
{
    timer->duration_ms = duration_ms;
    timer->elapsed_ms = 0;
    timer->run_started_ms = 0;
    timer->state = PUCK_TIMER_IDLE;
}

void puck_timer_set_duration(puck_timer_t *timer, uint32_t duration_ms)
{
    timer->duration_ms = duration_ms;
}

void puck_timer_start(puck_timer_t *timer, uint32_t now_ms)
{
    if (timer->state == PUCK_TIMER_RUNNING) {
        return;
    }
    timer->run_started_ms = now_ms;
    timer->state = PUCK_TIMER_RUNNING;
}

void puck_timer_pause(puck_timer_t *timer, uint32_t now_ms)
{
    if (timer->state != PUCK_TIMER_RUNNING) {
        return;
    }
    timer->elapsed_ms = puck_timer_elapsed_ms(timer, now_ms);
    timer->state = PUCK_TIMER_PAUSED;
}

void puck_timer_reset(puck_timer_t *timer)
{
    timer->elapsed_ms = 0;
    timer->run_started_ms = 0;
    timer->state = PUCK_TIMER_IDLE;
}

bool puck_timer_running(const puck_timer_t *timer)
{
    return timer->state == PUCK_TIMER_RUNNING;
}

uint32_t puck_timer_elapsed_ms(const puck_timer_t *timer, uint32_t now_ms)
{
    if (timer->state != PUCK_TIMER_RUNNING) {
        return timer->elapsed_ms;
    }
    return timer->elapsed_ms + (now_ms - timer->run_started_ms);
}

int32_t puck_timer_remaining_ms(const puck_timer_t *timer, uint32_t now_ms)
{
    int64_t remaining = (int64_t)timer->duration_ms - (int64_t)puck_timer_elapsed_ms(timer, now_ms);
    if (remaining > INT32_MAX) {
        return INT32_MAX;
    }
    if (remaining < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)remaining;
}

bool puck_timer_overtime(const puck_timer_t *timer, uint32_t now_ms)
{
    return puck_timer_remaining_ms(timer, now_ms) < 0;
}

size_t puck_timer_format(int32_t remaining_ms, char *buf, size_t cap)
{
    // Format into scratch space so a too-small buf is never written.
    char text[PUCK_TIMER_TEXT_MAX];
    int n;
    if (remaining_ms >= 0) {
        // Widen first: remaining_ms + 999 can exceed INT32_MAX.
        uint32_t secs = (uint32_t)(((int64_t)remaining_ms + 999) / 1000);
        n = snprintf(text, sizeof(text), "%u:%02u", (unsigned)(secs / 60), (unsigned)(secs % 60));
    } else {
        uint32_t secs = (uint32_t)((-(int64_t)remaining_ms) / 1000);
        n = snprintf(text, sizeof(text), "+%u:%02u", (unsigned)(secs / 60), (unsigned)(secs % 60));
    }
    size_t len = (size_t)n;
    if (len >= cap) {
        return 0;
    }
    memcpy(buf, text, len + 1);
    return len;
}

size_t puck_timer_encode(const puck_timer_t *timer, uint32_t now_ms, uint8_t *buf, size_t cap)
{
    if (cap < PUCK_TIMER_STATE_SIZE) {
        return 0;
    }
    memset(buf, 0, PUCK_TIMER_STATE_SIZE);
    memcpy(buf, MAGIC, sizeof(MAGIC));
    buf[4] = TIMER_VERSION;
    buf[5] = (uint8_t)timer->state;
    put_u32(buf + 8, timer->duration_ms);
    put_u32(buf + 12, puck_timer_elapsed_ms(timer, now_ms));
    put_u32(buf + CRC_OFFSET, puck_crc32(buf, CRC_OFFSET));
    return PUCK_TIMER_STATE_SIZE;
}

puck_state_result_t puck_timer_decode(const uint8_t *buf, size_t len, puck_timer_t *timer)
{
    if (len != PUCK_TIMER_STATE_SIZE) {
        return PUCK_STATE_ERR_SIZE;
    }
    if (memcmp(buf, MAGIC, sizeof(MAGIC)) != 0) {
        return PUCK_STATE_ERR_MAGIC;
    }
    if (buf[4] != TIMER_VERSION) {
        return PUCK_STATE_ERR_VERSION;
    }
    if (get_u32(buf + CRC_OFFSET) != puck_crc32(buf, CRC_OFFSET)) {
        return PUCK_STATE_ERR_CRC;
    }
    uint8_t state = buf[5];
    uint32_t duration_ms = get_u32(buf + 8);
    uint32_t elapsed_ms = get_u32(buf + 12);
    if (state > PUCK_TIMER_PAUSED || duration_ms == 0) {
        return PUCK_STATE_ERR_INVALID;
    }
    if (state == PUCK_TIMER_IDLE && elapsed_ms != 0) {
        return PUCK_STATE_ERR_INVALID;
    }
    timer->duration_ms = duration_ms;
    timer->elapsed_ms = elapsed_ms;
    timer->run_started_ms = 0;
    timer->state = (state == PUCK_TIMER_IDLE) ? PUCK_TIMER_IDLE : PUCK_TIMER_PAUSED;
    return PUCK_STATE_OK;
}
