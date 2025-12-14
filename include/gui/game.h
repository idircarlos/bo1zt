#ifndef UI_GAME_H_
#define UI_GAME_H_

#include "gui.h"

UIControlGroup *uiGameBuildControlGroup();
bool uiGameIsChecked(CheatName cheat);
char *uiGameGetLocation();
char *uiGameGetHostname();
bool uiGamePromptLocation(void);

#endif // UI_GAME_H_
