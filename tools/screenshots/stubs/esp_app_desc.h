// Host stand-in for esp_app_desc.h: a fixed version so screenshots are
// deterministic.
#pragma once

typedef struct {
    char version[32];
} esp_app_desc_t;

const esp_app_desc_t *esp_app_get_description(void);
