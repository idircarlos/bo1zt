#ifndef UI_CUSTOMIZER_H_
#define UI_CUSTOMIZER_H_

#include "gui/gui.h"
#include "utils/common.h"

UIControlGroup *uiCustomizerBuildControlGroup();
Color uiCustomizerGetCheatColor(SimpleCheatName cheat);
int uiCustomizerGetCheatInt(SimpleCheatName cheat);
bool uiCustomizerIsSavable();
void uiCustomizerReset();

#endif // UI_CUSTOMIZER_H_
