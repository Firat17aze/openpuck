// Rotation ambiguity of displayed numbers (6 vs 9 on a round puck).
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// True if value, rotated 180 degrees, reads as a different valid number
// (so it needs a mark showing which way up it is). Digits 0 and 8 rotate to
// themselves, 6 and 9 to each other; any other digit (including 1, which isn't
// symmetric in our font) makes the rotation unreadable. A rotation with a
// leading zero (e.g. 60 -> "09") isn't a valid display, so it's unambiguous.
// Negative values return false.
bool puck_number_rotation_ambiguous(int32_t value);

#ifdef __cplusplus
}
#endif
