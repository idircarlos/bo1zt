#ifndef TIMER_WIDGET_H_
#define TIMER_WIDGET_H_

#include "../widget.h"

// Timer-specific operations
Widget* timerWidgetCreate(int x, int y);
void timerWidgetStart(Widget* timer);
void timerWidgetPause(Widget* timer);
void timerWidgetRestart(Widget* timer);
double timerWidgetGetElapsedTime(const Widget* timer);

#endif // TIMER_WIDGET_H_