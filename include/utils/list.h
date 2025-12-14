#ifndef LIST_H_
#define LIST_H_

#include <stdbool.h>
#include <stddef.h>

#define listAddInt(list, val) listAdd(list, (void*)(intptr_t)(val))
#define listGetInt(list, index) ((int)(intptr_t)listGet(list, index))

typedef struct List List;

List *listCreate(void);
void listDestroy(List *list);
void listAdd(List *list, void *data);
void *listGet(List *list, size_t index);
void *listRemove(List *list, size_t index);
size_t listSize(List *list);
bool listIsEmpty(List *list);
void listClear(List *list);

#endif // LIST_H_
