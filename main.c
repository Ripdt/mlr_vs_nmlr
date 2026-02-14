#include <math.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

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
			accepted = 1;
		} else {
			const float acceptanceProbability = expf((float)(newMakespan - actualMakespan) / temperature);
			const float randomValue = (float)rand() / RAND_MAX;
			if (randomValue < acceptanceProbability) {
				machines = newSolution;
				criticMachine = newCriticMachine;
				destroySolution(oldSolution, nMachines);
				accepted = 1;
				piora_aceita = 1; /* Coleta: piora aceita */
				pioras_aceitas++;
			} else {
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

	if (convFile) fclose(convFile);

	/* Saída principal: heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro */
	fprintf(mainFile, "temperasimulada,%d,%d,%d,%.5f,%d,%d,%.2f\n",
		nTasks, nMachines, repNum, duration, it, makespan, alfa);

	/* Coleta análise 3: taxa de aceitação de pioras em sa_acceptance.csv */
	if (resultsDir) {
		char path[256];
		snprintf(path, sizeof(path), "%s/sa_acceptance.csv", resultsDir);
		FILE* accFile = fopen(path, "a");
		if (accFile) {
			double taxa = (pioras_propostas > 0) ? (100.0 * pioras_aceitas / pioras_propostas) : 0.0;
			fprintf(accFile, "temperasimulada,%d,%d,%.2f,%d,%d,%d,%d,%.2f\n",
				nTasks, nMachines, alfa, repNum, total_movimentos, pioras_propostas, pioras_aceitas, taxa);
			fclose(accFile);
		}
	}

	destroySolution(machines, nMachines);
}

void simulatedAnnealing(int nMachines, int nTasks, float alfa, int nRep, FILE* file, const char* resultsDir) {
	int execution = 0;
	while (execution++ != nRep) {
		run_one_sa_replication(nMachines, nTasks, alfa, execution, file, resultsDir);
	}
}

static int run_all(FILE* file) {
	const int nMachinesInputs[N_MACHINES_INPUTS] = {10, 20, 50};
	const float rInputs[N_R_INPUTS] = {1.5, 2.};
	const float alfaInputs[N_ALFA_INPUTS] = {.8, .85, .9, .95, .99};

	for (int nMachineIndex = 0; nMachineIndex < N_MACHINES_INPUTS; nMachineIndex++) {
		const int nMachines = nMachinesInputs[nMachineIndex];
		for (int rIndex = 0; rIndex < N_R_INPUTS; rIndex++) {
			const int nTasks = (int)pow(nMachines, rInputs[rIndex]);
			monotoneLocalReasoning(nMachines, nTasks, N_EXECUTIONS, file);
		}
	}

	for (int nMachineIndex = 0; nMachineIndex < N_MACHINES_INPUTS; nMachineIndex++) {
		const int nMachines = nMachinesInputs[nMachineIndex];
		for (int rIndex = 0; rIndex < N_R_INPUTS; rIndex++) {
			const int nTasks = (int)pow(nMachines, rInputs[rIndex]);
			for (int alfaIndex = 0; alfaIndex < N_ALFA_INPUTS; alfaIndex++)
				simulatedAnnealing(nMachines, nTasks, alfaInputs[alfaIndex], N_EXECUTIONS, file, RESULTS_DIR);
		}
	}
	return 0;
}

static void usage(const char* prog) {
	fprintf(stderr,
		"Uso:\n"
		"  %s                    Executa todas as combinações padrão (result.txt).\n"
		"  %s <m> <n> [rep]      Executa MLR e SA para m máquinas, n tarefas.\n"
		"                         rep = replicações (default: 10). SA usa todos os alfas.\n"
		"  %s <m> <n> <alfa> [rep]  Executa só SA para m, n e alfa.\n",
		prog, prog, prog);
}

/* Garante que results/ existe e que sa_acceptance.csv tem cabeçalho (para análise 3). */
static void ensure_results_dir_and_acceptance_header(void) {
#ifndef _WIN32
	mkdir(RESULTS_DIR, 0755);
#else
	_mkdir(RESULTS_DIR);
#endif
	{
		char path[256];
		snprintf(path, sizeof(path), "%s/sa_acceptance.csv", RESULTS_DIR);
		FILE* f = fopen(path, "r");
		if (!f) {
			f = fopen(path, "w");
			if (f) {
				fprintf(f, "heuristica,n,m,alfa,rep,total_movimentos,pioras_propostas,pioras_aceitas,taxa_aceitacao\n");
				fclose(f);
			}
		} else {
			fclose(f);
		}
	}
}

int main(int argc, char** argv) {
	/* Reproducibilidade: se MLR_SEED estiver definido, usa valor fixo; senão time(NULL) */
	const char* seedEnv = getenv("MLR_SEED");
	if (seedEnv && *seedEnv)
		srand((unsigned)atoi(seedEnv));
	else
		srand((unsigned)time(NULL));

	int nReplications = N_EXECUTIONS;
	int onlyM = -1, onlyN = -1;
	float onlyAlfa = -1.f;
	int append = 0;

	if (argc >= 4) {
		onlyM = atoi(argv[1]);
		onlyN = atoi(argv[2]);
		if (onlyM <= 0 || onlyN <= 0) {
			fprintf(stderr, "m e n devem ser inteiros positivos.\n");
			usage(argv[0]);
			return 1;
		}
		/* formato: ./prog m n rep  ou  ./prog m n alfa [rep] */
		if (strchr(argv[3], '.') != NULL) {
			onlyAlfa = (float)atof(argv[3]);
			if (argc >= 5) nReplications = atoi(argv[4]);
		} else {
			nReplications = atoi(argv[3]);
		}
	}

	const char* outPath = "result.txt";
	FILE* file;

	if (onlyM > 0 && onlyN > 0) {
		append = 1;
		file = fopen(outPath, "a");
	} else {
		file = fopen(outPath, "w");
	}

	if (!file) {
		fprintf(stderr, "Failed to open file %s\n", outPath);
		return 1;
	}

	if (!append)
		fprintf(file, "heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro\n");

	/* Garantir pasta results/ e cabeçalho de sa_acceptance antes de rodar SA */
	ensure_results_dir_and_acceptance_header();

	if (onlyM > 0 && onlyN > 0) {
		if (onlyAlfa >= 0.f) {
			int execution = 0;
			while (execution++ != nReplications) {
				run_one_sa_replication(onlyM, onlyN, onlyAlfa, execution, file, RESULTS_DIR);
			}
		} else {
			int execution = 0;
			while (execution++ != nReplications) {
				Machine* machines = createSolution(onlyM, onlyN);
				int criticIndex = 0;
				Machine* criticMachine = &machines[criticIndex];
				for (int taskIndex = 0; taskIndex < onlyN; taskIndex++)
					pushTask(criticMachine, rand() % TASK_FACTOR + 1);
				int makespan = criticMachine->makespan;
				int it = 0;
				const clock_t start = clock();
				do {
					int targetIndex = criticIndex + 1;
					if (criticIndex == onlyM - 1) targetIndex = 0;
					Machine* targetMachine = &machines[targetIndex];
					int task = peekTask(criticMachine);
					const int actualMakespan = criticMachine->makespan;
					const int targetMakespan = targetMachine->makespan + task;
					if (targetMakespan > actualMakespan) {
						makespan = actualMakespan;
						criticMachine = NULL;
					} else {
						popTask(criticMachine);
						pushTask(targetMachine, task);
						++it;
						if (targetMachine->makespan > criticMachine->makespan) {
							criticMachine = targetMachine;
							criticIndex = targetIndex;
						} else if (targetMachine->makespan == criticMachine->makespan) {
							criticMachine = NULL;
						}
					}
				} while (criticMachine);
				const clock_t stop = clock();
				const double duration = (double)(stop - start) / CLOCKS_PER_SEC;
				fprintf(file, "monotona,%d,%d,%d,%.5f,%d,%d,NA\n",
					onlyN, onlyM, execution, duration, it, makespan);
				destroySolution(machines, onlyM);
			}
			const float alfaInputs[N_ALFA_INPUTS] = {.8f, .85f, .9f, .95f, .99f};
			for (int alfaIndex = 0; alfaIndex < N_ALFA_INPUTS; alfaIndex++)
				simulatedAnnealing(onlyM, onlyN, alfaInputs[alfaIndex], nReplications, file, RESULTS_DIR);
		}
	} else {
		run_all(file);
	}

	fclose(file);
	return 0;
}
