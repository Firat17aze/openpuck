#include "puck_timer.h"
#include "unity.h"

#include <string.h>

#define DURATION_MS 3300000u  // 55 minutes

static puck_timer_t timer;

void setUp(void)
{
    puck_timer_init(&timer, DURATION_MS);
}

void tearDown(void) {}

static void expect_text(int32_t remaining_ms, const char *expected)
{
    char buf[PUCK_TIMER_TEXT_MAX];
    size_t n = puck_timer_format(remaining_ms, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING(expected, buf);
    TEST_ASSERT_EQUAL(strlen(expected), n);
}

// Rewrite the CRC so a hand-edited buffer fails on its contents, not its CRC.
static void refresh_crc(uint8_t *buf)
{
    uint32_t crc = puck_crc32(buf, 16);
    for (int i = 0; i < 4; i++) {
        buf[16 + i] = (uint8_t)(crc >> (8 * i));
    }
}

static void expect_rejected(const uint8_t *buf, size_t len, puck_state_result_t expected)
{
    puck_timer_t out = { .duration_ms = 111, .elapsed_ms = 222, .run_started_ms = 333,
                         .state = PUCK_TIMER_RUNNING };
    TEST_ASSERT_EQUAL(expected, puck_timer_decode(buf, len, &out));
    TEST_ASSERT_EQUAL_UINT32(111, out.duration_ms);
    TEST_ASSERT_EQUAL_UINT32(222, out.elapsed_ms);
    TEST_ASSERT_EQUAL_UINT32(333, out.run_started_ms);
    TEST_ASSERT_EQUAL(PUCK_TIMER_RUNNING, out.state);
}

static void test_init_is_idle(void)
{
    TEST_ASSERT_EQUAL(PUCK_TIMER_IDLE, timer.state);
    TEST_ASSERT_FALSE(puck_timer_running(&timer));
    TEST_ASSERT_EQUAL_UINT32(0, puck_timer_elapsed_ms(&timer, 123456));
    TEST_ASSERT_EQUAL_INT32((int32_t)DURATION_MS, puck_timer_remaining_ms(&timer, 123456));
    TEST_ASSERT_FALSE(puck_timer_overtime(&timer, 123456));
}

static void test_start_runs_and_accrues(void)
{
    puck_timer_start(&timer, 1000);
    TEST_ASSERT_TRUE(puck_timer_running(&timer));
    TEST_ASSERT_EQUAL_UINT32(0, puck_timer_elapsed_ms(&timer, 1000));
    TEST_ASSERT_EQUAL_UINT32(5000, puck_timer_elapsed_ms(&timer, 6000));
    TEST_ASSERT_EQUAL_INT32((int32_t)DURATION_MS - 5000, puck_timer_remaining_ms(&timer, 6000));
}

static void test_pause_freezes_elapsed(void)
{
    puck_timer_start(&timer, 1000);
    puck_timer_pause(&timer, 4000);
    TEST_ASSERT_EQUAL(PUCK_TIMER_PAUSED, timer.state);
    TEST_ASSERT_FALSE(puck_timer_running(&timer));
    TEST_ASSERT_EQUAL_UINT32(3000, puck_timer_elapsed_ms(&timer, 4000));
    TEST_ASSERT_EQUAL_UINT32(3000, puck_timer_elapsed_ms(&timer, 999999));
}

static void test_resume_accumulates(void)
{
    puck_timer_start(&timer, 1000);
    puck_timer_pause(&timer, 4000);
    puck_timer_start(&timer, 10000);
    TEST_ASSERT_TRUE(puck_timer_running(&timer));
    TEST_ASSERT_EQUAL_UINT32(3000, puck_timer_elapsed_ms(&timer, 10000));
    TEST_ASSERT_EQUAL_UINT32(5000, puck_timer_elapsed_ms(&timer, 12000));
    puck_timer_pause(&timer, 12000);
    TEST_ASSERT_EQUAL_UINT32(5000, puck_timer_elapsed_ms(&timer, 50000));
}

static void test_start_while_running_is_noop(void)
{
    puck_timer_start(&timer, 1000);
    puck_timer_start(&timer, 5000);
    TEST_ASSERT_EQUAL_UINT32(1000, timer.run_started_ms);
    TEST_ASSERT_EQUAL_UINT32(6000, puck_timer_elapsed_ms(&timer, 7000));
}

static void test_pause_while_not_running_is_noop(void)
{
    puck_timer_pause(&timer, 500);
    TEST_ASSERT_EQUAL(PUCK_TIMER_IDLE, timer.state);
    TEST_ASSERT_EQUAL_UINT32(0, timer.elapsed_ms);

    puck_timer_start(&timer, 0);
    puck_timer_pause(&timer, 2000);
    puck_timer_pause(&timer, 9000);
    TEST_ASSERT_EQUAL(PUCK_TIMER_PAUSED, timer.state);
    TEST_ASSERT_EQUAL_UINT32(2000, timer.elapsed_ms);
}

static void test_reset_returns_to_idle_keeping_duration(void)
{
    puck_timer_start(&timer, 0);
    puck_timer_pause(&timer, 2000);
    puck_timer_reset(&timer);
    TEST_ASSERT_EQUAL(PUCK_TIMER_IDLE, timer.state);
    TEST_ASSERT_EQUAL_UINT32(0, puck_timer_elapsed_ms(&timer, 5000));
    TEST_ASSERT_EQUAL_UINT32(DURATION_MS, timer.duration_ms);

    puck_timer_start(&timer, 100);
    puck_timer_reset(&timer);
    TEST_ASSERT_FALSE(puck_timer_running(&timer));
    TEST_ASSERT_EQUAL_UINT32(0, puck_timer_elapsed_ms(&timer, 9000));
}

static void test_set_duration_while_running_keeps_elapsed(void)
{
    puck_timer_start(&timer, 0);
    puck_timer_set_duration(&timer, 600000);
    TEST_ASSERT_TRUE(puck_timer_running(&timer));
    TEST_ASSERT_EQUAL_UINT32(60000, puck_timer_elapsed_ms(&timer, 60000));
    TEST_ASSERT_EQUAL_INT32(540000, puck_timer_remaining_ms(&timer, 60000));
}

static void test_remaining_goes_negative_in_overtime(void)
{
    puck_timer_set_duration(&timer, 10000);
    puck_timer_start(&timer, 0);
    TEST_ASSERT_EQUAL_INT32(1, puck_timer_remaining_ms(&timer, 9999));
    TEST_ASSERT_FALSE(puck_timer_overtime(&timer, 9999));
    TEST_ASSERT_EQUAL_INT32(0, puck_timer_remaining_ms(&timer, 10000));
    TEST_ASSERT_FALSE(puck_timer_overtime(&timer, 10000));
    TEST_ASSERT_EQUAL_INT32(-1, puck_timer_remaining_ms(&timer, 10001));
    TEST_ASSERT_TRUE(puck_timer_overtime(&timer, 10001));
    TEST_ASSERT_EQUAL_INT32(-5000, puck_timer_remaining_ms(&timer, 15000));
}

static void test_remaining_saturates(void)
{
    puck_timer_t t;
    puck_timer_init(&t, UINT32_MAX);
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, puck_timer_remaining_ms(&t, 0));

    puck_timer_init(&t, 1);
    t.state = PUCK_TIMER_PAUSED;
    t.elapsed_ms = UINT32_MAX;
    TEST_ASSERT_EQUAL_INT32(INT32_MIN, puck_timer_remaining_ms(&t, 0));
}

static void test_handles_millisecond_wraparound(void)
{
    const uint32_t near_wrap = UINT32_MAX - 1000;
    puck_timer_start(&timer, near_wrap);
    TEST_ASSERT_EQUAL_UINT32(3000, puck_timer_elapsed_ms(&timer, near_wrap + 3000));
    puck_timer_pause(&timer, near_wrap + 3000);
    TEST_ASSERT_EQUAL_UINT32(3000, timer.elapsed_ms);

    puck_timer_start(&timer, near_wrap + 3000);
    TEST_ASSERT_EQUAL_UINT32(5000, puck_timer_elapsed_ms(&timer, near_wrap + 5000));
}

static void test_format_countdown(void)
{
    expect_text(3300000, "55:00");
    expect_text(3299001, "55:00");
    expect_text(3299000, "54:59");
    expect_text(60000, "1:00");
    expect_text(59999, "1:00");
    expect_text(59000, "0:59");
    expect_text(1000, "0:01");
    expect_text(1, "0:01");
    expect_text(0, "0:00");
}

static void test_format_minutes_may_exceed_59(void)
{
    expect_text(180 * 60 * 1000, "180:00");
    expect_text(INT32_MAX, "35791:24");
}

static void test_format_overtime(void)
{
    expect_text(-1, "+0:00");
    expect_text(-999, "+0:00");
    expect_text(-1000, "+0:01");
    expect_text(-42999, "+0:42");
    expect_text(-60000, "+1:00");
    expect_text(-61999, "+1:01");
    expect_text(INT32_MIN, "+35791:23");
}

static void test_format_rejects_small_buffer(void)
{
    char buf[5];
    memset(buf, 'x', sizeof(buf));
    TEST_ASSERT_EQUAL(0, puck_timer_format(3300000, buf, 5));  // needs 6 with NUL
    for (size_t i = 0; i < sizeof(buf); i++) {
        TEST_ASSERT_EQUAL_CHAR('x', buf[i]);
    }
    TEST_ASSERT_EQUAL(0, puck_timer_format(0, buf, 0));
    TEST_ASSERT_EQUAL(0, puck_timer_format(0, buf, 4));  // "0:00" + NUL needs 5
    TEST_ASSERT_EQUAL(4, puck_timer_format(0, buf, 5));
    TEST_ASSERT_EQUAL_STRING("0:00", buf);
}

static void test_encode_rejects_small_buffer(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    TEST_ASSERT_EQUAL(0, puck_timer_encode(&timer, 0, buf, PUCK_TIMER_STATE_SIZE - 1));
    TEST_ASSERT_EQUAL(0, puck_timer_encode(&timer, 0, buf, 0));
}

// Released fixtures must NEVER be edited: they pin the on-flash v1 format, so
// a change here means saved timers from shipped firmware would stop loading.
// Paused, duration 3300000 ms, elapsed 600000 ms; CRC computed with Python's
// zlib.crc32 independently of this code.
static const uint8_t V1_PAUSED_FIXTURE[PUCK_TIMER_STATE_SIZE] = {
    0x4f, 0x50, 0x54, 0x4d, 0x01, 0x02, 0x00, 0x00,
    0xa0, 0x5a, 0x32, 0x00, 0xc0, 0x27, 0x09, 0x00,
    0xa3, 0x8c, 0xae, 0x3b,
};

static void test_encode_matches_v1_fixture(void)
{
    puck_timer_start(&timer, 0);
    puck_timer_pause(&timer, 600000);
    uint8_t buf[PUCK_TIMER_STATE_SIZE + 4];
    memset(buf, 0xEE, sizeof(buf));
    TEST_ASSERT_EQUAL(PUCK_TIMER_STATE_SIZE, puck_timer_encode(&timer, 777, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(V1_PAUSED_FIXTURE, buf, PUCK_TIMER_STATE_SIZE);
    TEST_ASSERT_EQUAL_HEX8(0xEE, buf[PUCK_TIMER_STATE_SIZE]);
}

static void test_v1_fixture_decodes(void)
{
    puck_timer_t out;
    TEST_ASSERT_EQUAL(PUCK_STATE_OK,
                      puck_timer_decode(V1_PAUSED_FIXTURE, sizeof(V1_PAUSED_FIXTURE), &out));
    TEST_ASSERT_EQUAL(PUCK_TIMER_PAUSED, out.state);
    TEST_ASSERT_EQUAL_UINT32(3300000, out.duration_ms);
    TEST_ASSERT_EQUAL_UINT32(600000, out.elapsed_ms);
}

static void test_round_trip_idle(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    TEST_ASSERT_EQUAL(PUCK_TIMER_STATE_SIZE, puck_timer_encode(&timer, 50, buf, sizeof(buf)));
    puck_timer_t out;
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_timer_decode(buf, sizeof(buf), &out));
    TEST_ASSERT_EQUAL(PUCK_TIMER_IDLE, out.state);
    TEST_ASSERT_EQUAL_UINT32(DURATION_MS, out.duration_ms);
    TEST_ASSERT_EQUAL_UINT32(0, out.elapsed_ms);
}

static void test_round_trip_running_decodes_paused(void)
{
    puck_timer_start(&timer, UINT32_MAX - 99);
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    uint32_t now = UINT32_MAX - 99 + 4000;  // wrapped
    TEST_ASSERT_EQUAL(PUCK_TIMER_STATE_SIZE, puck_timer_encode(&timer, now, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_UINT8(PUCK_TIMER_RUNNING, buf[5]);
    puck_timer_t out;
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_timer_decode(buf, sizeof(buf), &out));
    TEST_ASSERT_EQUAL(PUCK_TIMER_PAUSED, out.state);
    TEST_ASSERT_EQUAL_UINT32(DURATION_MS, out.duration_ms);
    TEST_ASSERT_EQUAL_UINT32(4000, out.elapsed_ms);
    TEST_ASSERT_EQUAL_UINT32(4000, puck_timer_elapsed_ms(&out, 123456));
}

static void test_round_trip_paused(void)
{
    puck_timer_start(&timer, 10);
    puck_timer_pause(&timer, 8010);
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    TEST_ASSERT_EQUAL(PUCK_TIMER_STATE_SIZE, puck_timer_encode(&timer, 99999, buf, sizeof(buf)));
    puck_timer_t out;
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_timer_decode(buf, sizeof(buf), &out));
    TEST_ASSERT_EQUAL(PUCK_TIMER_PAUSED, out.state);
    TEST_ASSERT_EQUAL_UINT32(8000, out.elapsed_ms);
}

static void test_rejects_wrong_size(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE + 1] = { 0 };
    expect_rejected(buf, 0, PUCK_STATE_ERR_SIZE);
    expect_rejected(buf, PUCK_TIMER_STATE_SIZE - 1, PUCK_STATE_ERR_SIZE);
    expect_rejected(buf, PUCK_TIMER_STATE_SIZE + 1, PUCK_STATE_ERR_SIZE);
}

static void test_rejects_bad_magic(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    memcpy(buf, V1_PAUSED_FIXTURE, sizeof(buf));
    buf[0] = 'X';
    refresh_crc(buf);
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_MAGIC);
}

static void test_rejects_unknown_version(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    memcpy(buf, V1_PAUSED_FIXTURE, sizeof(buf));
    buf[4] = 2;
    refresh_crc(buf);
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_VERSION);
}

static void test_rejects_corruption(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    for (size_t i = 5; i < PUCK_TIMER_STATE_SIZE; i++) {
        memcpy(buf, V1_PAUSED_FIXTURE, sizeof(buf));
        buf[i] ^= 0x01;
        expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_CRC);
    }
}

static void test_rejects_unknown_state(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    memcpy(buf, V1_PAUSED_FIXTURE, sizeof(buf));
    buf[5] = 3;
    refresh_crc(buf);
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_INVALID);
}

static void test_rejects_zero_duration(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    memcpy(buf, V1_PAUSED_FIXTURE, sizeof(buf));
    memset(buf + 8, 0, 4);
    refresh_crc(buf);
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_INVALID);
}

static void test_rejects_idle_with_elapsed_time(void)
{
    uint8_t buf[PUCK_TIMER_STATE_SIZE];
    memcpy(buf, V1_PAUSED_FIXTURE, sizeof(buf));
    buf[5] = PUCK_TIMER_IDLE;
    refresh_crc(buf);
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_INVALID);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_is_idle);
    RUN_TEST(test_start_runs_and_accrues);
    RUN_TEST(test_pause_freezes_elapsed);
    RUN_TEST(test_resume_accumulates);
    RUN_TEST(test_start_while_running_is_noop);
    RUN_TEST(test_pause_while_not_running_is_noop);
    RUN_TEST(test_reset_returns_to_idle_keeping_duration);
    RUN_TEST(test_set_duration_while_running_keeps_elapsed);
    RUN_TEST(test_remaining_goes_negative_in_overtime);
    RUN_TEST(test_remaining_saturates);
    RUN_TEST(test_handles_millisecond_wraparound);
    RUN_TEST(test_format_countdown);
    RUN_TEST(test_format_minutes_may_exceed_59);
    RUN_TEST(test_format_overtime);
    RUN_TEST(test_format_rejects_small_buffer);
    RUN_TEST(test_encode_rejects_small_buffer);
    RUN_TEST(test_encode_matches_v1_fixture);
    RUN_TEST(test_v1_fixture_decodes);
    RUN_TEST(test_round_trip_idle);
    RUN_TEST(test_round_trip_running_decodes_paused);
    RUN_TEST(test_round_trip_paused);
    RUN_TEST(test_rejects_wrong_size);
    RUN_TEST(test_rejects_bad_magic);
    RUN_TEST(test_rejects_unknown_version);
    RUN_TEST(test_rejects_corruption);
    RUN_TEST(test_rejects_unknown_state);
    RUN_TEST(test_rejects_zero_duration);
    RUN_TEST(test_rejects_idle_with_elapsed_time);
    return UNITY_END();
}
