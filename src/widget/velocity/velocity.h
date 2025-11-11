#ifndef VELOCITY_WIDGET_H_
#define VELOCITY_WIDGET_H_

#include "../widget.h"

// Velocity-specific operations
Widget* velocityWidgetCreate(int x, int y);
void velocityWidgetSetSpeed(Widget* velocity, float speed);
float velocityWidgetGetSpeed(const Widget* velocity);

#endif // VELOCITY_WIDGET_H_