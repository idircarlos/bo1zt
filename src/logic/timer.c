#include "logic/timer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <windows.h>

struct Timer{
    bool isRunning;
    LARGE_INTEGER start;
    LARGE_INTEGER pause;
    double elapsed;
};

Timer *timerCreate() {
    Timer *timer = (Timer*)malloc(sizeof(Timer));
    if (!timer) return NULL;
    timer->isRunning = false;
    timer->elapsed = 0.0;
    return timer;
}

void timerStart(Timer *timer) {
    if (!timer) return;
    if (!timer->isRunning) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        timer->start = now;
        timer->isRunning = true;
    }
}

void timerPause(Timer *timer) {
    if (!timer) return;
    if (timer->isRunning) {
        LARGE_INTEGER now, freq;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&freq);
        timer->elapsed += (double)(now.QuadPart - timer->start.QuadPart) * 1000.0 / (double)freq.QuadPart;
        timer->isRunning = false;
    }
}

void timerRestart(Timer *timer, bool pause) {
    if (!timer || !timer->isRunning) return;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    timer->start = now;
    timer->elapsed = 0.0;
    timer->isRunning = !pause;
}

double timerGetElapsedMillis(Timer *timer) {
    if (!timer) return 0.0;
    double totalElapsed = timer->elapsed;
    if (timer->isRunning) {
        LARGE_INTEGER now, freq;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&freq);
        totalElapsed += (double)(now.QuadPart - timer->start.QuadPart) * 1000.0 / (double)freq.QuadPart;
    }
    return totalElapsed;
}

void timerDestroy(Timer *timer) {
    if (timer) {
        free(timer);
    }
}
