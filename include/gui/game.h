#ifndef UI_GAME_H_
#define UI_GAME_H_

#include "gui.h"

#include <stddef.h>

UIControlGroup *uiGameBuildControlGroup();
bool uiGamePromptLocation(char *outDir, size_t size);

#endif // UI_GAME_H_
