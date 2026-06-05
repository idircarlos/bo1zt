#include "logic/gsc/pool.h"
#include "logger.h"
#include "utils/queue.h"
#include <stdlib.h>
#include <string.h>

#define MAX_WORKERS 10

typedef struct Waiter {
    HANDLE event;
    int assigned;
} Waiter;

GSCPool* poolCreate() {
    GSCPool *pool = (GSCPool*)malloc(sizeof(GSCPool));
    if (!pool) return NULL;

    InitializeCriticalSection(&pool->cs);

    pool->waiters = queueCreate();

    for (int i = 0; i < MAX_WORKERS; i++) {
        pool->responses[i] = NULL;
        pool->idles[i] = true;
    }

    return pool;
}

void poolDestroy(GSCPool *pool) {
    if (!pool) return;

    EnterCriticalSection(&pool->cs);

    Waiter *w;
    while ((w = (Waiter*)queuePop(pool->waiters)) != NULL) {
        w->assigned = -1;
        SetEvent(w->event);
    }

    LeaveCriticalSection(&pool->cs);
    DeleteCriticalSection(&pool->cs);

    queueDestroy(pool->waiters);
    free(pool);
}

int poolAcquire(GSCPool *pool) {
    EnterCriticalSection(&pool->cs);

    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pool->idles[i]) {
            pool->idles[i] = false;
            LeaveCriticalSection(&pool->cs);
            return i;
        }
    }

    Waiter *w = (Waiter*)malloc(sizeof(Waiter));
    w->event = CreateEvent(NULL, FALSE, FALSE, NULL);
    w->assigned = -1;

    queuePush(pool->waiters, w);

    LeaveCriticalSection(&pool->cs);

    WaitForSingleObject(w->event, INFINITE);
    int index = w->assigned;

    CloseHandle(w->event);
    free(w);

    return index;
}

void poolRelease(GSCPool *pool, int index) {
    if (index < 0 || index >= MAX_WORKERS) {
        LOG_ERROR("Index out of range");
        return;
    }

    EnterCriticalSection(&pool->cs);

    pool->idles[index] = true;

    Waiter *w = (Waiter*)queuePop(pool->waiters);
    if (w != NULL) {
        pool->idles[index] = false;
        w->assigned = index;
        SetEvent(w->event);
    }

    LeaveCriticalSection(&pool->cs);
}

void poolWriteResponseDirect(GSCPool *pool, int index, const char *response) {
    if (index < 0 || index >= MAX_WORKERS) return;
    char *copy = _strdup(response);
    InterlockedExchangePointer((PVOID*)&pool->responses[index], copy);
}
