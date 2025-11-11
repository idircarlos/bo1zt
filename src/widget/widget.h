#ifndef WIDGET_H_
#define WIDGET_H_

#include <windows.h>
#include <stdbool.h>
#include "../common/common.h"

typedef struct WidgetVTable WidgetVTable;
typedef struct Widget Widget;

void widgetDestroy(Widget* widget);
void widgetShow(Widget* widget);
void widgetHide(Widget* widget);
bool widgetIsVisible(const Widget* widget);
void widgetGetPosition(const Widget* widget, int* x, int* y);
void widgetSetPosition(Widget* widget, int x, int y);
void widgetSetFont(Widget* widget, const char* face);
void widgetSetTextColor(Widget* widget, Color color);

#endif // WIDGET_H_
