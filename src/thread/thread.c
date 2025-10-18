#include "thread.h"
#include "../logger/logger.h"
#include <windows.h>

struct Thread {
    HANDLE handle;
    int (*entryPoint)(void *);
    void *data;
};

Thread *threadCreate(int (*entryPoint)(void *), void *data) {
    Thread *thread = (Thread*)malloc(sizeof(Thread));
    thread->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)entryPoint, data, 0, NULL);
    thread->entryPoint = entryPoint;
    thread->data = data;
    return thread;
}

void threadSleep(int millis) {
    Sleep(millis);
}
