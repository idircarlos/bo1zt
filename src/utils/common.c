#include "utils/common.h"

Rect rectCreate(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    Rect rect = {x, y, w, h};
    return rect;
}

void flagsAdd(Flags *flags, Flags mask) {
    *flags |= mask;
}

void flagsRemove(Flags *flags, Flags mask) {
    *flags &= ~mask;
}

bool flagsContains(Flags flags, Flags mask) {
    return (flags & mask) != 0;
}

void flagsClear(Flags *flags) {
    *flags = 0;
}
