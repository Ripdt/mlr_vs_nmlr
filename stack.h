#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
	int capacity;
	int* arr;
	int top;
} Stack;

Stack* createStack(int capacity);
void freeStack(Stack* stack);

void push(Stack* stack, int value);
bool pop(Stack* stack, int* peek);
bool peek(Stack* stack, int* peek);

bool isEmpty(Stack* stack);
bool isFull(Stack* stack);

#endif
