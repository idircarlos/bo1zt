#include "timer.h"
#include "../widget.h"
#include "../widget_internal.h"
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <GL/gl.h>

typedef struct {
    bool isRunning;
    LARGE_INTEGER start;
    LARGE_INTEGER pause;
    double elapsed;
} TimerData;

static void timerRender(Widget* widget) {
    if (!widget || !widget->displayData) return;
    TimerData* data = (TimerData*)widget->displayData;

    HDC hdc_win = GetDC(widget->win.hwnd);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Update elapsed time if running
    if (data->isRunning) {
        LARGE_INTEGER now, freq;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&freq);
        data->elapsed = (double)(now.QuadPart - data->start.QuadPart) / freq.QuadPart;
    }

    // Format time as HH:MM:SS
    char buf[32];
    unsigned long total_seconds = (unsigned long)data->elapsed;
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
    free(widget->displayData);
    widget->displayData = NULL;
}

static WidgetVTable timerVTable = {
    .render = timerRender,
    .destroy = timerDestroy
};

Widget* timerWidgetCreate() {
    TimerData* data = (TimerData*)calloc(1, sizeof(TimerData));
    return widgetCreate("Timer", &timerVTable, data, WIDGET_TIMER_RECT, WIDGET_TIMER_FONT_SIZE);
}

void timerWidgetStart(Widget* timer) {
    if (!timer || !timer->displayData) return;
    TimerData* data = (TimerData*)timer->displayData;
    
    if (!data->isRunning) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (data->pause.QuadPart) {
            // Resume from pause
            data->start.QuadPart += now.QuadPart - data->pause.QuadPart;
            data->pause.QuadPart = 0;
        } else {
            // Fresh start
            data->start = now;
            data->elapsed = 0.0;
        }
        data->isRunning = true;
    }
}

void timerWidgetPause(Widget* timer) {
    if (!timer || !timer->displayData) return;
    TimerData* data = (TimerData*)timer->displayData;
    
    if (data->isRunning) {
        QueryPerformanceCounter(&data->pause);
        data->isRunning = false;
    }
}

void timerWidgetRestart(Widget* timer) {
    if (!timer || !timer->displayData) return;
    TimerData* data = (TimerData*)timer->displayData;
    
    QueryPerformanceCounter(&data->start);
    data->pause.QuadPart = 0;
    data->elapsed = 0.0;
    data->isRunning = true;
}

double timerWidgetGetElapsedTime(const Widget* timer) {
    if (!timer || !timer->displayData) return 0.0;
    const TimerData* data = (TimerData*)timer->displayData;

    if (!data->isRunning) {
        return data->elapsed;
    }

    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    
    return (double)(now.QuadPart - data->start.QuadPart) / freq.QuadPart;
}
