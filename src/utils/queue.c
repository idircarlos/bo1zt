#include "utils/queue.h"
#include <stdlib.h>

typedef struct QueueNode {
    void *value;
    struct QueueNode *next;
} QueueNode;

typedef struct Queue {
    QueueNode *head;
    QueueNode *tail;
    int count;
} Queue;


Queue* queueCreate() {
    Queue *queue = (Queue*)malloc(sizeof(Queue));
    queue->head = queue->tail = NULL;
    queue->count = 0;
    return queue;
}

void queueDestroy(Queue *queue) {
    if (!queue) return;

    QueueNode *n = queue->head;
    while (n) {
        QueueNode *tmp = n->next;
        free(n);
        n = tmp;
    }

    free(queue);
}

void queuePush(Queue *queue, void *value) {
    QueueNode *node = (QueueNode*)malloc(sizeof(QueueNode));
    node->value = value;
    node->next = NULL;

    if (queue->tail == NULL) {
        queue->head = queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }

    queue->count++;
}

void* queuePop(Queue *queue) {
    if (queue->head == NULL) return NULL;

    QueueNode *node = queue->head;
    void *value = node->value;

    queue->head = node->next;
    if (queue->head == NULL)
        queue->tail = NULL;

    free(node);
    queue->count--;

    return value;
}

int queueSize(Queue *queue) {
    return queue->count;
}
