#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "puck_number.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

static void test_every_value_0_to_99(void)
{
    const int32_t ambiguous[] = { 6, 9, 66, 68, 86, 89, 98, 99 };
    for (int32_t v = 0; v <= 99; v++) {
        bool expected = false;
        for (size_t i = 0; i < sizeof(ambiguous) / sizeof(ambiguous[0]); i++) {
            expected = expected || ambiguous[i] == v;
        }
        TEST_ASSERT_EQUAL_MESSAGE(expected, puck_number_rotation_ambiguous(v), "value");
    }
}

static void test_self_symmetric_values_are_not_ambiguous(void)
{
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(0));
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(8));
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(88));
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(69));   // rotates to itself
}

static void test_leading_zero_after_rotation_is_not_ambiguous(void)
{
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(60));   // "09"
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(90));   // "06"
}

static void test_negative_values(void)
{
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(-1));
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(-6));
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(INT32_MIN));
}

static void test_larger_values(void)
{
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(609));  // rotates to itself
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(106));  // 1 can't rotate
    TEST_ASSERT_TRUE(puck_number_rotation_ambiguous(669));
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(986));  // rotates to itself
    TEST_ASSERT_TRUE(puck_number_rotation_ambiguous(966));
    TEST_ASSERT_FALSE(puck_number_rotation_ambiguous(2147483647));  // contains 2
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_every_value_0_to_99);
    RUN_TEST(test_self_symmetric_values_are_not_ambiguous);
    RUN_TEST(test_leading_zero_after_rotation_is_not_ambiguous);
    RUN_TEST(test_negative_values);
    RUN_TEST(test_larger_values);
    return UNITY_END();
}
