#ifndef WIDGET_H_
#define WIDGET_H_

#include <windows.h>
#include <stdbool.h>
#include "utils/common.h"

typedef struct WidgetVTable WidgetVTable;
typedef struct Widget Widget;

void widgetDestroy(Widget* widget);
void widgetShow(Widget* widget);
void widgetHide(Widget* widget);
bool widgetIsVisible(const Widget* widget);
bool widgetIsTransforming(const Widget* widget);
Rect widgetGetPosition(const Widget* widget);
void widgetSetPosition(Widget* widget, Rect rect);
int widgetGetFontSize(const Widget *widget);
void widgetSetFontSize(Widget* widget, int fontSize);
void widgetSetFont(Widget* widget, const char* face);
void widgetSetTextColor(Widget* widget, Color color);

#endif // WIDGET_H_
