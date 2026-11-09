// Host stand-in for esp_system.h: only the reset reason, which the scenes set
// (a real power-on shows the splash).
#pragma once

typedef enum {
    ESP_RST_UNKNOWN,
    ESP_RST_POWERON,
    ESP_RST_SW,
} esp_reset_reason_t;

esp_reset_reason_t esp_reset_reason(void);
