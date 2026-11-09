// Decides when to save, so flash writes never happen on every tap.
//
// A save is due once the state has been idle for idle_ms after its last
// change, or max_delay_ms after the first unsaved change (so a long burst of
// taps still gets saved). Time is passed in, in milliseconds; uint32_t
// wraparound is handled.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t idle_ms;
    uint32_t max_delay_ms;
    bool pending;
    uint32_t first_change_ms;
    uint32_t last_change_ms;
} puck_save_policy_t;

void puck_save_policy_init(puck_save_policy_t *policy, uint32_t idle_ms, uint32_t max_delay_ms);
void puck_save_policy_note_change(puck_save_policy_t *policy, uint32_t now_ms);
bool puck_save_policy_due(const puck_save_policy_t *policy, uint32_t now_ms);
bool puck_save_policy_pending(const puck_save_policy_t *policy);
void puck_save_policy_mark_saved(puck_save_policy_t *policy);

#ifdef __cplusplus
}
#endif
