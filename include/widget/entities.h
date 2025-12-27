#ifndef ENTITIES_WIDGET_H_
#define ENTITIES_WIDGET_H_

#include "widget.h"

#define WIDGET_ENTITIES_RECT rectCreate(200, 400, 400, 40)
#define WIDGET_ENTITIES_FONT_SIZE 0

Widget* entitiesWidgetCreate(int *currentEntities, int *maxEntities);

#endif // ENTITIES_WIDGET_H_
