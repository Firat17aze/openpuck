#include "logo.h"

// Geometry matches tools/gen_logo.py, which draws on the 240x240 screen grid:
// ring center-line radius 80, stroke 32 (outer diameter 192), round caps;
// gap centered at -45 deg (top right) with the caps 32 deg either side of it;
// bead radius 16 sitting in the gap on the ring's center line.
// Angles: 0 = 3 o'clock, clockwise, as in both SVG and LVGL.
#define RING_START_DEG 347   // -45 + 32
#define RING_END_DEG   283   // -45 - 32 + 360
#define SIN_45_X10000  7071

lv_obj_t *logo_create(lv_obj_t *parent, int32_t diameter)
{
    const int32_t stroke = diameter / 6;                 // 32 / 192
    const int32_t radius = (diameter - stroke) / 2;      // ring center line
    const int32_t half = diameter / 2;

    lv_obj_t *logo = lv_obj_create(parent);
    lv_obj_remove_style_all(logo);
    lv_obj_set_size(logo, diameter, diameter);
    lv_obj_set_clickable(logo, false);
    lv_obj_set_scrollable(logo, false);

    lv_obj_t *ring = lv_arc_create(logo);
    lv_obj_remove_style_all(ring);
    lv_obj_set_size(ring, diameter, diameter);
    lv_obj_center(ring);
    lv_obj_set_clickable(ring, false);
    lv_arc_set_bg_angles(ring, RING_START_DEG, RING_END_DEG);
    lv_obj_set_style_arc_width(ring, stroke, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ring, LOGO_COLOR_RING, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ring, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(ring, true, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ring, LV_OPA_TRANSP, LV_PART_INDICATOR);

    // Bead at -45 deg: right of and above the center.
    const int32_t offset = radius * SIN_45_X10000 / 10000;
    lv_obj_t *bead = lv_obj_create(logo);
    lv_obj_remove_style_all(bead);
    lv_obj_set_size(bead, stroke, stroke);
    lv_obj_set_pos(bead, half + offset - stroke / 2, half - offset - stroke / 2);
    lv_obj_set_style_radius(bead, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(bead, LOGO_COLOR_BEAD, 0);
    lv_obj_set_style_bg_opa(bead, LV_OPA_COVER, 0);
    lv_obj_set_clickable(bead, false);

    return logo;
}
