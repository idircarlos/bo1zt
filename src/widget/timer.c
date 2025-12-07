#include "widget/timer.h"
#include "widget/widget.h"
#include "widget/widget_internal.h"
#include <stdio.h>
#include <GL/gl.h>

typedef struct {
    int *millis;
} TimerData;

static void timerRender(Widget* widget) {
    if (!widget || !widget->displayData) return;
    TimerData* data = (TimerData*)widget->displayData;

    HDC hdc_win = GetDC(widget->win.hwnd);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Get elapsed time from timer (in milliseconds)
    double elapsed_millis = data->millis ? *(data->millis) : 0;
    double elapsed_seconds = elapsed_millis / 1000.0;

    // Format time as HH:MM:SS
    char buf[32];
    unsigned long total_seconds = (unsigned long)elapsed_seconds;
    unsigned int hrs = (unsigned int)(total_seconds / 3600);
    unsigned int mins = (unsigned int)((total_seconds / 60) % 60);
    unsigned int secs = (unsigned int)(total_seconds % 60);
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hrs, mins, secs);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    widgetDrawText(widget, buf);
    glFinish();

    // Update layered window
    widgetUpdateLayeredWindow(widget, hdc_win);
    ReleaseDC(widget->win.hwnd, hdc_win);
}

static void timerDestroy(Widget* widget) {
    if (!widget) return;
    // Timer is managed externally, we only free the wrapper data
    free(widget->displayData);
    widget->displayData = NULL;
}

static WidgetVTable timerVTable = {
    .render = timerRender,
    .destroy = timerDestroy
};

Widget* timerWidgetCreate(int *elapsed) {    
    TimerData* data = (TimerData*)calloc(1, sizeof(TimerData));
    if (!data) return NULL;
    
    data->millis = elapsed;
    return widgetCreate("Timer", &timerVTable, data, WIDGET_TIMER_RECT, WIDGET_TIMER_FONT_SIZE);
}
