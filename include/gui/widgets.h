#ifndef UI_WIDGETS_H_
#define UI_WIDGETS_H_

#include "gui.h"
#include "widget.h"

UIControlGroup *uiWidgetsBuildControlGroup();
const char *uiWidgetsGetName(int index);
bool uiWidgetsIsEnabled(int index);
const char *uiWidgetsGetFont(int index);
Color uiWidgetsGetTextColor(int index);
bool uiWidgetsIsHideOutsideGameChecked(int index);
bool uiWidgetsIsSavable();
void uiWidgetsReset();
Rect uiWidgetsGetRect(int index);
int uiWidgetsGetFontSize(int index);
Rect uiWidgetsGetDefaultRect(int index);
int uiWidgetsGetDefaultFontSize(int index);

// Widget access
Widget* uiWidgetsGetCycleWidget();

#endif // UI_WIDGETS_H_
