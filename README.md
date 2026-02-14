# Monotone Local Reasoning vs Non-Monotone Local Reasoning

In this project, we are comparing two types of algorithms to solve the task scheduling problem. One uses logic to reach the result, the other one, uses meta-heuristics.

## Index
* [How to run](#how-to-run)
* [Scope](#scope)
* [Monotone Local Reasoning](#monotone-local-reasoning)
* [Simulated Annealing](#simulated-annealing)
* [Output](#output)

## How to run

### To compare Monotone vs Non-Monotone

After cloning this repository, run the shell command bellow to compile:

```shell
gcc main.c machine.c stack.c -o mlr_vs_nmlr -lm
```

Then run the executable:
```shell
./mlr_vs_nmlr
```

### To get data by iteration (slower)

After cloning this repository, run the shell command bellow to compile:

```shell
gcc main.c machine.c stack.c -DDEBUG -o mlr_vs_nmlr_db -lm
```

Then run the executable:
```shell
./mlr_vs_nmlr_db
```

## Scope

The program has a series of inputs setting number of machines, tasks and alfas. It runs each combination of inputs 10 times to prevent outliers.

## Monotone Local Reasoning

```
Get machine with higher makespan
If popping the higher task to neighbour machine lowers the makespan:
    Pops higher task to neighbour machine
Else:
    Stop algorithm
```

## Simulated Annealing

```
Initialize all tasks on first machine
Set temperature = 10000
Get current makespan from critic machine
Repeat while temperature > 0.1:
    Save reference to old solution
    Copy current solution (newSolution)
    Find critic machine in newSolution
    Compute disturbance level q(S) = (Cmax - μ) / μ where μ = (1/n) * Σ Lj
    Apply disturbance: move q(S) * qtdTasks from critic to random machines
    Find critic machine in newSolution
    Get newMakespan from newSolution's critic machine
    
    If newMakespan <= currentMakespan:
        Accept new solution
        Destroy old solution
    Else:
        Calculate acceptanceProbability = exp((newMakespan - currentMakespan) / temperature)
        Generate randomValue between 0 and 1
        If randomValue < acceptanceProbability:
            Accept new solution (worse solution accepted)
            Destroy old solution
        Else:
            Reject new solution
            Destroy new solution
    
    Update current makespan from critic machine
    Reduce temperature by multiplying with alfa
    Increment iteration counter
```

**Disturbance Level Formula:**

$$\mu = \frac{1}{m} \sum_{j=1}^{m} L_j$$

$$q(S) = \frac{C_{\max} - \mu}{\mu}$$

Where:
- $m$ = number of machines
- $L_j$ = makespan (load) of machine $j$
- $C_{\max}$ = makespan of critic machine (maximum makespan)
- $q(S)$ = disturbance level

**Disturbance Application:**

The disturbance is applied by calculating the number of tasks to move as $\lfloor q(S) \times \text{qtdTasks}_{\text{critic}} \rfloor$ (minimum 1 task, maximum all tasks). These tasks are popped from the critic machine and pushed to randomly selected machines (excluding the critic machine itself).

## Output

The results will be written in the `results/result.txt` file formatted as a CSV with the following header:

```
heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
```

If the program ran in debug mode, the `results` directory will have a file to each iteration of each configuration. Its general results will be printed in the `results/result_db.txt` formatted as a CSV with the header above (the `time` column must not be considered in this because the program spends time writing the extra data).
The data is going to be in files with this name format:

```
tempera_10m_31t_85a_2x.txt
```

Which, in this example, means that this file has the data of the second iteration (`2x`) using the config of 10 machines (`10m`), 31 tasks (`31t`) and 0.85 as its alfa (`85a`; the monotone approach does not have alfa constant, so its data files does not have this in its name).

> 09 fev 2026
