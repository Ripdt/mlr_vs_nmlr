# Monotone Local Reasoning vs Non-Monotone Local Reasoning

In this project, we are comparing two types of algorithms to solve the task scheduling problem. One uses logic to reach the result, the other one, uses meta-heuristics.

## How to run

After cloning this repository, run the shell command bellow to compile:

```shell
gcc main.c machine.c stack.c -o mnr_vs_nmlr -lm
```

Then run the executable:
```shell
./mnr_vs_nmlr
```

## Scope

The program has a series of inputs setting number of machines, tasks and alfas. It runs each combination of inputs 10 times to prevent outliers.

## Output

The results will be written in a `.txt` file formatted as a CSV with the following header:

```
heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
```

> 08 fev 2026
