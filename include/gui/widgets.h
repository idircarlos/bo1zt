#ifndef UI_WIDGETS_H_
#define UI_WIDGETS_H_

#include "gui.h"

uiControl *uiWidgetsBuild(Client *client, uiWindow *parent);
void uiWidgetsReload(void);
void uiWidgetsUpdate(void);

#endif // UI_WIDGETS_H_
