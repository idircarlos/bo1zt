#ifndef UI_CUSTOMIZER_H_
#define UI_CUSTOMIZER_H_

#include "../../gui.h"
#include "../../../common/common.h"

UIControlGroup *uiCustomizerBuildControlGroup();
Color uiCustomizerGetCheatColor(SimpleCheatName cheat);
int uiCustomizerGetCheatInt(SimpleCheatName cheat);

#endif // UI_CUSTOMIZER_H_
