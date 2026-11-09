/* LVGL configuration for the host screenshot renderer. It mirrors what the
 * firmware enables (sdkconfig.defaults plus the esp_lvgl_port defaults): RGB565,
 * Montserrat 14/20/28/48, the default theme and the widgets the UI uses.
 * Everything not listed keeps LVGL's own default (lv_conf_internal.h). */
#ifndef LV_CONF_H
#define LV_CONF_H

/* 16 bit color (RGB565), as the firmware */
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_RGB565

#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (2 * 1024 * 1024)

#define LV_USE_OS LV_OS_NONE
#define LV_USE_LOG 0

#define LV_USE_DRAW_SW 1
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_COMPLEX 1

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_USE_THEME_DEFAULT 1
#define LV_USE_FLEX 1

#define LV_USE_ARC 1
#define LV_USE_ARCLABEL 1
#define LV_USE_BUTTON 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1

#endif /* LV_CONF_H */
