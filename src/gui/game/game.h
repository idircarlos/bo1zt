#ifndef UI_GAME_H_
#define UI_GAME_H_

#include "../gui.h"

UIControlGroup *uiGameBuildControlGroup();
bool uiGameIsChecked(CheatName cheat);
char *uiGameGetHostname();

#endif // UI_GAME_H_
