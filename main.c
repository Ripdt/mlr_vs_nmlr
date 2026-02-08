#include <math.h>
#include <stdio.h>
#include <time.h>

#include "machine.h"

#define N_MACHINES_INPUTS 3
#define N_R_INPUTS 2
#define N_ALFA_INPUTS 5

#define TASK_FACTOR 100

#define N_EXECUTIONS 10

void monotoneLocalReasoning(int nMachines, int nTasks, FILE* file) {
	int execution = 0;
	while (++execution != N_EXECUTIONS) {
		Machine* machines = (Machine*)malloc(sizeof(Machine) * nMachines);
		for (int i = 0; i < nMachines; i++)
			initializeMachine(&machines[i], nTasks);

		int criticIndex = 0;
		Machine* criticMachine = &machines[criticIndex];

		for (int taskIndex = 0; taskIndex < nTasks; taskIndex++)
			pushTask(criticMachine, rand() % TASK_FACTOR + 1);

		int makespan = criticMachine->makespan;
		int it = 0;
		const clock_t start = clock();
		do {

			int targetIndex = criticIndex + 1;
			if (criticIndex == nMachines - 1) targetIndex = 0;

			Machine* targetMachine = &machines[targetIndex];

			int task = peekTask(criticMachine);
			const int actualMakespan = criticMachine->makespan;
			const int targetMakespan = targetMachine->makespan + task;

			if (targetMakespan > actualMakespan) {
				makespan = actualMakespan;
				criticMachine = NULL;
			}
			else {
				popTask(criticMachine);
				pushTask(targetMachine, task);

				++it;

				if (targetMachine->makespan > criticMachine->makespan) {
					criticMachine = targetMachine;
					criticIndex = targetIndex;
				}
				else if (targetMachine->makespan == criticMachine->makespan) {
					criticMachine = NULL;
				}
			}

		} while (criticMachine);

		const clock_t stop = clock();
		const double duration = (double)(stop - start) / CLOCKS_PER_SEC;

		// heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
		fprintf(file, "monotona,%d,%d,%d,%.2f,%d,%d,NA\n",
				nTasks,
				nMachines,
				execution,
				duration,
				it,
				makespan
			);
		for (int i = 0; i < nMachines; i++) destroyMachine(&machines[i]);
		free(machines);
	}
}

void simulatedAnnealing(int nMachines, int nTasks, int alfa, FILE* file) {
	int execution = 0;
	while (++execution != N_EXECUTIONS) {
		Machine* machines = (Machine*)malloc(sizeof(Machine) * nMachines);
		for (int i = 0; i < nMachines; i++)
			initializeMachine(&machines[i], nTasks);

		int criticIndex = 0;
		Machine* criticMachine = &machines[criticIndex];

		for (int taskIndex = 0; taskIndex < nTasks; taskIndex++)
			pushTask(criticMachine, rand() % TASK_FACTOR + 1);

		int makespan = criticMachine->makespan;
		int it = 0;
		const clock_t start = clock();

		

		const clock_t stop = clock();
		const double duration = (double)(stop - start) / CLOCKS_PER_SEC;

		// heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
		fprintf(file, "monotona,%d,%d,%d,%.2f,%d,%d,NA\n",
				nTasks,
				nMachines,
				execution,
				duration,
				it,
				makespan
			);
		for (int i = 0; i < nMachines; i++) destroyMachine(&machines[i]);
		free(machines);
	}
}

int main() {
	srand(time(NULL));

	const int nMachinesInputs[N_MACHINES_INPUTS] = {10, 20, 50};
	const float rInputs[N_R_INPUTS] = {1.5, 2.};
	const float alfaInputs[N_ALFA_INPUTS] = {.8, .85, .9, .95, .99};
	FILE* file;
	file = fopen("result.txt", "w");
	if (file) {
		fprintf(file, "heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro\n");
	}
	else {
		fprintf(stderr, "Failed to open file ./result.txt\n");
		return 1;
	}

	for (int nMachineIndex = 0; nMachineIndex < N_MACHINES_INPUTS; nMachineIndex++) {
		const int nMachines = nMachinesInputs[nMachineIndex];
		for (int rIndex = 0; rIndex < N_R_INPUTS; rIndex++) {
			const int nTasks = pow(nMachines, rInputs[rIndex]);
			monotoneLocalReasoning(nMachines, nTasks, file);
		}
	}

	for (int nMachineIndex = 0; nMachineIndex < N_MACHINES_INPUTS; nMachineIndex++) {
		const int nMachines = nMachinesInputs[nMachineIndex];
		for (int rIndex = 0; rIndex < N_R_INPUTS; rIndex++) {
			const int nTasks = pow(nMachines, rInputs[rIndex]);
			for (int alfaIndex = 0; alfaIndex < N_ALFA_INPUTS; alfaIndex++)
				simulatedAnnealing(nMachines, nTasks, alfaInputs[alfaIndex], file);
		}
	}

	return 0;
}
