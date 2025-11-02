#ifndef UI_CUSTOMIZER_H_
#define UI_CUSTOMIZER_H_

#include "../../gui.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

UIControlGroup *uiCustomizerBuildControlGroup();
Color uiCustomizerGetCheatColor(SimpleCheatName cheat);

#endif // UI_CUSTOMIZER_H_
