#include "machine.h"

#include <string.h>

void initializeMachine(Machine* machine, int qtdTasks) {
	machine->tasks = createStack(qtdTasks);
	machine->qtdTasks = 0;
	machine->makespan = 0;
}

void destroyMachine(Machine* machine) {
	freeStack(machine->tasks);
}

void copyMachine(Machine* dest, const Machine* src) {
	dest->tasks = createStack(src->tasks->capacity);
	dest->tasks->top = src->tasks->top;
	memcpy(dest->tasks->arr, src->tasks->arr, sizeof(int) * src->tasks->capacity);
	dest->qtdTasks = src->qtdTasks;
	dest->makespan = src->makespan;
}

void pushTask(Machine* machine, int task) {
	push(machine->tasks, task);
	++machine->qtdTasks;
	machine->makespan += task;
}

int popTask(Machine* machine) {
	int task;

	pop(machine->tasks, &task);
	--machine->qtdTasks;
	machine->makespan -= task;

	return task;
}

int peekTask(Machine* machine) {
	int task;

	peek(machine->tasks, &task);

	return task;
}

