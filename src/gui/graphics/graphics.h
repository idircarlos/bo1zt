#ifndef UI_GRAPHICS_H_
#define UI_GRAPHICS_H_

#include "../gui.h"

UIControlGroup *uiGraphicsBuildControlGroup();
bool uiGraphicsIsChecked(CheatName cheat);
int uiGraphicsGetFov();
int uiGraphicsGetFovScale();
int uiGraphicsGetFpsCap();

#endif // UI_GRAPHICS_H_
