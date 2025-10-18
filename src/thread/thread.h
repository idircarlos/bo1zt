#ifndef THREAD_H_
#define THREAD_H_

typedef struct Thread Thread;

Thread *threadCreate(int (*entryPoint)(void *), void *data);
void threadSleep(int millis);

#endif // THREAD_H_