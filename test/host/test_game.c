#include "puck_game.h"
#include "unity.h"

static puck_game_t game;

void setUp(void)
{
    TEST_ASSERT_TRUE(puck_game_new(&game, 1, 20));
}

void tearDown(void) {}

static void test_new_game_sets_starting_life(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, game.player_count);
    TEST_ASSERT_EQUAL_INT16(20, game.starting_life);
    TEST_ASSERT_EQUAL_INT16(20, game.life[0]);
    TEST_ASSERT_EQUAL_INT16(0, game.life[1]);
}

static void test_new_two_player_game(void)
{
    TEST_ASSERT_TRUE(puck_game_new(&game, 2, 40));
    TEST_ASSERT_EQUAL_INT16(40, game.life[0]);
    TEST_ASSERT_EQUAL_INT16(40, game.life[1]);
}

static void test_new_game_rejects_bad_setup_and_leaves_state(void)
{
    puck_game_adjust(&game, 0, -3);
    TEST_ASSERT_FALSE(puck_game_new(&game, 0, 20));
    TEST_ASSERT_FALSE(puck_game_new(&game, PUCK_MAX_PLAYERS + 1, 20));
    TEST_ASSERT_FALSE(puck_game_new(&game, 1, -1));
    TEST_ASSERT_FALSE(puck_game_new(&game, 1, PUCK_STARTING_LIFE_MAX + 1));
    TEST_ASSERT_EQUAL_INT16(17, game.life[0]);
}

static void test_new_game_can_start_at_zero(void)
{
    TEST_ASSERT_TRUE(puck_game_new(&game, 1, 0));
    TEST_ASSERT_EQUAL_INT16(0, game.life[0]);
    TEST_ASSERT_TRUE(puck_game_adjust(&game, 0, 1));
    TEST_ASSERT_EQUAL_INT16(1, game.life[0]);
}

static void test_new_game_accepts_range_limits(void)
{
    TEST_ASSERT_TRUE(puck_game_new(&game, 1, PUCK_STARTING_LIFE_MIN));
    TEST_ASSERT_TRUE(puck_game_new(&game, PUCK_MAX_PLAYERS, PUCK_STARTING_LIFE_MAX));
}

static void test_adjust_up_and_down(void)
{
    TEST_ASSERT_TRUE(puck_game_adjust(&game, 0, 1));
    TEST_ASSERT_TRUE(puck_game_adjust(&game, 0, 5));
    TEST_ASSERT_TRUE(puck_game_adjust(&game, 0, -10));
    TEST_ASSERT_EQUAL_INT16(16, game.life[0]);
}

static void test_total_never_goes_negative(void)
{
    TEST_ASSERT_TRUE(puck_game_adjust(&game, 0, -25));
    TEST_ASSERT_EQUAL_INT16(0, game.life[0]);
    TEST_ASSERT_FALSE(puck_game_adjust(&game, 0, -1));
    TEST_ASSERT_EQUAL_INT16(0, game.life[0]);
}

static void test_adjust_clamps_at_limits(void)
{
    TEST_ASSERT_TRUE(puck_game_adjust(&game, 0, 32000));
    TEST_ASSERT_EQUAL_INT16(PUCK_LIFE_MAX, game.life[0]);
    TEST_ASSERT_FALSE(puck_game_adjust(&game, 0, 1));
    TEST_ASSERT_TRUE(puck_game_adjust(&game, 0, -32000));  // max -> clamped at minimum
    TEST_ASSERT_EQUAL_INT16(PUCK_LIFE_MIN, game.life[0]);
    TEST_ASSERT_FALSE(puck_game_adjust(&game, 0, -1));
}

static void test_adjust_zero_is_not_a_change(void)
{
    TEST_ASSERT_FALSE(puck_game_adjust(&game, 0, 0));
}

static void test_adjust_rejects_missing_player(void)
{
    TEST_ASSERT_FALSE(puck_game_adjust(&game, 1, 1));
    TEST_ASSERT_FALSE(puck_game_adjust(&game, 200, 1));
    TEST_ASSERT_EQUAL_INT16(0, game.life[1]);
}

static void test_restart_restores_starting_life(void)
{
    TEST_ASSERT_TRUE(puck_game_new(&game, 2, 40));
    puck_game_adjust(&game, 0, -7);
    puck_game_adjust(&game, 1, 3);
    puck_game_restart(&game);
    TEST_ASSERT_EQUAL_UINT8(2, game.player_count);
    TEST_ASSERT_EQUAL_INT16(40, game.life[0]);
    TEST_ASSERT_EQUAL_INT16(40, game.life[1]);
}

static void test_is_valid(void)
{
    TEST_ASSERT_TRUE(puck_game_is_valid(&game));

    puck_game_t bad = game;
    bad.player_count = 0;
    TEST_ASSERT_FALSE(puck_game_is_valid(&bad));

    bad = game;
    bad.starting_life = -1;
    TEST_ASSERT_FALSE(puck_game_is_valid(&bad));

    bad = game;
    bad.life[0] = PUCK_LIFE_MAX + 1;
    TEST_ASSERT_FALSE(puck_game_is_valid(&bad));

    bad = game;
    bad.life[0] = PUCK_LIFE_MIN - 1;
    TEST_ASSERT_FALSE(puck_game_is_valid(&bad));

    bad = game;
    bad.life[1] = 5;  // unused player slot must be zero
    TEST_ASSERT_FALSE(puck_game_is_valid(&bad));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_new_game_sets_starting_life);
    RUN_TEST(test_new_two_player_game);
    RUN_TEST(test_new_game_rejects_bad_setup_and_leaves_state);
    RUN_TEST(test_new_game_can_start_at_zero);
    RUN_TEST(test_new_game_accepts_range_limits);
    RUN_TEST(test_adjust_up_and_down);
    RUN_TEST(test_total_never_goes_negative);
    RUN_TEST(test_adjust_clamps_at_limits);
    RUN_TEST(test_adjust_zero_is_not_a_change);
    RUN_TEST(test_adjust_rejects_missing_player);
    RUN_TEST(test_restart_restores_starting_life);
    RUN_TEST(test_is_valid);
    return UNITY_END();
}
