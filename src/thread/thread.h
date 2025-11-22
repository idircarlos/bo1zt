#ifndef THREAD_H_
#define THREAD_H_

#include <stdint.h>
#include <stdbool.h>
#include "../process/process_internal.h"

typedef struct Thread Thread;

Thread *threadCreate(int (*entryPoint)(void *), void *data);
Thread *threadCreateWatchdog(Thread *targetThread, uint32_t timeoutMillis, int (*errorCallback)(void *), void *errorCallbackData);
Thread *threadCreateRemote(Process *process, uintptr_t remoteFunctionAddress, uintptr_t parameter);
int threadGetExitCode(Thread *thread);
bool threadWait(Thread *thread, int millis);
void threadSleep(int millis);
bool threadClose(Thread *thread);

#endif // THREAD_H_