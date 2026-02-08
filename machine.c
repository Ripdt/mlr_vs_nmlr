#include "machine.h"

void initializeMachine(Machine* machine, int qtdTasks) {
	machine->tasks = createStack(qtdTasks);
	machine->qtdTasks = 0;
	machine->makespan = 0;
}

void destroyMachine(Machine* machine) {
	freeStack(machine->tasks);
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

