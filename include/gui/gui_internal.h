#ifndef GUI_INTERNAL_H_
#define GUI_INTERNAL_H_

#include "gui.h"
#include "utils/color.h"

RGBAColor buildColor(uiColorButton *button);
void setColorButton(uiColorButton *button, RGBAColor color);

#endif // GUI_INTERNAL_H_
