// Player colors: presets and the hue/tint model used by the color picker.
// Colors are 0xRRGGBB.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PUCK_COLOR_BLACK 0x000000u
#define PUCK_COLOR_WHITE 0xFFFFFFu

typedef struct {
    const char *name;
    uint32_t background;
    uint32_t text;
} puck_color_preset_t;

// Default (black/white), then the four base colors with their text color:
// Blue 0x3D91BC/white, Green 0x11A25C/white, Red 0xB72A2E/white,
// Yellow 0xD6AC48/black. White is a design choice on Blue and Green: it
// meets the 3:1 large-text contrast (3.5:1, 3.3:1) even though black scores
// higher there; Yellow needs black (white is only 2.1:1). Then four presets
// on a black background with the base color as the text: Blue text, Green
// text, Red text, Yellow text.
#define PUCK_COLOR_PRESET_COUNT 9
extern const puck_color_preset_t PUCK_COLOR_PRESETS[PUCK_COLOR_PRESET_COUNT];

// Picker model: HSL with full saturation. hue 0-359 degrees (values >= 360
// wrap); tint 0-100 is the lightness: 0 black, 50 the pure hue, 100 white
// (values > 100 are treated as 100). Channels round to nearest.
uint32_t puck_color_from_hue_tint(uint16_t hue, uint8_t tint);

// Inverse, to start the picker from a stored color: hue (0-359) and
// lightness (0-100, rounded) of rgb in HSL. Grays have hue 0. Saturation is
// dropped, so a color that isn't fully saturated won't round-trip exactly.
void puck_color_to_hue_tint(uint32_t rgb, uint16_t *hue, uint8_t *tint);

// PUCK_COLOR_BLACK or PUCK_COLOR_WHITE, whichever has the higher contrast
// ratio with background (WCAG 2 relative luminance). Ties go to white.
uint32_t puck_color_readable_text(uint32_t background);

#ifdef __cplusplus
}
#endif
