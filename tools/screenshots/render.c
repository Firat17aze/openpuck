#include "render.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host_hw.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO_FAILURE_MSG 1
#include "stb_image_write.h"

#define START_TIME_MS 100000u
#define STEP_MS       10u
#define BEZEL_WIDTH   3     // dark ring around the circle
#define RADIUS        (SCREEN_SIZE / 2)

static uint32_t s_now;
static uint16_t s_frame[SCREEN_SIZE * SCREEN_SIZE];
static lv_display_t *s_disp;
static lv_indev_t *s_indev;
static bool s_pressed;
static lv_point_t s_point;
static char s_out_dir[512] = SCREENSHOT_OUT_DEFAULT;
static bool s_bezel = true;

static uint32_t tick_cb(void)
{
    return s_now;
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const int32_t w = lv_area_get_width(area);
    for (int32_t y = area->y1; y <= area->y2; y++) {
        memcpy(&s_frame[y * SCREEN_SIZE + area->x1], px_map + (y - area->y1) * w * 2, (size_t)w * 2);
    }
    lv_display_flush_ready(disp);
}

static void indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point = s_point;
    data->state = s_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void render_begin(void)
{
    s_now = START_TIME_MS;
    s_pressed = false;
    host_storage_clear();
    host_battery_set(-1, 0, false);

    lv_init();
    lv_tick_set_cb(tick_cb);

    static uint16_t buf[SCREEN_SIZE * SCREEN_SIZE];
    s_disp = lv_display_create(SCREEN_SIZE, SCREEN_SIZE);
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(s_disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(s_disp, flush_cb);

    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, indev_read_cb);
    lv_indev_set_display(s_indev, s_disp);
}

lv_indev_t *render_indev(void)
{
    return s_indev;
}

uint32_t render_now(void)
{
    return s_now;
}

void render_advance(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += STEP_MS) {
        s_now += STEP_MS;
        lv_timer_handler();
    }
}

void render_jump(uint32_t ms)
{
    s_now += ms;
    lv_timer_handler();
}

void touch_down(int32_t x, int32_t y)
{
    s_point.x = x;
    s_point.y = y;
    s_pressed = true;
}

void touch_move(int32_t x, int32_t y)
{
    s_point.x = x;
    s_point.y = y;
}

void touch_up(void)
{
    s_pressed = false;
}

void touch_hold(int32_t x, int32_t y, uint32_t ms, uint32_t settle_ms)
{
    touch_down(x, y);
    render_advance(ms);
    touch_up();
    render_advance(settle_ms);
}

void touch_tap(int32_t x, int32_t y, uint32_t settle_ms)
{
    touch_hold(x, y, 60, settle_ms);
}

void render_set_output(const char *dir, bool bezel)
{
    snprintf(s_out_dir, sizeof(s_out_dir), "%s", dir);
    s_bezel = bezel;
}

static double clamp01(double v)
{
    return v < 0 ? 0 : v > 1 ? 1 : v;
}

bool render_save(const char *name)
{
    lv_refr_now(s_disp);

    const int pad = s_bezel ? BEZEL_WIDTH : 0;
    const int size = SCREEN_SIZE + 2 * pad;
    uint8_t *rgba = calloc((size_t)size * size, 4);
    if (!rgba) {
        return false;
    }
    const double center = size / 2.0;
    const double ring_outer = RADIUS + pad;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const double d = hypot(x + 0.5 - center, y + 0.5 - center);
            const double inside = clamp01(RADIUS + 0.5 - d);     // coverage of the screen circle
            const double outer = clamp01(ring_outer + 0.5 - d);  // coverage of circle plus bezel
            uint8_t *px = &rgba[((size_t)y * size + x) * 4];
            // True colors: the panel's hardware inversion is not applied here.
            double r = 0x2A, g = 0x2A, b = 0x2E;   // bezel
            if (inside > 0) {
                const uint16_t c = s_frame[(y - pad) * SCREEN_SIZE + (x - pad)];
                const double sr = ((c >> 11) & 0x1F) * 255 / 31;
                const double sg = ((c >> 5) & 0x3F) * 255 / 63;
                const double sb = (c & 0x1F) * 255 / 31;
                r = sr * inside + r * (1 - inside);
                g = sg * inside + g * (1 - inside);
                b = sb * inside + b * (1 - inside);
            }
            px[0] = (uint8_t)lround(r);
            px[1] = (uint8_t)lround(g);
            px[2] = (uint8_t)lround(b);
            px[3] = (uint8_t)lround((s_bezel ? outer : inside) * 255.0);
        }
    }
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.png", s_out_dir, name);
    const int ok = stbi_write_png(path, size, size, 4, rgba, size * 4);
    free(rgba);
    if (!ok) {
        fprintf(stderr, "cannot write %s\n", path);
        return false;
    }
    printf("wrote %s\n", path);
    return true;
}
