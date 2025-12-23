#include "widget/zombies.h"
#include "widget.h"
#include "widget/widget_internal.h"
#include <stdio.h>
#include <GL/gl.h>

typedef struct {
    int *zombiesLeft;
} ZombiesData;

static void zombiesRender(Widget* widget) {
    if (!widget || !widget->displayData) return;
    ZombiesData* data = (ZombiesData*)widget->displayData;

    HDC hdc_win = GetDC(widget->win.hwnd);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    int left = data->zombiesLeft ? *(data->zombiesLeft) : 0;

    char buf[64];
    snprintf(buf, sizeof(buf), "Zombies Left: %d", left);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    widgetDrawText(widget, buf);
    glFinish();

    widgetUpdateLayeredWindow(widget, hdc_win);
    ReleaseDC(widget->win.hwnd, hdc_win);
}

static void zombiesDestroy(Widget* widget) {
    if (!widget) return;
    free(widget->displayData);
    widget->displayData = NULL;
}

static WidgetVTable zombiesVTable = {
    .render = zombiesRender,
    .destroy = zombiesDestroy
};

Widget* zombiesWidgetCreate(int *zombiesLeft) {
    ZombiesData* data = (ZombiesData*)calloc(1, sizeof(ZombiesData));
    if (!data) return NULL;

    data->zombiesLeft = zombiesLeft;
    return widgetCreate("Zombies", &zombiesVTable, data, WIDGET_ZOMBIES_RECT, WIDGET_ZOMBIES_FONT_SIZE);
}
