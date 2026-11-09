#include "puck_save_policy.h"

void puck_save_policy_init(puck_save_policy_t *policy, uint32_t idle_ms, uint32_t max_delay_ms)
{
    *policy = (puck_save_policy_t) {
        .idle_ms = idle_ms,
        .max_delay_ms = max_delay_ms,
    };
}

void puck_save_policy_note_change(puck_save_policy_t *policy, uint32_t now_ms)
{
    if (!policy->pending) {
        policy->pending = true;
        policy->first_change_ms = now_ms;
    }
    policy->last_change_ms = now_ms;
}

bool puck_save_policy_due(const puck_save_policy_t *policy, uint32_t now_ms)
{
    if (!policy->pending) {
        return false;
    }
    // Unsigned subtraction keeps working across uint32_t wraparound.
    return now_ms - policy->last_change_ms >= policy->idle_ms ||
           now_ms - policy->first_change_ms >= policy->max_delay_ms;
}

bool puck_save_policy_pending(const puck_save_policy_t *policy)
{
    return policy->pending;
}

void puck_save_policy_mark_saved(puck_save_policy_t *policy)
{
    policy->pending = false;
}
