#ifndef THREAD_H_
#define THREAD_H_

#include <stdint.h>

typedef struct Thread Thread;

Thread *threadCreate(int (*entryPoint)(void *), void *data);
Thread *threadCreateWatchdog(Thread *targetThread, uint32_t timeoutMillis, int (*errorCallback)(void *), void *errorCallbackData);
bool threadClose(Thread *thread);
void threadSleep(int millis);

#endif // THREAD_H_