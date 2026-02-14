# Monotone Local Reasoning vs Non-Monotone Local Reasoning

In this project, we are comparing two types of algorithms to solve the task scheduling problem. One uses logic to reach the result, the other one, uses meta-heuristics.

## Index
* [How to run](#how-to-run)
* [Scope](#scope)
* [Monotone Local Reasoning](#monotone-local-reasoning)
* [Simulated Annealing](#simulated-annealing)
* [Output](#output)

## How to run

After cloning this repository, run the shell command bellow to compile:

```shell
gcc main.c machine.c stack.c -o mlr_vs_nmlr -lm
```

Then run the executable:
```shell
./mlr_vs_nmlr
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

The results will be written in a `.txt` file formatted as a CSV with the following header:

```
heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro
```

---

## Análise experimental

O projeto inclui instrumentação e scripts para análise comparativa MLR vs SA. Os resultados são gravados na pasta **`results/`**.

### Como gerar todos os CSVs

1. Rodar experimentos (ex.: `./run_experimentos.sh`). Isso gera `result.txt`, copia para `results/raw_result.txt` e chama o script de análise.
2. Ou rodar o binário manualmente e depois: `python3 analyze_results.py result.txt` (ou `python3 analyze_results.py results/raw_result.txt`).

### Reproducibilidade

Defina `MLR_SEED` para usar semente fixa:

```shell
MLR_SEED=42 ./mlr_vs_nmlr
```

### Arquivos em `results/`

| Arquivo | Descrição |
|--------|------------|
| `raw_result.txt` | Dados brutos (cópia de result.txt): heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro |
| **1_quality.csv** | Qualidade da solução: por (heuristica,n,m,alfa) → media_cmax, melhor_cmax, pior_cmax, desvio_padrao, tempo_medio, iteracoes_medias |
| **sa_acceptance.csv** | Taxa de aceitação de pioras (SA): por execução → total_movimentos, pioras_propostas, pioras_aceitas, taxa_aceitacao |
| **sa_convergence_\*\_\*\_\*\_\*.csv** | Convergência do SA por execução: iteracao, temperatura, makespan_atual, makespan_melhor, aceita, piora_aceita |
| **4_scalability.csv** | Escalabilidade: tempo e Cmax médios por (n,m,heuristica,alfa) e categorias tamanho_n/tamanho_m |
| **5_comparison_detail.csv** | Comparação MLR vs SA por réplica: diff_pct_cmax, diff_pct_tempo, winner_qualidade, winner_tempo |
| **5_comparison_summary.csv** | Resumo por (n,m,alfa): médias das diferenças % e contagem de vitórias MLR/SA |

### Gráficos comparativos

O script `generate_plots.py` gera visualizações em **`results/plots/`** a partir dos CSVs:

```shell
pip install matplotlib numpy   # dependências
python3 generate_plots.py
python3 generate_plots.py --instance 32,10   # filtrar por instância (n,m)
```

| Gráfico | Arquivo | Descrição |
|--------|--------|-----------|
| Boxplot Cmax | `01_boxplot_cmax_*.png` | Distribuição do Cmax por heurística (MLR vs SA) por instância |
| Convergência SA | `02_convergence_sa_*.png` | Iteração vs makespan atual/melhor e temperatura |
| Trade-off | `03_tradeoff_tempo_qualidade.png` | Tempo médio vs média do Cmax |
| Escalabilidade | `04_escalabilidade_*.png` | Tempo e Cmax vs número de tarefas (n) |
| Taxa aceitação pioras | `05_taxa_aceitacao_*.png` | % de pioras aceitas (SA) ao longo das iterações |
| Diferença % SA vs MLR | `06_diferenca_pct_*.png` | Bar chart: (MLR−SA)/MLR×100 por instância |
| Heatmap | `07_heatmap_diferenca_pct.png` | n×m: regiões onde cada algoritmo é melhor |
| Quality | `08_quality_*.png` | Média do Cmax por instância e heurística (1_quality) |
| Scalability bars | `09_scalability_bars.png` | Barras tempo/Cmax por instância (4_scalability) |

### Coleta de métricas no código (main.c)

- **Convergência SA**: em `run_one_sa_replication`, a cada iteração escreve-se em `sa_convergence_*.csv` (iteracao, temperatura, makespan atual/melhor, aceita, piora_aceita).
- **Taxa de aceitação de pioras**: em `run_one_sa_replication`, contadores `total_movimentos`, `pioras_propostas`, `pioras_aceitas`; ao final da replicação escreve-se uma linha em `sa_acceptance.csv`.
- **Qualidade, escalabilidade e comparação**: calculadas em `analyze_results.py` a partir de `raw_result.txt`.

> 09 fev 2026
