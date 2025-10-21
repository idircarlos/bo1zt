#ifndef GUI_H_
#define GUI_H_

#include <ui.h>
#include "../controller/controller.h"

typedef struct UIControlGroup UIControlGroup;

UIControlGroup *guiControlGroupCreate(uiGroup *(*build)(Controller *, uiWindow *), void (*update)());
void guiInit(Controller *controller);
void guiRun(void);
bool guiIsCheatChecked(CheatName cheat);
void guiUpdate();
void guiCleanup(void);

#endif // GUI_H_