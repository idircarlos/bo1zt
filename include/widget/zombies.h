#ifndef ZOMBIES_WIDGET_H_
#define ZOMBIES_WIDGET_H_

#include "widget.h"

#define WIDGET_ZOMBIES_RECT rectCreate(200, 320, 400, 80)
#define WIDGET_ZOMBIES_FONT_SIZE 48

Widget* zombiesWidgetCreate(int *zombiesLeft);

#endif // ZOMBIES_WIDGET_H_
