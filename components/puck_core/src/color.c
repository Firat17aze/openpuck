#include "puck_color.h"

#include <math.h>

const puck_color_preset_t PUCK_COLOR_PRESETS[PUCK_COLOR_PRESET_COUNT] = {
    { "Default", PUCK_COLOR_BLACK, PUCK_COLOR_WHITE },
    { "Blue",    0x3D91BCu,        PUCK_COLOR_WHITE },
    { "Green",   0x11A25Cu,        PUCK_COLOR_WHITE },
    { "Red",     0xB72A2Eu,        PUCK_COLOR_WHITE },
    { "Yellow",  0xD6AC48u,        PUCK_COLOR_BLACK },
    { "Blue text",   PUCK_COLOR_BLACK, 0x3D91BCu },
    { "Green text",  PUCK_COLOR_BLACK, 0x11A25Cu },
    { "Red text",    PUCK_COLOR_BLACK, 0xB72A2Eu },
    { "Yellow text", PUCK_COLOR_BLACK, 0xD6AC48u },
};

static uint32_t round_channel(double v)
{
    return (uint32_t)(v * 255.0 + 0.5);
}

uint32_t puck_color_from_hue_tint(uint16_t hue, uint8_t tint)
{
    if (tint > 100) {
        tint = 100;
    }
    double l = tint / 100.0;
    double c = 1.0 - fabs(2.0 * l - 1.0);  // chroma at saturation 1
    double sector = (hue % 360) / 60.0;
    double x = c * (1.0 - fabs(fmod(sector, 2.0) - 1.0));
    double m = l - c / 2.0;

    double r = 0.0, g = 0.0, b = 0.0;
    switch ((int)sector) {
    case 0:  r = c; g = x; break;
    case 1:  r = x; g = c; break;
    case 2:  g = c; b = x; break;
    case 3:  g = x; b = c; break;
    case 4:  r = x; b = c; break;
    default: r = c; b = x; break;
    }
    return (round_channel(r + m) << 16) | (round_channel(g + m) << 8) | round_channel(b + m);
}

void puck_color_to_hue_tint(uint32_t rgb, uint16_t *hue, uint8_t *tint)
{
    double r = ((rgb >> 16) & 0xFF) / 255.0;
    double g = ((rgb >> 8) & 0xFF) / 255.0;
    double b = (rgb & 0xFF) / 255.0;
    double max = fmax(r, fmax(g, b));
    double min = fmin(r, fmin(g, b));
    double delta = max - min;

    double h = 0.0;  // grays stay at 0
    if (delta > 0.0) {
        if (max == r) {
            h = fmod((g - b) / delta, 6.0);
        } else if (max == g) {
            h = (b - r) / delta + 2.0;
        } else {
            h = (r - g) / delta + 4.0;
        }
        h *= 60.0;
        if (h < 0.0) {
            h += 360.0;
        }
    }
    uint16_t degrees = (uint16_t)(h + 0.5);
    *hue = degrees % 360;  // 359.5 and up rounds to 360, which wraps
    *tint = (uint8_t)((max + min) / 2.0 * 100.0 + 0.5);
}

// WCAG 2 linearization of one 8-bit sRGB channel.
static double linearize(uint32_t channel)
{
    double c = channel / 255.0;
    return c <= 0.03928 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
}

uint32_t puck_color_readable_text(uint32_t background)
{
    double lum = 0.2126 * linearize((background >> 16) & 0xFF)
               + 0.7152 * linearize((background >> 8) & 0xFF)
               + 0.0722 * linearize(background & 0xFF);
    double white = 1.05 / (lum + 0.05);
    double black = (lum + 0.05) / 0.05;
    return black > white ? PUCK_COLOR_BLACK : PUCK_COLOR_WHITE;
}
