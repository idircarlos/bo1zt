#include "widget/velocity.h"
#include "widget.h"
#include "widget/widget_internal.h"
#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>

typedef struct {
    float *speed;
} VelocityData;

static void velocityRender(Widget* widget) {
    if (!widget || !widget->displayData) return;
    VelocityData* data = (VelocityData*)widget->displayData;

    HDC hdc_win = GetDC(widget->win.hwnd);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    char buf[32];
    // Keep a fixed-width number to avoid widget width changes. Assuming max 3 integer digits and 2 decimals
    float s = data->speed ? *(data->speed) : 0.0f;
    if (s < 0.0f) s = 0.0f;
    if (s > 999.99f) s = 999.99f;
    // %6.2f ensures the numeric part is always width 6 (e.g., "  0.00", "999.99")
    snprintf(buf, sizeof(buf), "%6.2f u/s", s);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    widgetDrawText(widget, buf);
    glFinish();

    widgetUpdateLayeredWindow(widget, hdc_win);
    ReleaseDC(widget->win.hwnd, hdc_win);
}

static void velocityDestroy(Widget* widget) {
    if (!widget) return;
    free(widget->displayData);
    widget->displayData = NULL;
}

static WidgetVTable velocityVTable = {
    .render = velocityRender,
    .destroy = velocityDestroy
};

Widget* velocityWidgetCreate(float *movementSpeed) {
    VelocityData* data = (VelocityData*)calloc(1, sizeof(VelocityData));
    if (!data) return NULL;

    data->speed = movementSpeed;
    return widgetCreate("VelocityFloat", &velocityVTable, data, WIDGET_VELOCITY_RECT, WIDGET_VELOCITY_FONT_SIZE);
}
