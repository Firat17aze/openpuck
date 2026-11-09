// Keyed blob storage in NVS (namespace "openpuck"). Keys are at most 15 chars.
// Every function is safe to call from any task. storage_save_async never
// waits for a flash write.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Initialize NVS and start the background save task.
void storage_init(void);

// Load the blob for key into buf (capacity *len). On success *len is set to
// the stored size and true is returned.
bool storage_load(const char *key, uint8_t *buf, size_t *len);

// Save now, on the calling task (used just before power-off).
bool storage_save(const char *key, const uint8_t *buf, size_t len);

// Queue a save for the background task; returns immediately. A newer request
// for the same key replaces one that hasn't been written yet.
void storage_save_async(const char *key, const uint8_t *buf, size_t len);
