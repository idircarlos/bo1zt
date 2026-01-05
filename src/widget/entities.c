#include "widget/entities.h"
#include "widget.h"
#include "widget/widget_internal.h"
#include <GL/gl.h>

typedef struct {
    int *currentEntities;
    int *maxEntities;
} EntitiesData;

static void entitiesRender(Widget* widget) {
    if (!widget || !widget->displayData || !widget->win.hwnd) {
        if (!widget->win.hwnd)
        return;   
    }
    EntitiesData* data = (EntitiesData*)widget->displayData;

    HDC hdc_win = GetDC(widget->win.hwnd);
    if (!hdc_win) {
        if (!hdc_win)
        return;
    }

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    int current = data->currentEntities ? *(data->currentEntities) : 0;
    int max = data->maxEntities ? *(data->maxEntities) : 1;
    if (max <= 0) max = 1;

    float percentage = (float)current / (float)max;
    if (percentage > 1.0f) percentage = 1.0f;
    if (percentage < 0.0f) percentage = 0.0f;

    float barWidth = 2.0f * percentage;
    Color c = widget->render.textColor;

    // Draw translucent background
    // Using a visible gray color so widgetUpdateLayeredWindow detects it as non-background
    glColor4ub(30, 30, 30, 255);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Draw progress bar fill using widget's configured color (fully opaque)
    glColor4ub(c.r, c.g, c.b, 255);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(-1.0f + barWidth, -1.0f);
    glVertex2f(-1.0f + barWidth, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    glFinish();

    widgetUpdateLayeredWindow(widget, hdc_win);
    ReleaseDC(widget->win.hwnd, hdc_win);
}

static void entitiesDestroy(Widget* widget) {
    if (!widget) return;
    free(widget->displayData);
    widget->displayData = NULL;
}

static WidgetVTable entitiesVTable = {
    .render = entitiesRender,
    .destroy = entitiesDestroy
};

Widget* entitiesWidgetCreate(int *currentEntities, int *maxEntities) {
    EntitiesData* data = (EntitiesData*)calloc(1, sizeof(EntitiesData));
    if (!data) return NULL;

    data->currentEntities = currentEntities;
    data->maxEntities = maxEntities;
    return widgetCreate("Entities", &entitiesVTable, data, WIDGET_ENTITIES_RECT, WIDGET_ENTITIES_FONT_SIZE);
}
