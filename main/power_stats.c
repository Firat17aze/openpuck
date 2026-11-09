#include "power_stats.h"

#if CONFIG_OPENPUCK_POWER_STATS

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_pm.h"
#include "ui_internal.h"

static const char *TAG = "power_stats";

#define UPDATE_MS    5000
#define MODE_COUNT   4          // SLEEP, APB_MIN, APB_MAX, CPU_MAX
#define LABEL_BOTTOM (-8)

static lv_obj_t *s_label;

// esp_pm_dump_locks prints the profiling data (time in each power mode) only
// to a stream, so capture it and read the "Mode stats" table:
//   SLEEP     40M         12345678    45%
typedef struct {
    int64_t boot_us;
    int64_t sleep_us;
    int64_t awake_us;       // largest non-sleep mode
    unsigned awake_mhz;
} stats_t;

static bool read_stats(stats_t *out)
{
    char *text = NULL;
    size_t size = 0;
    FILE *f = open_memstream(&text, &size);
    if (!f) {
        return false;
    }
    esp_pm_dump_locks(f);
    fclose(f);

    memset(out, 0, sizeof(*out));
    bool have_sleep = false;
    char *save = NULL;
    for (char *line = strtok_r(text, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        char mode[16];
        unsigned mhz;
        long long us;
        if (sscanf(line, "Time since boot up: %lld", &us) == 1) {
            out->boot_us = us;
        // The frequency is printed padded ("40 M", "160M"); " M" matches both.
        } else if (sscanf(line, "%15s %u M %lld", mode, &mhz, &us) == 3) {
            if (strcmp(mode, "SLEEP") == 0) {
                out->sleep_us = us;
                have_sleep = true;
            } else if (!strncmp(mode, "APB_", 4) || !strcmp(mode, "CPU_MAX")) {
                if (us > out->awake_us) {
                    out->awake_us = us;
                    out->awake_mhz = mhz;
                }
            }
        }
    }
    free(text);
    return have_sleep && out->boot_us > 0;
}

static void update(lv_timer_t *timer)
{
    stats_t st;
    if (!read_stats(&st)) {
        ESP_LOGW(TAG, "could not read power stats");
        return;
    }
    const int sleep_pct = (int)(st.sleep_us * 100 / st.boot_us);
    lv_label_set_text_fmt(s_label, "sleep %d%%  %uM", sleep_pct, st.awake_mhz);
    ESP_LOGI(TAG, "light sleep %d%% of %lld ms since boot", sleep_pct, st.boot_us / 1000);
}

void power_stats_start(void)
{
    lvgl_port_lock(0);
    s_label = ui_add_label(lv_layer_top(), &lv_font_montserrat_14, "sleep --%");
    lv_obj_set_style_text_color(s_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(s_label, LV_ALIGN_BOTTOM_MID, 0, LABEL_BOTTOM);
    lv_timer_create(update, UPDATE_MS, NULL);
    lvgl_port_unlock();
}

#endif  // CONFIG_OPENPUCK_POWER_STATS
