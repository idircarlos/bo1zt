#ifndef UI_WIDGETS_H_
#define UI_WIDGETS_H_

#include "gui.h"

UIControlGroup *uiWidgetsBuildControlGroup();
const char *uiWidgetsGetName(int index);
bool uiWidgetsIsEnabled(int index);
const char *uiWidgetsGetFont(int index);
Color uiWidgetsGetTextColor(int index);
bool uiWidgetsIsHideOnDefaultChecked(int index);
bool uiWidgetsIsSavable();
void uiWidgetsReset();
Rect uiWidgetsGetRect(int index);
int uiWidgetsGetFontSize(int index);
Rect uiWidgetsGetDefaultRect(int index);
int uiWidgetsGetDefaultFontSize(int index);

#endif // UI_WIDGETS_H_
