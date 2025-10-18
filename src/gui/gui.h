#ifndef GUI_H_
#define GUI_H_

#include "../controller/controller.h"

void guiInit(Controller *controller);
void guiRun(void);

bool guiIsCheatChecked(Controller *controller, CheatName cheat);

void guiUpdate(Controller *controller);
void guiCleanup(void);

#endif // GUI_H_