#include "puck_save_policy.h"
#include "unity.h"

#define IDLE_MS      2000
#define MAX_DELAY_MS 10000

static puck_save_policy_t policy;

void setUp(void)
{
    puck_save_policy_init(&policy, IDLE_MS, MAX_DELAY_MS);
}

void tearDown(void) {}

static void test_nothing_to_save_initially(void)
{
    TEST_ASSERT_FALSE(puck_save_policy_pending(&policy));
    TEST_ASSERT_FALSE(puck_save_policy_due(&policy, 0));
    TEST_ASSERT_FALSE(puck_save_policy_due(&policy, 1000000));
}

static void test_not_due_before_idle(void)
{
    puck_save_policy_note_change(&policy, 1000);
    TEST_ASSERT_TRUE(puck_save_policy_pending(&policy));
    TEST_ASSERT_FALSE(puck_save_policy_due(&policy, 1000));
    TEST_ASSERT_FALSE(puck_save_policy_due(&policy, 1000 + IDLE_MS - 1));
}

static void test_due_after_idle(void)
{
    puck_save_policy_note_change(&policy, 1000);
    TEST_ASSERT_TRUE(puck_save_policy_due(&policy, 1000 + IDLE_MS));
}

static void test_each_change_restarts_idle_wait(void)
{
    puck_save_policy_note_change(&policy, 0);
    puck_save_policy_note_change(&policy, 1500);
    TEST_ASSERT_FALSE(puck_save_policy_due(&policy, 2500));
    TEST_ASSERT_TRUE(puck_save_policy_due(&policy, 1500 + IDLE_MS));
}

static void test_continuous_tapping_still_saves_after_max_delay(void)
{
    uint32_t t = 0;
    for (; t < MAX_DELAY_MS; t += 500) {
        puck_save_policy_note_change(&policy, t);
        TEST_ASSERT_FALSE(puck_save_policy_due(&policy, t));
    }
    puck_save_policy_note_change(&policy, t);
    TEST_ASSERT_TRUE(puck_save_policy_due(&policy, t));
}

static void test_mark_saved_clears_pending(void)
{
    puck_save_policy_note_change(&policy, 0);
    puck_save_policy_mark_saved(&policy);
    TEST_ASSERT_FALSE(puck_save_policy_pending(&policy));
    TEST_ASSERT_FALSE(puck_save_policy_due(&policy, IDLE_MS * 10));
}

static void test_new_run_after_save_measures_from_new_first_change(void)
{
    puck_save_policy_note_change(&policy, 0);
    puck_save_policy_mark_saved(&policy);
    puck_save_policy_note_change(&policy, 50000);
    TEST_ASSERT_FALSE(puck_save_policy_due(&policy, 50000 + IDLE_MS - 1));
    TEST_ASSERT_TRUE(puck_save_policy_due(&policy, 50000 + IDLE_MS));
}

static void test_handles_millisecond_wraparound(void)
{
    const uint32_t near_wrap = UINT32_MAX - 500;
    puck_save_policy_note_change(&policy, near_wrap);
    TEST_ASSERT_FALSE(puck_save_policy_due(&policy, near_wrap + 1000));  // wrapped, 1 s later
    TEST_ASSERT_TRUE(puck_save_policy_due(&policy, near_wrap + IDLE_MS));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_nothing_to_save_initially);
    RUN_TEST(test_not_due_before_idle);
    RUN_TEST(test_due_after_idle);
    RUN_TEST(test_each_change_restarts_idle_wait);
    RUN_TEST(test_continuous_tapping_still_saves_after_max_delay);
    RUN_TEST(test_mark_saved_clears_pending);
    RUN_TEST(test_new_run_after_save_measures_from_new_first_change);
    RUN_TEST(test_handles_millisecond_wraparound);
    return UNITY_END();
}
