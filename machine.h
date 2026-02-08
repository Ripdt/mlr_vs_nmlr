#ifndef MACHINE_H
#define MACHINE_H

#include "stack.h"

typedef struct {
	Stack* tasks;
	int qtdTasks;
	int makespan;
} Machine;

void initializeMachine(Machine* machine, int qtdTasks);
void destroyMachine(Machine* machine);

void pushTask(Machine* machine, int task);
int popTask(Machine* machine);
int peekTask(Machine* machine);

#endif
