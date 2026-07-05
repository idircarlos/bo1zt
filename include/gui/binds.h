#ifndef UI_BINDS_H_
#define UI_BINDS_H_

#include "gui.h"

uiControl *uiBindsBuild(Client *client, uiWindow *parent);
bool uiBindsIsSavable();
void uiBindsReset();
void uiBindsUpdate(void);

#endif // UI_BINDS_H_
