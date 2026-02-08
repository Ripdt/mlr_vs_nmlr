#include "stack.h"

Stack* createStack(int capacity) {
	Stack* stack = (Stack*) malloc(sizeof(Stack));
	stack->top = -1;
	stack->capacity = capacity;

	stack->arr = (int*)malloc(sizeof(int) * capacity);

	return stack;
}

void freeStack(Stack* stack) {
	free(stack->arr);
	free(stack);
}

void push(Stack* stack, int value) {
	if (isFull(stack)) {
		stack->capacity *= 2;
		stack->arr = (int*)realloc(stack->arr, stack->capacity * sizeof(int));
	}

	stack->arr[++stack->top] = value;
}

bool pop(Stack* stack, int* peek) {
	if (isEmpty(stack)) {
		*peek = -1;
		return false;
	}

	*peek = stack->arr[stack->top--];
	return true;
}

bool peek(Stack* stack, int* peek) {
	if (isEmpty(stack)) {
		*peek = -1;
		return false;
	}

	*peek = stack->arr[stack->top];
	return true;
}

bool isEmpty(Stack* stack) {
	return stack->top < 0;
}

bool isFull(Stack* stack) {
	return stack->top == stack->capacity - 1;
}
