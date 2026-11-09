#include "storage.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "storage";

#define NAMESPACE      "openpuck"
#define MAX_SLOTS      4     // game, history, timer, settings
#define MAX_BLOB       600   // fits PUCK_HISTORY_ENCODED_MAX
#define KEY_MAX        16    // NVS keys are at most 15 chars, plus NUL
#define TASK_PRIORITY  2     // below the LVGL task: saving never delays drawing
#define TASK_STACK     3072

typedef struct {
    char key[KEY_MAX];       // empty = slot unused
    bool dirty;
    size_t len;
    uint8_t data[MAX_BLOB];
} slot_t;

// Two locks so queuing a save never waits for a flash write: s_slots_lock is
// held only to copy a few hundred bytes; s_nvs_lock serializes NVS access.
static SemaphoreHandle_t s_slots_lock;
static SemaphoreHandle_t s_nvs_lock;
static TaskHandle_t s_task;
static slot_t s_slots[MAX_SLOTS];  // guarded by s_slots_lock

bool storage_save(const char *key, const uint8_t *buf, size_t len)
{
    nvs_handle_t nvs;
    xSemaphoreTake(s_nvs_lock, portMAX_DELAY);
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        err = nvs_set_blob(nvs, key, buf, len);
        if (err == ESP_OK) {
            err = nvs_commit(nvs);
        }
        nvs_close(nvs);
    }
    xSemaphoreGive(s_nvs_lock);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "save '%s' failed: %s", key, esp_err_to_name(err));
        return false;
    }
    return true;
}

bool storage_load(const char *key, uint8_t *buf, size_t *len)
{
    nvs_handle_t nvs;
    xSemaphoreTake(s_nvs_lock, portMAX_DELAY);
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        err = nvs_get_blob(nvs, key, buf, len);
        nvs_close(nvs);
    }
    xSemaphoreGive(s_nvs_lock);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "load '%s' failed: %s", key, esp_err_to_name(err));
    }
    return err == ESP_OK;
}

void storage_save_async(const char *key, const uint8_t *buf, size_t len)
{
    if (len > MAX_BLOB || strlen(key) >= KEY_MAX) {
        ESP_LOGE(TAG, "'%s' dropped: key or blob too large (%u bytes)", key, (unsigned)len);
        return;
    }
    xSemaphoreTake(s_slots_lock, portMAX_DELAY);
    slot_t *slot = NULL;
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (strcmp(s_slots[i].key, key) == 0) {
            slot = &s_slots[i];
            break;
        }
        if (!slot && s_slots[i].key[0] == '\0') {
            slot = &s_slots[i];  // first free slot, unless the key turns up later
        }
    }
    if (slot) {
        strcpy(slot->key, key);
        memcpy(slot->data, buf, len);
        slot->len = len;
        slot->dirty = true;
    }
    xSemaphoreGive(s_slots_lock);
    if (!slot) {
        ESP_LOGE(TAG, "'%s' dropped: no free slot", key);
        return;
    }
    xTaskNotifyGive(s_task);
}

static void storage_task(void *arg)
{
    // Static: too big for the task stack.
    static slot_t work;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        for (int i = 0; i < MAX_SLOTS; i++) {
            xSemaphoreTake(s_slots_lock, portMAX_DELAY);
            const bool dirty = s_slots[i].dirty;
            if (dirty) {
                work = s_slots[i];
                s_slots[i].dirty = false;
            }
            xSemaphoreGive(s_slots_lock);
            if (dirty) {
                storage_save(work.key, work.data, work.len);
            }
        }
    }
}

void storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS unusable (%s), erasing", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    s_slots_lock = xSemaphoreCreateMutex();
    s_nvs_lock = xSemaphoreCreateMutex();
    xTaskCreate(storage_task, "storage", TASK_STACK, NULL, TASK_PRIORITY, &s_task);
}
