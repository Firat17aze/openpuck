// Development aid (CONFIG_OPENPUCK_POWER_STATS): an on-screen label showing
// how much of the time since boot the chip spent in light sleep.
#pragma once

#include "sdkconfig.h"

#if CONFIG_OPENPUCK_POWER_STATS
// Create the label on the top layer. Call after ui_start.
void power_stats_start(void);
#else
static inline void power_stats_start(void) {}
#endif
