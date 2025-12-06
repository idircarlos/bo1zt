#ifndef UI_GAME_H_
#define UI_GAME_H_

#include "gui/gui.h"

UIControlGroup *uiGameBuildControlGroup();
bool uiGameIsChecked(CheatName cheat);
char *uiGameGetLocation();
char *uiGameGetHostname();

#endif // UI_GAME_H_
