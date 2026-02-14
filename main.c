#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "machine.h"

#ifndef _WIN32
#include <sys/stat.h>
#endif

#define RESULTS_DIR "results"

#define N_MACHINES_INPUTS 3
#define N_R_INPUTS 2
#define N_ALFA_INPUTS 5

#define TASK_FACTOR 100

#define N_EXECUTIONS 10

Machine* createSolution(int nMachines, int nTasks) {
	Machine* machines = (Machine*)malloc(sizeof(Machine) * nMachines);
	for (int i = 0; i < nMachines; i++)
		initializeMachine(&machines[i], nTasks);

	return machines;
}

Machine* copySolution(Machine* src, int nMachines) {
	Machine* cpyMachines = (Machine*)malloc(sizeof(Machine) * nMachines);
	for (int i = 0; i < nMachines; i++)
		copyMachine(&cpyMachines[i], &src[i]);
	return cpyMachines;
}

void destroySolution(Machine* machines, int nMachines) {
	for (int i = 0; i < nMachines; i++)
		destroyMachine(&machines[i]);
	free(machines);
}

void monotoneLocalReasoning(int nMachines, int nTasks, int nRep, FILE* file) {
	int execution = 0;
	while (execution++ != nRep) {
		Machine* machines = createSolution(nMachines, nTasks);

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

				if (targetMachine->makespan > criticMachine->makespan) {
					criticMachine = targetMachine;
					criticIndex = targetIndex;
				}
				else if (targetMachine->makespan == criticMachine->makespan) {
					makespan = criticMachine->makespan;
					criticMachine = NULL;
				}
			}

			if (criticMachine) {
				makespan = criticMachine->makespan;
			}

			++it;
		} while (criticMachine);

		const clock_t stop = clock();
		const double duration = (double)(stop - start) / CLOCKS_PER_SEC;

		// heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
		fprintf(file, "monotona,%d,%d,%d,%.5f,%d,%d,NA\n",
				nTasks,
				nMachines,
				execution,
				duration,
				it,
				makespan
			);

		destroySolution(machines, nMachines);
	}
}

void monotoneLocalReasoning_debug(int nMachines, int nTasks, FILE* file) {
	int execution = 0;
	while (execution++ != N_EXECUTIONS) {
		Machine* machines = createSolution(nMachines, nTasks);

		int criticIndex = 0;
		Machine* criticMachine = &machines[criticIndex];

		for (int taskIndex = 0; taskIndex < nTasks; taskIndex++)
			pushTask(criticMachine, rand() % TASK_FACTOR + 1);

		int makespan = criticMachine->makespan;
		int it = 0;
		char buffer[256];

		sprintf(buffer, "results/monotona_%dm_%dt_%dx.txt", nMachines, nTasks, execution);

		FILE* csv;
		csv = fopen(buffer, "w+");
		if (csv) {
			fprintf(csv, "iteration,makespan\n");
		}
		else {
			printf("\nFAILED TO OPEN FILE %s\n", buffer);
			return;
		}

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

				if (targetMachine->makespan > criticMachine->makespan) {
					criticMachine = targetMachine;
					criticIndex = targetIndex;
				}
				else if (targetMachine->makespan == criticMachine->makespan) {
					makespan = criticMachine->makespan;
					criticMachine = NULL;
				}
			}

			if (criticMachine) {
				makespan = criticMachine->makespan;
			}

			++it;
			fprintf(csv, "%d,%d\n", it, makespan);

		} while (criticMachine);

		const clock_t stop = clock();
		const double duration = (double)(stop - start) / CLOCKS_PER_SEC;

		// heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
		fprintf(file, "monotona,%d,%d,%d,%.5f,%d,%d,NA\n",
				nTasks,
				nMachines,
				execution,
				duration,
				it,
				makespan
			);

		fclose(csv);
		destroySolution(machines, nMachines);
	}
}

Machine* findCriticMachine(Machine* machines, int nMachines) {
	Machine* higher = &machines[0];
	for (int i = 1; i < nMachines; i++) {
		if (machines[i].makespan > higher->makespan) higher = &machines[i];
	}
	return higher;
}

float calculateDisturbanceLevel(Machine* machines, int nMachines, Machine* criticMachine) {
	int sum = 0;
	for (int i = 0; i < nMachines; i++) {
		sum += machines[i].makespan;
	}
	float mu = (float)sum / nMachines;

	return (criticMachine->makespan - mu) / mu;
}

void applyDisturbance(Machine* machines, int nMachines, float disturbanceLevel) {
	Machine* critic = findCriticMachine(machines, nMachines);
	
	if (critic->qtdTasks == 0) return;
	
	int tasksToMove = (int)(disturbanceLevel * critic->qtdTasks);
	if (tasksToMove < 1) tasksToMove = 1;
	if (tasksToMove > critic->qtdTasks) tasksToMove = critic->qtdTasks;
	
	for (int i = 0; i < tasksToMove; i++) {
		if (critic->qtdTasks == 0) break;
		int task = popTask(critic);
		int targetIdx;
		do {
			targetIdx = rand() % nMachines;
		} while (&machines[targetIdx] == critic && nMachines > 1);

		pushTask(&machines[targetIdx], task);
	}
}

/* Executa uma única replicação do SA com coleta de métricas (convergência e taxa de aceitação). */
static void run_one_sa_replication(int nMachines, int nTasks, float alfa, int repNum,
	FILE* mainFile, const char* resultsDir) {
	Machine* machines = createSolution(nMachines, nTasks);

	for (int taskIndex = 0; taskIndex < nTasks; taskIndex++)
		pushTask(&machines[0], rand() % TASK_FACTOR + 1);

	float temperature = 10000.0f;
	Machine* criticMachine = &machines[0];
	int makespan = criticMachine->makespan;
	int best_makespan = makespan; /* métrica: melhor Cmax ao longo da execução */
	int it = 0;

	/* Métricas para taxa de aceitação de pioras (análise 3) */
	int total_movimentos = 0;
	int pioras_propostas = 0;
	int pioras_aceitas = 0;

	FILE* convFile = NULL;
	if (resultsDir) {
		char path[256];
		snprintf(path, sizeof(path), "%s/sa_convergence_%d_%d_%.2f_%d.csv", resultsDir, nTasks, nMachines, alfa, repNum);
		convFile = fopen(path, "w");
		if (convFile)
			fprintf(convFile, "iteracao,temperatura,makespan_atual,makespan_melhor,aceita,piora_aceita\n");
	}

	const clock_t start = clock();

	while (temperature > 0.1f) {
		Machine* oldSolution = machines;
		Machine* newSolution = copySolution(machines, nMachines);

		float disturbanceLevel = calculateDisturbanceLevel(newSolution, nMachines, findCriticMachine(newSolution, nMachines));
		applyDisturbance(newSolution, nMachines, disturbanceLevel);

		const int actualMakespan = makespan;
		Machine* newCriticMachine = findCriticMachine(newSolution, nMachines);
		const int newMakespan = newCriticMachine->makespan;

		/* Coleta: total de movimentos por iteração */
		total_movimentos++;
		/* Coleta: piora proposta quando newMakespan > actualMakespan */
		int foi_piora = (newMakespan > actualMakespan);
		if (foi_piora) pioras_propostas++;

		int accepted = 0;
		int piora_aceita = 0;

			if (newMakespan <= actualMakespan) {
				machines = newSolution;
				criticMachine = newCriticMachine;
				destroySolution(oldSolution, nMachines);
			}
			else {
				const float delta = (float)(newMakespan - actualMakespan);
				const float acceptanceProbability = expf(-delta / temperature);
				const float randomValue = (float)rand() / RAND_MAX;
				if (randomValue < acceptanceProbability) {
					machines = newSolution;
					criticMachine = newCriticMachine;
					destroySolution(oldSolution, nMachines);
				}
				else {
					destroySolution(newSolution, nMachines);
				}
			}

			makespan = criticMachine->makespan;
			temperature *= alfa;
			++it;
		}

		const clock_t stop = clock();
		const double duration = (double)(stop - start) / CLOCKS_PER_SEC;

		// heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
		fprintf(file, "temperasimulada,%d,%d,%d,%.5f,%d,%d,%.2f\n",
				nTasks,
				nMachines,
				execution,
				duration,
				it,
				makespan,
				alfa
			);

		destroySolution(machines, nMachines);
	}
}

void simulatedAnnealing_debug(int nMachines, int nTasks, float alfa, FILE* file) {
	int execution = 0;
	while (execution++ != N_EXECUTIONS) {
		Machine* machines = createSolution(nMachines, nTasks);

		for (int taskIndex = 0; taskIndex < nTasks; taskIndex++)
			pushTask(&machines[0], rand() % TASK_FACTOR + 1);

		float temperature = 10000.0f;

		Machine* criticMachine = &machines[0];
		int makespan = criticMachine->makespan;
		int it = 0;

		char buffer[256];
		const int iAlfa = alfa*100;
		sprintf(buffer, "results/tempera_%dm_%dt_%da_%dx.txt", nMachines, nTasks, iAlfa, execution);

		FILE* csv;
		csv = fopen(buffer, "w+");
		if (csv) {
			fprintf(csv, "temperatura,makespan\n");
		}
		else {
			printf("\nFAILED TO OPEN FILE %s\n", buffer);
			return;
		}

		const clock_t start = clock();

		while (temperature > 0.1f) {
			fprintf(csv, "%.5f,%d\n", temperature, makespan);

			Machine* oldSolution = machines;
			Machine* newSolution = copySolution(machines, nMachines);

			float disturbanceLevel = calculateDisturbanceLevel(newSolution, nMachines, findCriticMachine(newSolution, nMachines));
			applyDisturbance(newSolution, nMachines, disturbanceLevel);

			const int actualMakespan = makespan;

			Machine* newCriticMachine = findCriticMachine(newSolution, nMachines);
			const int newMakespan = newCriticMachine->makespan;

			if (newMakespan <= actualMakespan) {
				machines = newSolution;
				criticMachine = newCriticMachine;
				destroySolution(oldSolution, nMachines);
			}
			else {
				const float delta = (float)(newMakespan - actualMakespan);
				const float acceptanceProbability = expf(-delta / temperature);
				const float randomValue = (float)rand() / RAND_MAX;
				if (randomValue < acceptanceProbability) {
					machines = newSolution;
					criticMachine = newCriticMachine;
					destroySolution(oldSolution, nMachines);
				}
				else {
					destroySolution(newSolution, nMachines);
				}
			}

		makespan = criticMachine->makespan;
		if (makespan < best_makespan) best_makespan = makespan;
		temperature *= alfa;
		++it;

		/* Coleta convergência (análise 2): iteracao, temperatura, makespan atual/melhor, aceita, piora_aceita */
		if (convFile)
			fprintf(convFile, "%d,%.4f,%d,%d,%d,%d\n", it, temperature, makespan, best_makespan, accepted, piora_aceita);
	}

	const clock_t stop = clock();
	const double duration = (double)(stop - start) / CLOCKS_PER_SEC;

		// heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
		fprintf(file, "temperasimulada,%d,%d,%d,%.5f,%d,%d,%.2f\n",
				nTasks,
				nMachines,
				execution,
				duration,
				it,
				makespan,
				alfa
			);

		fclose(csv);
		destroySolution(machines, nMachines);
	}
}

int main() {
	srand(time(NULL));

#ifdef DEBUG
	printf("DEBUG MODE ACTIVATED\n");
#endif

	const int nMachinesInputs[N_MACHINES_INPUTS] = {10, 20, 50};
	const float rInputs[N_R_INPUTS] = {1.5, 2.};
	const float alfaInputs[N_ALFA_INPUTS] = {.8, .85, .9, .95, .99};

	const char* path = "results";
	struct stat buffer;
	if (stat(path, &buffer) < 0 && errno != ENOENT) {
		printf("FAILED TO CHECK IF DIRECTORY results EXISTS\n");
		return 1;
	}

	if (!S_ISDIR(buffer.st_mode)) {
		if ( mkdir("results", 0755) != 0) {
			printf("FAILED TO CREATE DIRECTORY results\n");
			return 1;
		}
	}

	FILE* file;
#ifdef DEBUG
	file = fopen("results/result_db.txt", "w+");
#else
	file = fopen("results/result.txt", "w+");
#endif
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
#ifdef DEBUG
			monotoneLocalReasoning_debug(nMachines, nTasks, file);
#else
			monotoneLocalReasoning(nMachines, nTasks, file);
#endif
		}
	}

	for (int nMachineIndex = 0; nMachineIndex < N_MACHINES_INPUTS; nMachineIndex++) {
		const int nMachines = nMachinesInputs[nMachineIndex];
		for (int rIndex = 0; rIndex < N_R_INPUTS; rIndex++) {
			const int nTasks = (int)pow(nMachines, rInputs[rIndex]);
			for (int alfaIndex = 0; alfaIndex < N_ALFA_INPUTS; alfaIndex++)
#ifdef DEBUG
				simulatedAnnealing_debug(nMachines, nTasks, alfaInputs[alfaIndex], file);
#else
				simulatedAnnealing(nMachines, nTasks, alfaInputs[alfaIndex], file);
#endif
		}
	}

	fclose(file);

	return 0;
}
