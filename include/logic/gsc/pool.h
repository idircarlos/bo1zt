#ifndef GSC_POOL_H
#define GSC_POOL_H

#include "utils/queue.h"
#include <windows.h>
#include <stdbool.h>

#define MAX_WORKERS 10

typedef struct GSCPool {
    const char* responses[MAX_WORKERS];
    bool idles[MAX_WORKERS];
    Queue *waiters;
    CRITICAL_SECTION cs;
} GSCPool;

GSCPool* poolCreate();
void poolDestroy(GSCPool *pool);

int poolAcquire(GSCPool *pool);
void poolRelease(GSCPool *pool, int index);

void poolWriteResponseDirect(GSCPool *pool, int index, const char *response);

#endif // GSC_POOL_H
