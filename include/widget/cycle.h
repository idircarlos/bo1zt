#ifndef CYCLE_WIDGET_H_
#define CYCLE_WIDGET_H_

#include "widget.h"
#include "logic/game/powerup.h"

#define WIDGET_CYCLE_RECT rectCreate(200, 300, 266, 44)
#define WIDGET_CYCLE_ICON_SIZE 32
#define WIDGET_CYCLE_ICON_PADDING 6
#define WIDGET_CYCLE_POWERUP_COUNT 7

Widget* cycleWidgetCreate(void);
void cycleWidgetActivate(Widget* cycle, Powerup powerup);
void cycleWidgetReset(Widget* cycle);
bool cycleWidgetIsActive(const Widget* cycle, Powerup powerup);

#endif // CYCLE_WIDGET_H_
