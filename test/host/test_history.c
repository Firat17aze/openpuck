#include <string.h>

#include "puck_game.h"
#include "puck_history.h"
#include "unity.h"

#define COMBINE_MS 1000

static puck_history_t h;

void setUp(void)
{
    puck_history_init(&h, COMBINE_MS);
}

void tearDown(void) {}

static void assert_entry(uint16_t index, uint8_t player, int16_t delta, int16_t total)
{
    const puck_history_entry_t *e = puck_history_get(&h, index);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_UINT8(player, e->player);
    TEST_ASSERT_EQUAL_INT16(delta, e->delta);
    TEST_ASSERT_EQUAL_INT16(total, e->total);
}

// Recompute the trailing CRC so tests can corrupt contents and still reach the
// content checks.
static void fix_crc(uint8_t *buf, size_t len)
{
    uint32_t crc = puck_crc32(buf, len - 4);
    for (int i = 0; i < 4; i++) {
        buf[len - 4 + (size_t)i] = (uint8_t)(crc >> (8 * i));
    }
}

static void test_starts_empty(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, puck_history_count(&h));
    TEST_ASSERT_NULL(puck_history_get(&h, 0));
}

static void test_separate_players_make_separate_entries(void)
{
    puck_history_record(&h, 0, 1, 1, 0);
    puck_history_record(&h, 1, 1, 1, 10);
    TEST_ASSERT_EQUAL_UINT16(2, puck_history_count(&h));
    assert_entry(0, 1, 1, 1);
    assert_entry(1, 0, 1, 1);
}

static void test_burst_within_window_combines(void)
{
    puck_history_record(&h, 0, 1, 1, 0);
    puck_history_record(&h, 0, 1, 2, 500);
    puck_history_record(&h, 0, 5, 7, 1200);  // window measured from the last change
    TEST_ASSERT_EQUAL_UINT16(1, puck_history_count(&h));
    assert_entry(0, 0, 7, 7);
}

static void test_no_combine_at_exactly_window(void)
{
    puck_history_record(&h, 0, 1, 1, 0);
    puck_history_record(&h, 0, 1, 2, COMBINE_MS - 1);
    puck_history_record(&h, 0, 1, 3, COMBINE_MS - 1 + COMBINE_MS);
    TEST_ASSERT_EQUAL_UINT16(2, puck_history_count(&h));
    assert_entry(0, 0, 1, 3);
    assert_entry(1, 0, 2, 2);
}

static void test_no_combine_after_long_pause(void)
{
    puck_history_record(&h, 0, 1, 1, 0);
    puck_history_record(&h, 0, 1, 2, 60000);
    TEST_ASSERT_EQUAL_UINT16(2, puck_history_count(&h));
}

static void test_switching_player_breaks_combining(void)
{
    puck_history_record(&h, 0, 1, 1, 0);
    puck_history_record(&h, 1, 1, 1, 100);
    puck_history_record(&h, 0, 1, 2, 200);
    TEST_ASSERT_EQUAL_UINT16(3, puck_history_count(&h));
    assert_entry(0, 0, 1, 2);
    assert_entry(1, 1, 1, 1);
    assert_entry(2, 0, 1, 1);
}

static void test_combine_to_zero_removes_entry(void)
{
    puck_history_record(&h, 0, 3, 3, 0);
    puck_history_record(&h, 1, 1, 1, 100);
    puck_history_record(&h, 1, -1, 0, 200);
    TEST_ASSERT_EQUAL_UINT16(1, puck_history_count(&h));
    assert_entry(0, 0, 3, 3);

    // Even the same player within the window starts fresh after a removal.
    puck_history_record(&h, 0, 1, 4, 300);
    TEST_ASSERT_EQUAL_UINT16(2, puck_history_count(&h));
    assert_entry(0, 0, 1, 4);
    assert_entry(1, 0, 3, 3);
}

static void test_combine_to_zero_on_only_entry(void)
{
    puck_history_record(&h, 0, 2, 2, 0);
    puck_history_record(&h, 0, -2, 0, 100);
    TEST_ASSERT_EQUAL_UINT16(0, puck_history_count(&h));
    TEST_ASSERT_NULL(puck_history_get(&h, 0));
}

static void test_zero_delta_is_ignored(void)
{
    puck_history_record(&h, 0, 0, 5, 0);
    TEST_ASSERT_EQUAL_UINT16(0, puck_history_count(&h));

    puck_history_record(&h, 0, 1, 1, 100);
    puck_history_record(&h, 0, 0, 1, 900);  // does not refresh the window
    puck_history_record(&h, 0, 1, 2, 1050);
    TEST_ASSERT_EQUAL_UINT16(1, puck_history_count(&h));
    assert_entry(0, 0, 2, 2);
}

static void test_combine_saturates_at_int16_limits(void)
{
    puck_history_record(&h, 0, INT16_MAX, 100, 0);
    puck_history_record(&h, 0, 10, 110, 10);
    assert_entry(0, 0, INT16_MAX, 110);

    puck_history_clear(&h);
    puck_history_record(&h, 0, INT16_MIN, 100, 0);
    puck_history_record(&h, 0, -10, 90, 10);
    assert_entry(0, 0, INT16_MIN, 90);
    TEST_ASSERT_EQUAL_UINT16(1, puck_history_count(&h));
}

static void test_capacity_drops_oldest(void)
{
    // Alternate players so nothing combines; delta carries the sequence number.
    for (int i = 0; i < PUCK_HISTORY_CAPACITY + 5; i++) {
        puck_history_record(&h, (uint8_t)(i % 2), (int16_t)(i + 1), 1, 0);
    }
    TEST_ASSERT_EQUAL_UINT16(PUCK_HISTORY_CAPACITY, puck_history_count(&h));
    for (uint16_t i = 0; i < PUCK_HISTORY_CAPACITY; i++) {
        const puck_history_entry_t *e = puck_history_get(&h, i);
        TEST_ASSERT_NOT_NULL(e);
        TEST_ASSERT_EQUAL_INT16((int16_t)(PUCK_HISTORY_CAPACITY + 5 - i), e->delta);
    }
    TEST_ASSERT_NULL(puck_history_get(&h, PUCK_HISTORY_CAPACITY));
}

static void test_combining_works_on_wrapped_ring(void)
{
    for (int i = 0; i < PUCK_HISTORY_CAPACITY + 3; i++) {
        puck_history_record(&h, (uint8_t)(i % 2), (int16_t)(i + 1), 1, 0);
    }
    // Newest has delta 103 and player (102 % 2) == 0.
    puck_history_record(&h, 0, 7, 9, 10);
    TEST_ASSERT_EQUAL_UINT16(PUCK_HISTORY_CAPACITY, puck_history_count(&h));
    assert_entry(0, 0, 110, 9);
    assert_entry(1, 1, 102, 1);

    // Combining to zero on a wrapped ring removes just the newest.
    puck_history_record(&h, 0, -110, 0, 20);
    TEST_ASSERT_EQUAL_UINT16(PUCK_HISTORY_CAPACITY - 1, puck_history_count(&h));
    assert_entry(0, 1, 102, 1);
}

static void test_get_out_of_range(void)
{
    puck_history_record(&h, 0, 1, 1, 0);
    TEST_ASSERT_NOT_NULL(puck_history_get(&h, 0));
    TEST_ASSERT_NULL(puck_history_get(&h, 1));
    TEST_ASSERT_NULL(puck_history_get(&h, UINT16_MAX));
}

static void test_clear_empties_and_stops_combining(void)
{
    puck_history_record(&h, 0, 1, 1, 0);
    puck_history_clear(&h);
    TEST_ASSERT_EQUAL_UINT16(0, puck_history_count(&h));
    puck_history_record(&h, 0, 1, 1, 10);
    TEST_ASSERT_EQUAL_UINT16(1, puck_history_count(&h));
    assert_entry(0, 0, 1, 1);
}

static void test_init_sets_combine_window(void)
{
    puck_history_init(&h, 50);
    puck_history_record(&h, 0, 1, 1, 0);
    puck_history_record(&h, 0, 1, 2, 49);
    TEST_ASSERT_EQUAL_UINT16(1, puck_history_count(&h));
    puck_history_record(&h, 0, 1, 3, 99);
    TEST_ASSERT_EQUAL_UINT16(2, puck_history_count(&h));
}

static void test_combining_handles_millisecond_wraparound(void)
{
    const uint32_t near_wrap = UINT32_MAX - 200;
    puck_history_record(&h, 0, 1, 1, near_wrap);
    puck_history_record(&h, 0, 1, 2, near_wrap + 500);  // wrapped, 500 ms later
    TEST_ASSERT_EQUAL_UINT16(1, puck_history_count(&h));
    puck_history_record(&h, 0, 1, 3, near_wrap + 500 + COMBINE_MS);
    TEST_ASSERT_EQUAL_UINT16(2, puck_history_count(&h));
}

// Released fixtures must never be edited: they pin the v1 saved format that
// devices in the field already hold. Add a new fixture for a new version.
// Generated independently with Python struct + zlib.crc32:
//   player 0 +5 -> 5, then player 1 -2 -> 3
static const uint8_t V1_FIXTURE[] = {
    0x4f, 0x50, 0x48, 0x53,        // "OPHS"
    0x01,                          // version
    0x02, 0x00,                    // count
    0x00, 0x05, 0x00, 0x05, 0x00,  // player 0, delta 5, total 5
    0x01, 0xfe, 0xff, 0x03, 0x00,  // player 1, delta -2, total 3
    0xbc, 0x34, 0x04, 0x23,        // crc32
};

static void test_encode_matches_v1_fixture(void)
{
    puck_history_record(&h, 0, 5, 5, 0);
    puck_history_record(&h, 1, -2, 3, 5000);
    uint8_t buf[PUCK_HISTORY_ENCODED_MAX];
    size_t n = puck_history_encode(&h, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_UINT32(sizeof(V1_FIXTURE), n);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(V1_FIXTURE, buf, sizeof(V1_FIXTURE));
}

static void test_decodes_v1_fixture(void)
{
    puck_history_record(&h, 1, 9, 9, 0);  // replaced by decode
    TEST_ASSERT_EQUAL(PUCK_STATE_OK,
                      puck_history_decode(V1_FIXTURE, sizeof(V1_FIXTURE), &h, COMBINE_MS));
    TEST_ASSERT_EQUAL_UINT16(2, puck_history_count(&h));
    assert_entry(0, 1, -2, 3);
    assert_entry(1, 0, 5, 5);
}

static void test_empty_history_round_trip(void)
{
    uint8_t buf[PUCK_HISTORY_ENCODED_MAX];
    size_t n = puck_history_encode(&h, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_UINT32(11, n);
    puck_history_record(&h, 0, 1, 1, 0);
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_history_decode(buf, n, &h, COMBINE_MS));
    TEST_ASSERT_EQUAL_UINT16(0, puck_history_count(&h));
}

static void test_round_trip_full_wrapped_ring(void)
{
    for (int i = 0; i < PUCK_HISTORY_CAPACITY + 37; i++) {
        puck_history_record(&h, (uint8_t)(i % 2), (int16_t)(i + 1),
                            (int16_t)((i * 3) % (PUCK_LIFE_MAX + 1)), 0);
    }
    uint8_t buf[PUCK_HISTORY_ENCODED_MAX];
    size_t n = puck_history_encode(&h, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_UINT32(PUCK_HISTORY_ENCODED_MAX, n);

    puck_history_t out;
    puck_history_init(&out, 123);
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_history_decode(buf, n, &out, 777));
    TEST_ASSERT_EQUAL_UINT16(PUCK_HISTORY_CAPACITY, puck_history_count(&out));
    TEST_ASSERT_EQUAL_UINT32(777, out.combine_ms);
    TEST_ASSERT_EQUAL_UINT16(0, out.start);
    for (uint16_t i = 0; i < PUCK_HISTORY_CAPACITY; i++) {
        const puck_history_entry_t *a = puck_history_get(&h, i);
        const puck_history_entry_t *b = puck_history_get(&out, i);
        TEST_ASSERT_EQUAL_UINT8(a->player, b->player);
        TEST_ASSERT_EQUAL_INT16(a->delta, b->delta);
        TEST_ASSERT_EQUAL_INT16(a->total, b->total);
    }
}

static void test_decoded_history_does_not_combine(void)
{
    uint8_t buf[PUCK_HISTORY_ENCODED_MAX];
    puck_history_record(&h, 0, 1, 1, 0);
    size_t n = puck_history_encode(&h, buf, sizeof(buf));
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_history_decode(buf, n, &h, COMBINE_MS));
    puck_history_record(&h, 0, 1, 2, 0);
    TEST_ASSERT_EQUAL_UINT16(2, puck_history_count(&h));
}

static void test_encode_rejects_small_buffer(void)
{
    puck_history_record(&h, 0, 1, 1, 0);
    uint8_t buf[PUCK_HISTORY_ENCODED_MAX];
    TEST_ASSERT_EQUAL_UINT32(0, puck_history_encode(&h, buf, 15));
    TEST_ASSERT_EQUAL_UINT32(16, puck_history_encode(&h, buf, 16));
}

static size_t make_two_entry_buffer(uint8_t *buf)
{
    memcpy(buf, V1_FIXTURE, sizeof(V1_FIXTURE));
    return sizeof(V1_FIXTURE);
}

static void test_decode_rejects_bad_sizes(void)
{
    uint8_t buf[32];
    size_t n = make_two_entry_buffer(buf);
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_SIZE, puck_history_decode(buf, 0, &h, COMBINE_MS));
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_SIZE, puck_history_decode(buf, 6, &h, COMBINE_MS));
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_SIZE, puck_history_decode(buf, 10, &h, COMBINE_MS));
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_SIZE, puck_history_decode(buf, n - 1, &h, COMBINE_MS));
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_SIZE, puck_history_decode(buf, n + 1, &h, COMBINE_MS));
}

static void test_decode_rejects_bad_magic(void)
{
    uint8_t buf[32];
    size_t n = make_two_entry_buffer(buf);
    buf[0] = 'X';
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_MAGIC, puck_history_decode(buf, n, &h, COMBINE_MS));
}

static void test_decode_rejects_bad_version(void)
{
    uint8_t buf[32];
    size_t n = make_two_entry_buffer(buf);
    buf[4] = 2;
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_VERSION, puck_history_decode(buf, n, &h, COMBINE_MS));
}

static void test_decode_rejects_bad_crc(void)
{
    uint8_t buf[32];
    size_t n = make_two_entry_buffer(buf);
    buf[8] ^= 0x01;
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_CRC, puck_history_decode(buf, n, &h, COMBINE_MS));
}

static void test_decode_rejects_invalid_contents(void)
{
    uint8_t buf[32];
    size_t n;

    n = make_two_entry_buffer(buf);
    buf[7] = PUCK_MAX_PLAYERS;
    fix_crc(buf, n);
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_INVALID, puck_history_decode(buf, n, &h, COMBINE_MS));

    n = make_two_entry_buffer(buf);
    buf[8] = 0;
    buf[9] = 0;
    fix_crc(buf, n);
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_INVALID, puck_history_decode(buf, n, &h, COMBINE_MS));

    n = make_two_entry_buffer(buf);
    buf[10] = 0xff;  // total -1
    buf[11] = 0xff;
    fix_crc(buf, n);
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_INVALID, puck_history_decode(buf, n, &h, COMBINE_MS));

    n = make_two_entry_buffer(buf);
    buf[10] = (uint8_t)((PUCK_LIFE_MAX + 1) & 0xff);
    buf[11] = (uint8_t)((PUCK_LIFE_MAX + 1) >> 8);
    fix_crc(buf, n);
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_INVALID, puck_history_decode(buf, n, &h, COMBINE_MS));
}

static void test_decode_accepts_total_bounds(void)
{
    uint8_t buf[32];
    size_t n = make_two_entry_buffer(buf);
    buf[10] = 0;  // total 0 == PUCK_LIFE_MIN
    buf[11] = 0;
    buf[15] = (uint8_t)(PUCK_LIFE_MAX & 0xff);
    buf[16] = (uint8_t)(PUCK_LIFE_MAX >> 8);
    fix_crc(buf, n);
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_history_decode(buf, n, &h, COMBINE_MS));
    assert_entry(0, 1, -2, PUCK_LIFE_MAX);
    assert_entry(1, 0, 5, 0);
}

static void test_decode_rejects_count_over_capacity(void)
{
    uint8_t buf[7 + 5 * (PUCK_HISTORY_CAPACITY + 1) + 4];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, V1_FIXTURE, 5);
    buf[5] = (uint8_t)(PUCK_HISTORY_CAPACITY + 1);
    for (int i = 0; i < PUCK_HISTORY_CAPACITY + 1; i++) {
        uint8_t *e = buf + 7 + 5 * i;
        e[1] = 1;
        e[3] = 1;
    }
    fix_crc(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_INVALID,
                      puck_history_decode(buf, sizeof(buf), &h, COMBINE_MS));
}

static void test_decode_error_leaves_history_untouched(void)
{
    puck_history_record(&h, 1, 4, 4, 0);
    uint8_t buf[32];
    size_t n = make_two_entry_buffer(buf);
    buf[7] = PUCK_MAX_PLAYERS;
    buf[13] = PUCK_MAX_PLAYERS;
    fix_crc(buf, n);
    TEST_ASSERT_EQUAL(PUCK_STATE_ERR_INVALID, puck_history_decode(buf, n, &h, 5));
    TEST_ASSERT_EQUAL_UINT16(1, puck_history_count(&h));
    TEST_ASSERT_EQUAL_UINT32(COMBINE_MS, h.combine_ms);
    assert_entry(0, 1, 4, 4);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_starts_empty);
    RUN_TEST(test_separate_players_make_separate_entries);
    RUN_TEST(test_burst_within_window_combines);
    RUN_TEST(test_no_combine_at_exactly_window);
    RUN_TEST(test_no_combine_after_long_pause);
    RUN_TEST(test_switching_player_breaks_combining);
    RUN_TEST(test_combine_to_zero_removes_entry);
    RUN_TEST(test_combine_to_zero_on_only_entry);
    RUN_TEST(test_zero_delta_is_ignored);
    RUN_TEST(test_combine_saturates_at_int16_limits);
    RUN_TEST(test_capacity_drops_oldest);
    RUN_TEST(test_combining_works_on_wrapped_ring);
    RUN_TEST(test_get_out_of_range);
    RUN_TEST(test_clear_empties_and_stops_combining);
    RUN_TEST(test_init_sets_combine_window);
    RUN_TEST(test_combining_handles_millisecond_wraparound);
    RUN_TEST(test_encode_matches_v1_fixture);
    RUN_TEST(test_decodes_v1_fixture);
    RUN_TEST(test_empty_history_round_trip);
    RUN_TEST(test_round_trip_full_wrapped_ring);
    RUN_TEST(test_decoded_history_does_not_combine);
    RUN_TEST(test_encode_rejects_small_buffer);
    RUN_TEST(test_decode_rejects_bad_sizes);
    RUN_TEST(test_decode_rejects_bad_magic);
    RUN_TEST(test_decode_rejects_bad_version);
    RUN_TEST(test_decode_rejects_bad_crc);
    RUN_TEST(test_decode_rejects_invalid_contents);
    RUN_TEST(test_decode_accepts_total_bounds);
    RUN_TEST(test_decode_rejects_count_over_capacity);
    RUN_TEST(test_decode_error_leaves_history_untouched);
    return UNITY_END();
}
