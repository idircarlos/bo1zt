#ifndef QUEUE_H
#define QUEUE_H

typedef struct Queue Queue;

Queue* queueCreate();
void queueDestroy(Queue *queue);
void queuePush(Queue *queue, void *value);
void* queuePop(Queue *queue);
int queueSize(Queue *queue);

#endif // QUEUE_H
