#include <string.h>

#include "puck_color.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

static void test_preset_table(void)
{
    TEST_ASSERT_EQUAL(9, PUCK_COLOR_PRESET_COUNT);
    const char *names[] = { "Default", "Blue", "Green", "Red", "Yellow",
                         "Blue text", "Green text", "Red text", "Yellow text" };
    uint32_t backgrounds[] = { 0x000000, 0x3D91BC, 0x11A25C, 0xB72A2E, 0xD6AC48,
                             0x000000, 0x000000, 0x000000, 0x000000 };
    uint32_t texts[] = { 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0x000000,
                         0x3D91BC, 0x11A25C, 0xB72A2E, 0xD6AC48 };
    for (int i = 0; i < PUCK_COLOR_PRESET_COUNT; i++) {
        TEST_ASSERT_EQUAL_STRING(names[i], PUCK_COLOR_PRESETS[i].name);
        TEST_ASSERT_EQUAL_HEX32(backgrounds[i], PUCK_COLOR_PRESETS[i].background);
        TEST_ASSERT_EQUAL_HEX32(texts[i], PUCK_COLOR_PRESETS[i].text);
    }
}

// The header specifies Blue and Green with white text, but strict WCAG 2
// contrast (computed independently in Python) favors black on both. The
// designed table wins for those two; the other colored-background presets
// agree with the formula. The black-background text presets are exempt: their
// text is deliberately colored.
static void test_preset_text_agrees_with_readable_text_where_wcag_does(void)
{
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_PRESETS[0].text,
                            puck_color_readable_text(PUCK_COLOR_PRESETS[0].background));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_PRESETS[3].text,
                            puck_color_readable_text(PUCK_COLOR_PRESETS[3].background));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_PRESETS[4].text,
                            puck_color_readable_text(PUCK_COLOR_PRESETS[4].background));
}

static void test_from_hue_tint_endpoints(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x000000, puck_color_from_hue_tint(0, 0));
    TEST_ASSERT_EQUAL_HEX32(0x000000, puck_color_from_hue_tint(200, 0));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF, puck_color_from_hue_tint(0, 100));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF, puck_color_from_hue_tint(200, 100));
}

static void test_from_hue_tint_primaries_and_secondaries(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xFF0000, puck_color_from_hue_tint(0, 50));
    TEST_ASSERT_EQUAL_HEX32(0xFFFF00, puck_color_from_hue_tint(60, 50));
    TEST_ASSERT_EQUAL_HEX32(0x00FF00, puck_color_from_hue_tint(120, 50));
    TEST_ASSERT_EQUAL_HEX32(0x00FFFF, puck_color_from_hue_tint(180, 50));
    TEST_ASSERT_EQUAL_HEX32(0x0000FF, puck_color_from_hue_tint(240, 50));
    TEST_ASSERT_EQUAL_HEX32(0xFF00FF, puck_color_from_hue_tint(300, 50));
}

// Expected values from Python colorsys.hls_to_rgb(h/360, t/100, 1).
static void test_from_hue_tint_mid_values(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x006699, puck_color_from_hue_tint(200, 30));
    TEST_ASSERT_EQUAL_HEX32(0xFF80FF, puck_color_from_hue_tint(300, 75));
    TEST_ASSERT_EQUAL_HEX32(0xFF9933, puck_color_from_hue_tint(30, 60));
    TEST_ASSERT_EQUAL_HEX32(0x1A3300, puck_color_from_hue_tint(90, 10));
}

static void test_from_hue_tint_wraps_and_clamps(void)
{
    TEST_ASSERT_EQUAL_HEX32(puck_color_from_hue_tint(0, 50), puck_color_from_hue_tint(360, 50));
    TEST_ASSERT_EQUAL_HEX32(puck_color_from_hue_tint(30, 60), puck_color_from_hue_tint(750, 60));
    TEST_ASSERT_EQUAL_HEX32(puck_color_from_hue_tint(90, 100), puck_color_from_hue_tint(90, 101));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF, puck_color_from_hue_tint(90, 255));
}

static void expect_hue_tint(uint32_t rgb, uint16_t want_hue, uint8_t want_tint)
{
    uint16_t hue = 999;
    uint8_t tint = 255;
    puck_color_to_hue_tint(rgb, &hue, &tint);
    TEST_ASSERT_EQUAL_UINT16(want_hue, hue);
    TEST_ASSERT_EQUAL_UINT8(want_tint, tint);
}

static void test_to_hue_tint_black_white_gray(void)
{
    expect_hue_tint(0x000000, 0, 0);
    expect_hue_tint(0xFFFFFF, 0, 100);
    expect_hue_tint(0x808080, 0, 50);  // 128/255 = 50.2%
}

static void test_to_hue_tint_primaries(void)
{
    expect_hue_tint(0xFF0000, 0, 50);
    expect_hue_tint(0xFFFF00, 60, 50);
    expect_hue_tint(0x00FF00, 120, 50);
    expect_hue_tint(0x00FFFF, 180, 50);
    expect_hue_tint(0x0000FF, 240, 50);
    expect_hue_tint(0xFF00FF, 300, 50);
}

// Red-dominant with blue > green gives a negative hue that wraps up, and a
// hue just under 360 rounds up to 360, which wraps to 0.
static void test_to_hue_tint_wrapping_hues(void)
{
    expect_hue_tint(0xFF0080, 330, 50);
    expect_hue_tint(0xFF0001, 0, 50);  // 359.76 degrees
}

static void test_to_hue_tint_round_trips(void)
{
    uint16_t hues[] = { 0, 15, 60, 97, 120, 200, 240, 301, 359 };
    uint8_t tints[] = { 0, 10, 25, 50, 64, 80, 100 };
    for (size_t i = 0; i < sizeof(hues) / sizeof(hues[0]); i++) {
        for (size_t j = 0; j < sizeof(tints); j++) {
            uint32_t rgb = puck_color_from_hue_tint(hues[i], tints[j]);
            uint16_t hue = 0;
            uint8_t tint = 0;
            puck_color_to_hue_tint(rgb, &hue, &tint);
            // At the extremes (black/white) the hue is lost; skip it there.
            if (tints[j] != 0 && tints[j] != 100) {
                int diff = (int)hue - (int)hues[i];
                if (diff > 180) {
                    diff -= 360;
                }
                if (diff < -180) {
                    diff += 360;
                }
                TEST_ASSERT_TRUE_MESSAGE(diff >= -1 && diff <= 1, "hue off by more than 1");
            }
            TEST_ASSERT_INT_WITHIN(1, tints[j], tint);
        }
    }
}

static void test_readable_text_extremes(void)
{
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_WHITE, puck_color_readable_text(PUCK_COLOR_BLACK));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_BLACK, puck_color_readable_text(PUCK_COLOR_WHITE));
}

// Expected values from Python WCAG 2 contrast ratios. Blue and Green come out
// black here even though the preset table pairs them with white.
static void test_readable_text_presets_and_primaries(void)
{
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_BLACK, puck_color_readable_text(0x3D91BC));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_BLACK, puck_color_readable_text(0x11A25C));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_WHITE, puck_color_readable_text(0xB72A2E));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_BLACK, puck_color_readable_text(0xD6AC48));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_WHITE, puck_color_readable_text(0x0000FF));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_BLACK, puck_color_readable_text(0x00FF00));
}

static void test_readable_text_grays(void)
{
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_BLACK, puck_color_readable_text(0x808080));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_BLACK, puck_color_readable_text(0x767676));
    TEST_ASSERT_EQUAL_HEX32(PUCK_COLOR_WHITE, puck_color_readable_text(0x595959));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_preset_table);
    RUN_TEST(test_preset_text_agrees_with_readable_text_where_wcag_does);
    RUN_TEST(test_from_hue_tint_endpoints);
    RUN_TEST(test_from_hue_tint_primaries_and_secondaries);
    RUN_TEST(test_from_hue_tint_mid_values);
    RUN_TEST(test_from_hue_tint_wraps_and_clamps);
    RUN_TEST(test_to_hue_tint_black_white_gray);
    RUN_TEST(test_to_hue_tint_primaries);
    RUN_TEST(test_to_hue_tint_wrapping_hues);
    RUN_TEST(test_to_hue_tint_round_trips);
    RUN_TEST(test_readable_text_extremes);
    RUN_TEST(test_readable_text_presets_and_primaries);
    RUN_TEST(test_readable_text_grays);
    return UNITY_END();
}
