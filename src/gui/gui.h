#pragma once

#include "../controller/controller.h"

void guiInit(Controller *controller);
void guiRun(void);

bool guiIsCheatChecked(Controller *controller, Cheat cheat);

void guiCleanup(void);