#include "utils/list.h"
#include <stdlib.h>

typedef struct ListNode ListNode;

struct ListNode {
    void *data;
    ListNode *next;
};

struct List {
    ListNode *head;
    ListNode *tail;
    size_t size;
};

List *listCreate(void) {
    List *list = (List*)malloc(sizeof(List));
    if (!list) return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void listDestroy(List *list) {
    if (!list) return;
    listClear(list);
    free(list);
}

void listAdd(List *list, void *data) {
    if (!list) return;

    ListNode *node = (ListNode*)malloc(sizeof(ListNode));
    if (!node) return;

    node->data = data;
    node->next = NULL;

    if (!list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->size++;
}

void *listGet(List *list, size_t index) {
    if (!list || index >= list->size) return NULL;

    ListNode *current = list->head;
    for (size_t i = 0; i < index; i++) {
        current = current->next;
    }
    return current->data;
}

void *listRemove(List *list, size_t index) {
    if (!list || index >= list->size) return NULL;

    ListNode *prev = NULL;
    ListNode *current = list->head;

    for (size_t i = 0; i < index; i++) {
        prev = current;
        current = current->next;
    }

    void *data = current->data;

    if (!prev) {
        list->head = current->next;
    } else {
        prev->next = current->next;
    }

    if (current == list->tail) {
        list->tail = prev;
    }

    free(current);
    list->size--;
    return data;
}

size_t listSize(List *list) {
    return list ? list->size : 0;
}

bool listIsEmpty(List *list) {
    return !list || list->size == 0;
}

void listClear(List *list) {
    if (!list) return;

    ListNode *current = list->head;
    while (current) {
        ListNode *next = current->next;
        free(current);
        current = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}
