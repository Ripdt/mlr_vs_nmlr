#!/usr/bin/env python3
"""
Gera gráficos comparativos e analíticos a partir dos CSVs em results/.
Salva em results/plots/. Código modular; permite filtrar por instância (--instance n,m).

Uso:
  python generate_plots.py
  python generate_plots.py --instance 32,10
"""

import os
import sys
import argparse
import csv
import glob
from collections import defaultdict

# Dependências opcionais
try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("Instale matplotlib e numpy: pip install matplotlib numpy")
    sys.exit(1)

try:
    import pandas as pd
except ImportError:
    pd = None

RESULTS_DIR = "results"
PLOTS_DIR = os.path.join(RESULTS_DIR, "plots")


def ensure_plots_dir():
    os.makedirs(PLOTS_DIR, exist_ok=True)


def load_raw(path=None):
    p = path or os.path.join(RESULTS_DIR, "raw_result.txt")
    if not os.path.isfile(p):
        return None
    rows = []
    with open(p, newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            row["n"] = int(row["n"])
            row["m"] = int(row["m"])
            row["replicacao"] = int(row["replicacao"])
            row["tempo"] = float(row["tempo"])
            row["iteracoes"] = int(row["iteracoes"])
            row["valor"] = int(row["valor"])
            row["parametro"] = row["parametro"].strip()
            rows.append(row)
    return rows


def load_quality(path=None):
    p = path or os.path.join(RESULTS_DIR, "1_quality.csv")
    if not os.path.isfile(p):
        return None
    rows = []
    with open(p, newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            row["n"] = int(row["n"])
            row["m"] = int(row["m"])
            for k in ("media_cmax", "melhor_cmax", "pior_cmax", "desvio_padrao", "tempo_medio", "iteracoes_medias"):
                try:
                    row[k] = float(row[k])
                except (ValueError, KeyError):
                    pass
            row["alfa"] = row.get("alfa", "").strip() or None
            rows.append(row)
    return rows


def load_scalability(path=None):
    p = path or os.path.join(RESULTS_DIR, "4_scalability.csv")
    if not os.path.isfile(p):
        return None
    rows = []
    with open(p, newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            row["n"] = int(row["n"])
            row["m"] = int(row["m"])
            try:
                row["tempo_medio"] = float(row["tempo_medio"])
                row["media_cmax"] = float(row["media_cmax"])
            except (ValueError, KeyError):
                pass
            rows.append(row)
    return rows


def load_comparison_summary(path=None):
    p = path or os.path.join(RESULTS_DIR, "5_comparison_summary.csv")
    if not os.path.isfile(p):
        return None
    rows = []
    with open(p, newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            row["n"] = int(row["n"])
            row["m"] = int(row["m"])
            row["alfa"] = float(row["alfa"])
            row["media_diff_pct_cmax"] = float(row["media_diff_pct_cmax"])
            row["media_diff_pct_tempo"] = float(row["media_diff_pct_tempo"])
            rows.append(row)
    return rows


def load_sa_acceptance(path=None):
    p = path or os.path.join(RESULTS_DIR, "sa_acceptance.csv")
    if not os.path.isfile(p):
        return None
    rows = []
    with open(p, newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            row["n"] = int(row["n"])
            row["m"] = int(row["m"])
            row["alfa"] = float(row["alfa"])
            row["rep"] = int(row["rep"])
            row["total_movimentos"] = int(row["total_movimentos"])
            row["pioras_propostas"] = int(row["pioras_propostas"])
            row["pioras_aceitas"] = int(row["pioras_aceitas"])
            row["taxa_aceitacao"] = float(row["taxa_aceitacao"])
            rows.append(row)
    return rows


def list_convergence_files(base_dir=None):
    base_dir = base_dir or RESULTS_DIR
    pattern = os.path.join(base_dir, "sa_convergence_*.csv")
    return sorted(glob.glob(pattern))


def load_convergence(filepath):
    rows = []
    with open(filepath, newline="", encoding="utf-8") as f:
        r = csv.DictReader(f)
        for row in r:
            row["iteracao"] = int(row["iteracao"])
            row["temperatura"] = float(row["temperatura"])
            row["makespan_atual"] = int(row["makespan_atual"])
            row["makespan_melhor"] = int(row["makespan_melhor"])
            row["aceita"] = int(row["aceita"])
            row["piora_aceita"] = int(row["piora_aceita"])
            rows.append(row)
    return rows


def parse_instance_filter(s):
    if not s:
        return None
    try:
        n, m = map(int, s.strip().split(","))
        return (n, m)
    except Exception:
        return None


def filter_instance(rows, key_n, key_m, instance):
    if not instance or not rows:
        return rows
    n0, m0 = instance
    return [r for r in rows if r.get(key_n) == n0 and r.get(key_m) == m0]


# ---------- 1. Boxplot Cmax: MLR vs SA por instância ----------
def plot_boxplot_cmax(raw_rows, instance_filter, plots_dir):
    if not raw_rows:
        return
    raw_rows = filter_instance(raw_rows, "n", "m", instance_filter)
    if not raw_rows:
        return

    # Agrupar por (n, m)
    by_instance = defaultdict(list)
    for r in raw_rows:
        by_instance[(r["n"], r["m"])].append(r)

    for (n, m), rows in sorted(by_instance.items()):
        mlr_vals = [r["valor"] for r in rows if r["heuristica"] == "monotona"]
        sa_vals = [r["valor"] for r in rows if r["heuristica"] == "temperasimulada"]
        if not mlr_vals and not sa_vals:
            continue

        fig, ax = plt.subplots(figsize=(6, 5))
        data = []
        labels = []
        if mlr_vals:
            data.append(mlr_vals)
            labels.append("MLR")
        if sa_vals:
            data.append(sa_vals)
            labels.append("SA")
        bp = ax.boxplot(data, tick_labels=labels, patch_artist=True)
        for patch in bp["boxes"]:
            patch.set_facecolor("lightblue")
        ax.set_ylabel("Cmax (makespan final)")
        ax.set_xlabel("Heurística")
        ax.set_title(f"Distribuição do Cmax por heurística — instância n={n}, m={m}")
        plt.tight_layout()
        out = os.path.join(plots_dir, f"01_boxplot_cmax_n{n}_m{m}.png")
        plt.savefig(out, dpi=150, bbox_inches="tight")
        plt.close()
        print("Salvo:", out)

    # Um único gráfico com todos os (n,m) em subplots
    instances = sorted(by_instance.keys())
    if len(instances) > 1:
        ncols = min(3, len(instances))
        nrows = (len(instances) + ncols - 1) // ncols
        fig, axes = plt.subplots(nrows, ncols, figsize=(5 * ncols, 4 * nrows))
        if nrows == 1 and ncols == 1:
            axes = [[axes]]
        elif nrows == 1:
            axes = [axes]
        for idx, (n, m) in enumerate(instances):
            row, col = idx // ncols, idx % ncols
            ax = axes[row][col]
            rows = by_instance[(n, m)]
            mlr_vals = [r["valor"] for r in rows if r["heuristica"] == "monotona"]
            sa_vals = [r["valor"] for r in rows if r["heuristica"] == "temperasimulada"]
            data, labels = [], []
            if mlr_vals:
                data.append(mlr_vals)
                labels.append("MLR")
            if sa_vals:
                data.append(sa_vals)
                labels.append("SA")
            ax.boxplot(data, tick_labels=labels, patch_artist=True)
            ax.set_ylabel("Cmax")
            ax.set_title(f"n={n}, m={m}")
        for idx in range(len(instances), nrows * ncols):
            row, col = idx // ncols, idx % ncols
            axes[row][col].set_visible(False)
        fig.suptitle("Comparação de qualidade (Boxplot Cmax) — MLR vs SA por instância", y=1.02)
        plt.tight_layout()
        out = os.path.join(plots_dir, "01_boxplot_cmax_all.png")
        plt.savefig(out, dpi=150, bbox_inches="tight")
        plt.close()
        print("Salvo:", out)


# ---------- 2. Convergência SA: iteração vs makespan atual/melhor; temperatura vs iteração ----------
def plot_convergence_sa(instance_filter, plots_dir, results_dir=None):
    files = list_convergence_files(results_dir)
    if not files:
        return

    # Agrupar por (n, m)
    by_key = defaultdict(list)
    for fp in files:
        base = os.path.basename(fp)
        # sa_convergence_32_10_0.90_1.csv
        parts = base.replace("sa_convergence_", "").replace(".csv", "").split("_")
        if len(parts) >= 4:
            n, m = int(parts[0]), int(parts[1])
            if instance_filter and (n, m) != instance_filter:
                continue
            by_key[(n, m)].append(fp)

    for (n, m), paths in sorted(by_key.items()):
        # Escolher um arquivo por (n,m): primeiro alfa 0.9, rep 1 se existir
        chosen = None
        for p in paths:
            if "0.90_1.csv" in p:
                chosen = p
                break
        if not chosen:
            chosen = paths[0]
        rows = load_convergence(chosen)
        if not rows:
            continue

        iters = [r["iteracao"] for r in rows]
        atual = [r["makespan_atual"] for r in rows]
        melhor = [r["makespan_melhor"] for r in rows]
        temp = [r["temperatura"] for r in rows]

        fig, axes = plt.subplots(3, 1, figsize=(8, 10), sharex=True)
        axes[0].plot(iters, atual, color="tab:blue", label="Makespan atual", alpha=0.8)
        axes[0].set_ylabel("Makespan atual")
        axes[0].set_title("Simulated Annealing: convergência (exemplo)")
        axes[0].legend(loc="upper right")
        axes[0].grid(True, alpha=0.3)

        axes[1].plot(iters, melhor, color="tab:green", label="Melhor makespan")
        axes[1].set_ylabel("Melhor makespan")
        axes[1].legend(loc="upper right")
        axes[1].grid(True, alpha=0.3)

        axes[2].plot(iters, temp, color="tab:orange", label="Temperatura")
        axes[2].set_xlabel("Iteração")
        axes[2].set_ylabel("Temperatura")
        axes[2].legend(loc="upper right")
        axes[2].grid(True, alpha=0.3)

        fig.suptitle(f"Convergência SA — n={n}, m={m} (ex.: alfa=0.9, rep=1)", y=1.02)
        plt.tight_layout()
        out = os.path.join(plots_dir, f"02_convergence_sa_n{n}_m{m}.png")
        plt.savefig(out, dpi=150, bbox_inches="tight")
        plt.close()
        print("Salvo:", out)


# ---------- 3. Trade-off Tempo vs Qualidade (scatter) ----------
def plot_tradeoff_time_quality(quality_rows, instance_filter, plots_dir):
    if not quality_rows:
        return
    quality_rows = filter_instance(quality_rows, "n", "m", instance_filter)
    if not quality_rows:
        return

    mlr = [(r["tempo_medio"], r["media_cmax"]) for r in quality_rows if r["heuristica"] == "monotona"]
    sa = [(r["tempo_medio"], r["media_cmax"]) for r in quality_rows if r["heuristica"] == "temperasimulada"]
    if not mlr and not sa:
        return

    fig, ax = plt.subplots(figsize=(8, 6))
    if mlr:
        tx, ty = zip(*mlr)
        ax.scatter(tx, ty, label="MLR", color="tab:blue", s=80, alpha=0.8, edgecolors="black")
    if sa:
        tx, ty = zip(*sa)
        ax.scatter(tx, ty, label="SA", color="tab:orange", s=80, alpha=0.8, edgecolors="black")
    ax.set_xlabel("Tempo médio (s)")
    ax.set_ylabel("Média do Cmax")
    ax.set_title("Trade-off Tempo vs Qualidade (um ponto por heurística por instância/alfa)")
    ax.legend()
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    out = os.path.join(plots_dir, "03_tradeoff_tempo_qualidade.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print("Salvo:", out)


# ---------- 4. Escalabilidade: Tempo vs n e Cmax vs n ----------
def plot_scalability(scalability_rows, instance_filter, plots_dir):
    if not scalability_rows:
        return
    scalability_rows = filter_instance(scalability_rows, "n", "m", instance_filter)
    if not scalability_rows:
        return

    mlr = [r for r in scalability_rows if r["heuristica"] == "monotona"]
    sa = [r for r in scalability_rows if r["heuristica"] == "temperasimulada"]
    if not mlr and not sa:
        return

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
    ns = sorted(set(r["n"] for r in scalability_rows))
    for heur, label, color in [("monotona", "MLR", "tab:blue"), ("temperasimulada", "SA", "tab:orange")]:
        sub = [r for r in scalability_rows if r["heuristica"] == heur]
        if not sub:
            continue
        by_n = defaultdict(list)
        for r in sub:
            by_n[r["n"]].append(r)
        x = []
        t_med = []
        c_med = []
        for n in ns:
            if n not in by_n:
                continue
            vals = by_n[n]
            x.append(n)
            t_med.append(sum(r["tempo_medio"] for r in vals) / len(vals))
            c_med.append(sum(r["media_cmax"] for r in vals) / len(vals))
        if x:
            ax1.plot(x, t_med, "o-", label=label, color=color)
            ax2.plot(x, c_med, "o-", label=label, color=color)
    ax1.set_xlabel("Número de tarefas (n)")
    ax1.set_ylabel("Tempo médio (s)")
    ax1.set_title("Escalabilidade: Tempo vs n")
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax2.set_xlabel("Número de tarefas (n)")
    ax2.set_ylabel("Cmax médio")
    ax2.set_title("Escalabilidade: Cmax vs n")
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    fig.suptitle("Escalabilidade por heurística", y=1.02)
    plt.tight_layout()
    out = os.path.join(plots_dir, "04_escalabilidade_tempo_cmax_vs_n.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print("Salvo:", out)


# ---------- 5. Taxa de aceitação de pioras (SA): a partir de um arquivo de convergência ----------
def plot_acceptance_rate_worse(instance_filter, plots_dir, results_dir=None):
    files = list_convergence_files(results_dir)
    if not files:
        return
    chosen = None
    for fp in files:
        base = os.path.basename(fp)
        parts = base.replace("sa_convergence_", "").replace(".csv", "").split("_")
        if len(parts) >= 4:
            n, m = int(parts[0]), int(parts[1])
            if instance_filter and (n, m) != instance_filter:
                continue
            chosen = fp
            break
    if not chosen:
        chosen = files[0]
    rows = load_convergence(chosen)
    if not rows:
        return

    iters = [r["iteracao"] for r in rows]
    temp = [r["temperatura"] for r in rows]
    piora_aceita = [r["piora_aceita"] for r in rows]
    cum_aceitas = np.cumsum(piora_aceita)
    running_rate = cum_aceitas / (np.arange(len(piora_aceita)) + 1) * 100  # % acumulado de pioras aceitas

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 8))
    ax1.plot(iters, running_rate, color="tab:purple", label="% pioras aceitas (acumulado)")
    ax1.set_xlabel("Iteração")
    ax1.set_ylabel("% pioras aceitas (acumulado)")
    ax1.set_title("Taxa de aceitação de pioras ao longo das iterações (SA)")
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    ax2.plot(temp, running_rate, color="tab:red", alpha=0.7)
    ax2.set_xlabel("Temperatura")
    ax2.set_ylabel("% pioras aceitas (acumulado)")
    ax2.set_title("Taxa de aceitação de pioras vs Temperatura (SA)")
    ax2.grid(True, alpha=0.3)
    plt.tight_layout()
    base = os.path.basename(chosen).replace(".csv", "")
    out = os.path.join(plots_dir, f"05_taxa_aceitacao_pioras_{base}.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print("Salvo:", out)


# ---------- 6. Bar chart: diferença percentual SA vs MLR ----------
def plot_diff_pct_bars(summary_rows, instance_filter, plots_dir):
    if not summary_rows:
        return
    summary_rows = filter_instance(summary_rows, "n", "m", instance_filter)
    if not summary_rows:
        return

    # (MLR - SA) / MLR * 100 => positivo = MLR melhor. Nosso media_diff_pct_cmax = (SA-MLR)/MLR*100
    # então diff_qualidade = -media_diff_pct_cmax (positivo = MLR melhor)
    by_instance = defaultdict(list)
    for r in summary_rows:
        by_instance[(r["n"], r["m"])].append(r["media_diff_pct_cmax"])
    instances = sorted(by_instance.keys())
    if not instances:
        return
    labels = [f"n={n}\nm={m}" for n, m in instances]
    # Média sobre alfas da diferença (SA-MLR)/MLR*100; depois invertemos para (MLR-SA)/MLR*100
    means = [-np.mean(by_instance[k]) for k in instances]
    colors = ["tab:green" if x > 0 else "tab:red" for x in means]  # verde MLR melhor, vermelho SA melhor
    x_pos = np.arange(len(labels))
    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.2), 5))
    bars = ax.bar(x_pos, means, color=colors, edgecolor="black")
    ax.axhline(0, color="black", linewidth=0.8)
    ax.set_xticks(x_pos)
    ax.set_xticklabels(labels)
    ax.set_ylabel("Diferença percentual (MLR - SA) / MLR × 100")
    ax.set_title("Diferença percentual de qualidade: positivo = MLR melhor, negativo = SA melhor")
    plt.tight_layout()
    out = os.path.join(plots_dir, "06_diferenca_pct_sa_vs_mlr.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print("Salvo:", out)


# ---------- 7. Heatmap: n x m, valor = diferença % SA vs MLR ----------
def plot_heatmap_diff(summary_rows, instance_filter, plots_dir):
    if not summary_rows:
        return
    summary_rows = filter_instance(summary_rows, "n", "m", instance_filter)
    if not summary_rows:
        return

    by_nm = defaultdict(list)
    for r in summary_rows:
        by_nm[(r["n"], r["m"])].append(r["media_diff_pct_cmax"])
    # Média sobre alfas; usar (MLR-SA)/MLR*100 = -media_diff_pct_cmax
    matrix = {}
    for (n, m), vals in by_nm.items():
        matrix[(n, m)] = -np.mean(vals)
    ns = sorted(set(n for n, m in matrix))
    ms = sorted(set(m for n, m in matrix))
    if not ns or not ms:
        return
    M = np.full((len(ns), len(ms)), np.nan)
    for i, n in enumerate(ns):
        for j, m in enumerate(ms):
            M[i, j] = matrix.get((n, m), np.nan)
    fig, ax = plt.subplots(figsize=(max(5, len(ms) * 0.8), max(4, len(ns) * 0.5)))
    im = ax.imshow(M, aspect="auto", cmap="RdYlGn", vmin=-80, vmax=20)
    ax.set_xticks(np.arange(len(ms)))
    ax.set_yticks(np.arange(len(ns)))
    ax.set_xticklabels(ms)
    ax.set_yticklabels(ns)
    ax.set_xlabel("m (máquinas)")
    ax.set_ylabel("n (tarefas)")
    ax.set_title("Diferença percentual (MLR - SA) / MLR × 100\n(verde = MLR melhor, vermelho = SA melhor)")
    plt.colorbar(im, ax=ax, label="(MLR - SA) / MLR × 100")
    plt.tight_layout()
    out = os.path.join(plots_dir, "07_heatmap_diferenca_pct.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print("Salvo:", out)


# ---------- Quality: barras media/melhor/pior Cmax por instância e heurística ----------
def plot_quality_bars(quality_rows, instance_filter, plots_dir):
    if not quality_rows:
        return
    quality_rows = filter_instance(quality_rows, "n", "m", instance_filter)
    if not quality_rows:
        return

    mlr = [r for r in quality_rows if r["heuristica"] == "monotona"]
    sa = [r for r in quality_rows if r["heuristica"] == "temperasimulada"]
    # Para cada (n,m) um grupo de barras: MLR (media, melhor, pior) e SA (média sobre alfas)
    by_inst_heur = defaultdict(list)
    for r in quality_rows:
        by_inst_heur[(r["n"], r["m"]), r["heuristica"]].append(r)

    instances = sorted(set(k[0] for k in by_inst_heur))
    x = np.arange(len(instances))
    width = 0.2
    fig, ax = plt.subplots(figsize=(max(8, len(instances) * 1.5), 5))
    for i, (n, m) in enumerate(instances):
        for j, (heur, label) in enumerate([("monotona", "MLR"), ("temperasimulada", "SA")]):
            rows = by_inst_heur.get(((n, m), heur), [])
            if not rows:
                continue
            med = np.mean([r["media_cmax"] for r in rows])
            mn = np.mean([r["melhor_cmax"] for r in rows])
            mx = np.mean([r["pior_cmax"] for r in rows])
            off = (j - 0.5) * width
            ax.bar(x[i] + off, med, width, label=label if i == 0 else "", color="tab:blue" if heur == "monotona" else "tab:orange")
    ax.set_xticks(x)
    ax.set_xticklabels([f"n={n}\nm={m}" for n, m in instances])
    ax.set_ylabel("Cmax")
    ax.set_title("Qualidade: média do Cmax por instância e heurística (1_quality)")
    ax.legend()
    plt.tight_layout()
    out = os.path.join(plots_dir, "08_quality_media_cmax_por_instancia.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print("Salvo:", out)


# ---------- Scalability: tempo e Cmax por instância (usando 4_scalability) ----------
def plot_scalability_bars(scalability_rows, instance_filter, plots_dir):
    if not scalability_rows:
        return
    scalability_rows = filter_instance(scalability_rows, "n", "m", instance_filter)
    if not scalability_rows:
        return

    by_heur = defaultdict(list)
    for r in scalability_rows:
        by_heur[r["heuristica"]].append(r)
    keys = sorted(set((r["n"], r["m"]) for rows in by_heur.values() for r in rows))
    if not keys:
        return
    labels_nm = [f"n={n}\nm={m}" for n, m in keys]
    n_inst = len(keys)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
    width = 0.35
    x = np.arange(n_inst)
    for heur, label, color in [("monotona", "MLR", "tab:blue"), ("temperasimulada", "SA", "tab:orange")]:
        rows = by_heur.get(heur, [])
        if not rows:
            continue
        by_nm = defaultdict(list)
        for r in rows:
            by_nm[(r["n"], r["m"])].append(r)
        t_med = [np.mean([x["tempo_medio"] for x in by_nm[k]]) for k in keys]
        c_med = [np.mean([x["media_cmax"] for x in by_nm[k]]) for k in keys]
        off = width / 2 if heur == "temperasimulada" else -width / 2
        ax1.bar(x + off, t_med, width, label=label, color=color)
        ax2.bar(x + off, c_med, width, label=label, color=color)
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels_nm)
    ax1.set_ylabel("Tempo médio (s)")
    ax1.set_title("Escalabilidade: tempo por instância (4_scalability)")
    ax1.legend()
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels_nm)
    ax2.set_ylabel("Cmax médio")
    ax2.set_title("Escalabilidade: Cmax por instância (4_scalability)")
    ax2.legend()
    plt.tight_layout()
    out = os.path.join(plots_dir, "09_scalability_bars.png")
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print("Salvo:", out)


def main():
    parser = argparse.ArgumentParser(description="Gera gráficos a partir dos CSVs em results/")
    parser.add_argument("--instance", type=str, default=None, help="Filtrar por instância: n,m (ex: 32,10)")
    parser.add_argument("--results-dir", type=str, default=RESULTS_DIR, help="Pasta com os CSVs")
    args = parser.parse_args()
    results_dir = args.results_dir
    plots_dir = os.path.join(results_dir, "plots")

    instance = parse_instance_filter(args.instance)
    os.makedirs(plots_dir, exist_ok=True)

    raw = load_raw(os.path.join(results_dir, "raw_result.txt"))
    quality = load_quality(os.path.join(results_dir, "1_quality.csv"))
    scalability = load_scalability(os.path.join(results_dir, "4_scalability.csv"))
    summary = load_comparison_summary(os.path.join(results_dir, "5_comparison_summary.csv"))

    if not raw and not quality:
        print("Nenhum dado encontrado em", results_dir, "(raw_result.txt ou 1_quality.csv).")
        sys.exit(1)

    plot_boxplot_cmax(raw, instance, plots_dir)
    plot_convergence_sa(instance, plots_dir, results_dir)
    plot_tradeoff_time_quality(quality, instance, plots_dir)
    plot_scalability(scalability, instance, plots_dir)
    plot_acceptance_rate_worse(instance, plots_dir, results_dir)
    plot_diff_pct_bars(summary, instance, plots_dir)
    plot_heatmap_diff(summary, instance, plots_dir)
    plot_quality_bars(quality, instance, plots_dir)
    plot_scalability_bars(scalability, instance, plots_dir)

    print("Gráficos salvos em:", plots_dir)


if __name__ == "__main__":
    main()
