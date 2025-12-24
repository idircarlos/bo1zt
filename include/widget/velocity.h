#ifndef VELOCITY_WIDGET_H_
#define VELOCITY_WIDGET_H_

#include "widget.h"

#define WIDGET_VELOCITY_RECT rectCreate(200, 200, 175, 50)
#define WIDGET_VELOCITY_FONT_SIZE 36

// Velocity-specific operations
Widget* velocityWidgetCreate(float *movementSpeed);

#endif // VELOCITY_WIDGET_H_
