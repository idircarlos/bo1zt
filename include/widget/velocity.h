#ifndef VELOCITY_WIDGET_H_
#define VELOCITY_WIDGET_H_

#include "widget/widget.h"

#define WIDGET_VELOCITY_RECT rectCreate(200, 200, 175, 50)
#define WIDGET_VELOCITY_FONT_SIZE 36

// Velocity-specific operations
Widget* velocityWidgetCreate();
void velocityWidgetSetSpeed(Widget* velocity, float speed);
float velocityWidgetGetSpeed(const Widget* velocity);

#endif // VELOCITY_WIDGET_H_
