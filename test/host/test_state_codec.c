#include <string.h>

#include "puck_state_codec.h"
#include "unity.h"

// Version 1 encoding of {1 player, starting life 20, life 17}. The CRC was
// computed independently with Python's zlib.crc32. Released formats must keep
// decoding: never edit this fixture, add new ones for new versions.
static const uint8_t V1_FIXTURE[PUCK_STATE_SIZE] = {
    0x4f, 0x50, 0x43, 0x4b, 0x01, 0x01, 0x14, 0x00,
    0x11, 0x00, 0x00, 0x00, 0xee, 0x69, 0xe9, 0x72,
};

static puck_game_t game;
static uint8_t buf[PUCK_STATE_SIZE];

void setUp(void)
{
    puck_game_new(&game, 1, 20);
    puck_game_adjust(&game, 0, -3);
    TEST_ASSERT_EQUAL(PUCK_STATE_SIZE, puck_state_encode(&game, buf, sizeof(buf)));
}

void tearDown(void) {}

static void test_crc32_matches_standard_check_value(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926, puck_crc32((const uint8_t *)"123456789", 9));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, puck_crc32(NULL, 0));
}

static void test_encode_matches_v1_fixture(void)
{
    TEST_ASSERT_EQUAL_UINT8_ARRAY(V1_FIXTURE, buf, PUCK_STATE_SIZE);
}

static void test_v1_fixture_decodes(void)
{
    puck_game_t out;
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_state_decode(V1_FIXTURE, sizeof(V1_FIXTURE), &out));
    TEST_ASSERT_EQUAL_UINT8(1, out.player_count);
    TEST_ASSERT_EQUAL_INT16(20, out.starting_life);
    TEST_ASSERT_EQUAL_INT16(17, out.life[0]);
}

static void test_round_trip_two_players_with_extremes(void)
{
    puck_game_new(&game, 2, PUCK_STARTING_LIFE_MAX);
    puck_game_adjust(&game, 0, -32000);
    puck_game_adjust(&game, 1, 32000);
    TEST_ASSERT_EQUAL(PUCK_STATE_SIZE, puck_state_encode(&game, buf, sizeof(buf)));
    puck_game_t out;
    TEST_ASSERT_EQUAL(PUCK_STATE_OK, puck_state_decode(buf, sizeof(buf), &out));
    TEST_ASSERT_EQUAL_MEMORY(&game, &out, sizeof(game));
}

static void test_encode_rejects_small_buffer(void)
{
    uint8_t small[PUCK_STATE_SIZE - 1];
    TEST_ASSERT_EQUAL(0, puck_state_encode(&game, small, sizeof(small)));
}

static void expect_rejected(const uint8_t *data, size_t len, puck_state_result_t expected)
{
    puck_game_t out;
    memset(&out, 0xAB, sizeof(out));
    puck_game_t untouched = out;
    TEST_ASSERT_EQUAL(expected, puck_state_decode(data, len, &out));
    TEST_ASSERT_EQUAL_MEMORY(&untouched, &out, sizeof(out));
}

static void test_rejects_wrong_size(void)
{
    expect_rejected(buf, PUCK_STATE_SIZE - 1, PUCK_STATE_ERR_SIZE);
    expect_rejected(buf, 0, PUCK_STATE_ERR_SIZE);
}

static void test_rejects_bad_magic(void)
{
    buf[0] = 'X';
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_MAGIC);
}

static void test_rejects_unknown_version(void)
{
    buf[4] = PUCK_STATE_VERSION + 1;
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_VERSION);
}

static void test_rejects_corruption(void)
{
    for (size_t i = 5; i < PUCK_STATE_SIZE; i++) {
        uint8_t copy[PUCK_STATE_SIZE];
        memcpy(copy, buf, sizeof(copy));
        copy[i] ^= 0x01;
        expect_rejected(copy, sizeof(copy), PUCK_STATE_ERR_CRC);
    }
}

static void test_rejects_valid_crc_with_invalid_contents(void)
{
    puck_game_t bad = game;
    bad.player_count = 0;
    TEST_ASSERT_EQUAL(PUCK_STATE_SIZE, puck_state_encode(&bad, buf, sizeof(buf)));
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_INVALID);
}

static void test_rejects_negative_total(void)
{
    puck_game_t bad = game;
    bad.life[0] = -5;
    TEST_ASSERT_EQUAL(PUCK_STATE_SIZE, puck_state_encode(&bad, buf, sizeof(buf)));
    expect_rejected(buf, sizeof(buf), PUCK_STATE_ERR_INVALID);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc32_matches_standard_check_value);
    RUN_TEST(test_encode_matches_v1_fixture);
    RUN_TEST(test_v1_fixture_decodes);
    RUN_TEST(test_round_trip_two_players_with_extremes);
    RUN_TEST(test_encode_rejects_small_buffer);
    RUN_TEST(test_rejects_wrong_size);
    RUN_TEST(test_rejects_bad_magic);
    RUN_TEST(test_rejects_unknown_version);
    RUN_TEST(test_rejects_corruption);
    RUN_TEST(test_rejects_valid_crc_with_invalid_contents);
    RUN_TEST(test_rejects_negative_total);
    return UNITY_END();
}
