// Host implementations of the hardware-facing modules the UI code calls:
// battery_sense.h (settable values), storage.h (in-memory key/value store)
// and esp_reset_reason().
#include <stdlib.h>
#include <string.h>

#include "battery_sense.h"
#include "esp_app_desc.h"
#include "host_hw.h"
#include "storage.h"

static int s_percent = -1;
static uint16_t s_mv;
static bool s_charging;
static esp_reset_reason_t s_reset_reason = ESP_RST_SW;

void host_battery_set(int percent, uint16_t mv, bool charging)
{
    s_percent = percent;
    s_mv = mv;
    s_charging = charging;
}

void battery_sense_start(void) {}
int battery_sense_percent(void) { return s_percent; }
uint16_t battery_sense_mv(void) { return s_mv; }
bool battery_sense_charging(void) { return s_charging; }

void host_set_reset_reason(esp_reset_reason_t reason) { s_reset_reason = reason; }
esp_reset_reason_t esp_reset_reason(void) { return s_reset_reason; }

const esp_app_desc_t *esp_app_get_description(void)
{
    static const esp_app_desc_t desc = { .version = "0.0.2" };
    return &desc;
}

#define KEY_MAX  16
#define BLOB_MAX 1024
#define SLOTS    8

static struct {
    char key[KEY_MAX];
    uint8_t data[BLOB_MAX];
    size_t len;
    bool used;
} s_store[SLOTS];

void host_storage_clear(void) { memset(s_store, 0, sizeof(s_store)); }

void storage_init(void) {}

bool storage_load(const char *key, uint8_t *buf, size_t *len)
{
    for (int i = 0; i < SLOTS; i++) {
        if (s_store[i].used && strcmp(s_store[i].key, key) == 0) {
            if (s_store[i].len > *len) {
                return false;
            }
            memcpy(buf, s_store[i].data, s_store[i].len);
            *len = s_store[i].len;
            return true;
        }
    }
    return false;
}

bool storage_save(const char *key, const uint8_t *buf, size_t len)
{
    if (len > BLOB_MAX || strlen(key) >= KEY_MAX) {
        return false;
    }
    int slot = -1;
    for (int i = 0; i < SLOTS; i++) {
        if (s_store[i].used && strcmp(s_store[i].key, key) == 0) {
            slot = i;
            break;
        }
        if (!s_store[i].used && slot < 0) {
            slot = i;
        }
    }
    if (slot < 0) {
        return false;
    }
    strcpy(s_store[slot].key, key);
    memcpy(s_store[slot].data, buf, len);
    s_store[slot].len = len;
    s_store[slot].used = true;
    return true;
}

void storage_save_async(const char *key, const uint8_t *buf, size_t len)
{
    storage_save(key, buf, len);
}
