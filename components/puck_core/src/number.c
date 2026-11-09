#include "puck_number.h"

bool puck_number_rotation_ambiguous(int32_t value)
{
    if (value < 0) {
        return false;
    }
    // Rotating 180 degrees reverses the digits and turns each one around.
    // The original's lowest digit becomes the rotated number's leading one,
    // so a trailing zero (other than in "0" itself) is a leading zero.
    if (value >= 10 && value % 10 == 0) {
        return false;
    }
    uint32_t rest = (uint32_t)value;
    uint64_t rotated = 0;
    do {
        const uint32_t digit = rest % 10u;
        uint32_t turned;
        switch (digit) {
        case 0:
        case 8:
            turned = digit;
            break;
        case 6:
            turned = 9;
            break;
        case 9:
            turned = 6;
            break;
        default:
            return false;
        }
        rotated = rotated * 10u + turned;
        rest /= 10u;
    } while (rest != 0u);
    return rotated != (uint64_t)value;
}
