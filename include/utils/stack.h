#ifndef STACK_H
#define STACK_H

typedef struct Stack Stack;

Stack* stackCreate();
void stackDestroy(Stack *stack);
void stackPush(Stack *stack, void *value);
void* stackPop(Stack *stack);
int stackSize(Stack *stack);

#endif // STACK_H
