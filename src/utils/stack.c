#include "utils/stack.h"
#include <stdlib.h>

typedef struct StackNode {
    void *value;
    struct StackNode *next;
} StackNode;

typedef struct Stack {
    StackNode *top;
    int count;
} Stack;

Stack* stackCreate() {
    Stack *stack = (Stack*)malloc(sizeof(Stack));
    stack->top = NULL;
    stack->count = 0;
    return stack;
}

void stackDestroy(Stack *stack) {
    if (!stack) return;

    StackNode *n = stack->top;
    while (n) {
        StackNode *tmp = n->next;
        free(n);
        n = tmp;
    }

    free(stack);
}

void stackPush(Stack *stack, void *value) {
    StackNode *node = (StackNode*)malloc(sizeof(StackNode));
    node->value = value;
    node->next = stack->top;
    stack->top = node;
    stack->count++;
}

void* stackPop(Stack *stack) {
    if (stack->top == NULL) return NULL;

    StackNode *node = stack->top;
    void *value = node->value;

    stack->top = node->next;
    free(node);
    stack->count--;

    return value;
}

int stackSize(Stack *stack) {
    return stack->count;
}
