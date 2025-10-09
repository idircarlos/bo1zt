#ifndef OFFSET_H_
#define OFFSET_H_

#include <stdint.h>

typedef enum {
    CHEAT_GOD_MODE,
    CHEAT_NO_CLIP,
    CHEAT_INVISIBLE,
} Cheat;

typedef enum {
    CHEAT_OFFSET_GOD_MODE = 0x01A79868,
    CHEAT_OFFSET_NO_CLIP = 0x01C0A74C,
    CHEAT_OFFSET_INVISIBLE = CHEAT_OFFSET_GOD_MODE, // This Cheat uses the same address as God Mode but with a different value.

    CHEAT_OFFSET_INVALID = 0x00000000,
} CheatOffset;

typedef enum {
    CHEAT_VALUE_GOD_MODE_ON = 2081,
    CHEAT_VALUE_GOD_MODE_OFF = 2080,

    CHEAT_VALUE_NO_CLIP_ON = 1,
    CHEAT_VALUE_NO_CLIP_OFF = 0,

    CHEAT_VALUE_INVISIBLE_ON = 2085, // There is no off value for Invisible. Invisible is just God Mode attributes + Invisible.
} CheatValue;

#endif // OFFSET_H_