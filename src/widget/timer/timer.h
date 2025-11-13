#ifndef TIMER_WIDGET_H_
#define TIMER_WIDGET_H_

#include "../widget.h"
#include "../../common/common.h"

#define WIDGET_TIMER_RECT rectCreate(200, 200, 300, 100)
#define WIDGET_TIMER_FONT_SIZE 72

// Timer-specific operations
Widget* timerWidgetCreate();
void timerWidgetStart(Widget* timer);
void timerWidgetPause(Widget* timer);
void timerWidgetRestart(Widget* timer);
double timerWidgetGetElapsedTime(const Widget* timer);

#endif // TIMER_WIDGET_H_