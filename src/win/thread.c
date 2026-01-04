#include "win/thread.h"
#include "win/process_internal.h"
#include "logger.h"
#include <windows.h>

struct Thread {
    HANDLE handle;
};

typedef struct {
    Thread *targetThread;
    uint32_t timeoutMillis;
    int (*errorCallback)(void *);
    void *errorCallbackData;
} WatchdogProps;

static int threadWatchdogRoutine(void *data) {
    WatchdogProps *props = (WatchdogProps *)data;

    // Wait until target thread ends or timeout expires
    DWORD res = WaitForSingleObject(props->targetThread->handle, props->timeoutMillis);

    if (res == WAIT_TIMEOUT) {
        LOG_WARN("Timeout expired for Thread %d. Terminating thread and executing error callback function...", props->targetThread->handle);
        TerminateThread(props->targetThread->handle, 1);
        props->errorCallback(props->errorCallbackData);
    }

    threadClose(props->targetThread);
    free(props);
    return 0;
}

Thread *threadCreate(int (*entryPoint)(void *), void *data) {
    Thread *thread = (Thread*)malloc(sizeof(Thread));
    thread->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)entryPoint, data, 0, NULL);
    return thread;
}

Thread *threadCreateWatchdog(Thread *targetThread, uint32_t timeoutMillis, int (*errorCallback)(void *), void *errorCallbackData) {
    LOG_INFO("Launching Watchdog Thread against Thread %d", targetThread->handle);
    WatchdogProps *props = (WatchdogProps*)malloc(sizeof(WatchdogProps));
    props->targetThread = targetThread;
    props->timeoutMillis = timeoutMillis;
    props->errorCallback = errorCallback;
    props->errorCallbackData = errorCallbackData;

    Thread *thread = (Thread*)malloc(sizeof(Thread));
    thread->handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadWatchdogRoutine, props, 0, NULL);
    return thread;
}

Thread *threadCreateRemote(Process *process, uintptr_t remoteFunctionAddress, uintptr_t parameter) {
    Thread *thread = (Thread*)malloc(sizeof(Thread));
    thread->handle = CreateRemoteThread(_processGetHandle(process), NULL, 0, (LPTHREAD_START_ROUTINE)remoteFunctionAddress, (void*)parameter, 0, NULL);
    return thread;
}

int threadGetExitCode(Thread *thread) {
    if (!thread) return -1;
    DWORD exitCode;
    if (GetExitCodeThread(thread->handle, &exitCode)) {
        return (int)exitCode;
    }
    return -1;
}

bool threadWait(Thread *thread, int millis) {
    if (!thread) return false;
    DWORD res = WaitForSingleObject(thread->handle, millis);
    return res == WAIT_OBJECT_0;
}

bool threadClose(Thread *thread) {
    if (!thread) return false;
    return CloseHandle(thread->handle);
}

void threadSleep(int millis) {
    Sleep(millis);
}
