#ifndef TIMER_WIDGET_H_
#define TIMER_WIDGET_H_

#include "widget/widget.h"

#define WIDGET_TIMER_RECT rectCreate(200, 200, 300, 100)
#define WIDGET_TIMER_FONT_SIZE 72

// Widget creation (Timer object is managed externally)
Widget* timerWidgetCreate(int *elapsed);

#endif // TIMER_WIDGET_H_
