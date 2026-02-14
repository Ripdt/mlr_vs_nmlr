#!/usr/bin/env python3
"""
Análise experimental comparativa MLR vs SA.
Lê result.txt (ou results/raw_result.txt) e results/sa_acceptance.csv,
gera CSVs em results/ para qualidade, escalabilidade e comparação direta.

Uso: python analyze_results.py [caminho_para_result.txt]
     Se não passar caminho, usa result.txt ou results/raw_result.txt.
"""

import os
import sys
import csv
from collections import defaultdict
import statistics

RESULTS_DIR = "results"
RAW_DEFAULT = "result.txt"
RAW_IN_RESULTS = os.path.join(RESULTS_DIR, "raw_result.txt")

# Limites para categorias de escalabilidade (n e m)
N_PEQUENO, N_MEDIO = 200, 1000   # n < 200 pequeno, n < 1000 medio, senao grande
M_PEQUENO, M_MEDIO = 20, 40      # m < 20 pequeno, m < 40 medio, senao grande


def ensure_results_dir():
    os.makedirs(RESULTS_DIR, exist_ok=True)


def find_raw_result(user_path=None):
    if user_path and os.path.isfile(user_path):
        return user_path
    if os.path.isfile(RAW_DEFAULT):
        return RAW_DEFAULT
    if os.path.isfile(RAW_IN_RESULTS):
        return RAW_IN_RESULTS
    return None


def load_raw_data(path):
    """Carrega result.txt: heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro"""
    rows = []
    with open(path, newline="", encoding="utf-8") as f:
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


def analysis_1_quality(rows, out_dir):
    """
    Análise 1: Qualidade da solução (Cmax).
    Agrupa por heuristica, n, m, alfa (NA para MLR); calcula média, melhor, pior, desvio, tempo médio, iterações médias.
    """
    key_fn = lambda r: (r["heuristica"], r["n"], r["m"], r["parametro"])
    groups = defaultdict(list)
    for r in rows:
        groups[key_fn(r)].append(r)

    out_path = os.path.join(out_dir, "1_quality.csv")
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["heuristica", "n", "m", "alfa", "media_cmax", "melhor_cmax", "pior_cmax",
                    "desvio_padrao", "tempo_medio", "iteracoes_medias"])
        for (heur, n, m, alfa), replist in sorted(groups.items()):
            vals = [x["valor"] for x in replist]
            tempos = [x["tempo"] for x in replist]
            iters = [x["iteracoes"] for x in replist]
            media_cmax = statistics.mean(vals)
            melhor = min(vals)
            pior = max(vals)
            desvio = statistics.stdev(vals) if len(vals) > 1 else 0.0
            tempo_medio = statistics.mean(tempos)
            iter_medio = statistics.mean(iters)
            alfa_out = alfa if alfa != "NA" else ""
            w.writerow([heur, n, m, alfa_out, round(media_cmax, 4), melhor, pior,
                        round(desvio, 4), round(tempo_medio, 6), round(iter_medio, 2)])
    print("Escrito:", out_path)
    return out_path


def tamanho_n(n):
    if n < N_PEQUENO:
        return "pequeno"
    if n < N_MEDIO:
        return "medio"
    return "grande"


def tamanho_m(m):
    if m < M_PEQUENO:
        return "pequeno"
    if m < M_MEDIO:
        return "medio"
    return "grande"


def analysis_4_scalability(rows, out_dir):
    """
    Análise 4: Escalabilidade.
    Tabela com crescimento de tempo e Cmax por (n, m, heuristica, alfa) e categorias tamanho_n, tamanho_m.
    """
    key_fn = lambda r: (r["n"], r["m"], r["heuristica"], r["parametro"])
    groups = defaultdict(list)
    for r in rows:
        groups[key_fn(r)].append(r)

    out_path = os.path.join(out_dir, "4_scalability.csv")
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["n", "m", "heuristica", "alfa", "tempo_medio", "media_cmax",
                    "tamanho_n", "tamanho_m"])
        for (n, m, heur, alfa), replist in sorted(groups.items()):
            vals = [x["valor"] for x in replist]
            tempos = [x["tempo"] for x in replist]
            alfa_out = alfa if alfa != "NA" else ""
            w.writerow([n, m, heur, alfa_out,
                        round(statistics.mean(tempos), 6),
                        round(statistics.mean(vals), 4),
                        tamanho_n(n), tamanho_m(m)])
    print("Escrito:", out_path)
    return out_path


def analysis_5_comparison(rows, out_dir):
    """
    Análise 5: Comparação direta MLR vs SA.
    Para cada (n, m, alfa, replicacao) junta MLR e SA; calcula diferença % qualidade e tempo; quem venceu.
    Gera também um resumo por (n, m, alfa).
    """
    mlr = [r for r in rows if r["heuristica"] == "monotona"]
    sa = [r for r in rows if r["heuristica"] == "temperasimulada"]

    # Chave por (n, m, replicacao) para MLR
    mlr_by = {(r["n"], r["m"], r["replicacao"]): r for r in mlr}

    # Para cada execução SA, comparar com a mesma replicação MLR
    detail_path = os.path.join(out_dir, "5_comparison_detail.csv")
    with open(detail_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["n", "m", "alfa", "rep", "mlr_cmax", "sa_cmax", "mlr_tempo", "sa_tempo",
                    "diff_pct_cmax", "diff_pct_tempo", "winner_qualidade", "winner_tempo"])
        for r in sa:
            key = (r["n"], r["m"], r["replicacao"])
            mlr_row = mlr_by.get(key)
            if not mlr_row:
                continue
            mlr_cmax = mlr_row["valor"]
            sa_cmax = r["valor"]
            mlr_tempo = mlr_row["tempo"]
            sa_tempo = r["tempo"]
            diff_pct_cmax = (sa_cmax - mlr_cmax) / mlr_cmax * 100.0 if mlr_cmax else 0.0
            diff_pct_tempo = (sa_tempo - mlr_tempo) / mlr_tempo * 100.0 if mlr_tempo else 0.0
            winner_q = "MLR" if mlr_cmax <= sa_cmax else "SA"
            winner_t = "MLR" if mlr_tempo <= sa_tempo else "SA"
            w.writerow([r["n"], r["m"], r["parametro"], r["replicacao"],
                        mlr_cmax, sa_cmax, round(mlr_tempo, 6), round(sa_tempo, 6),
                        round(diff_pct_cmax, 2), round(diff_pct_tempo, 2), winner_q, winner_t])
    print("Escrito:", detail_path)

    # Resumo por (n, m, alfa): média das diferenças %, vitórias MLR/SA qualidade e tempo
    detail_rows = list(csv.DictReader(open(detail_path, newline="", encoding="utf-8")))
    for row in detail_rows:
        row["diff_pct_cmax"] = float(row["diff_pct_cmax"])
        row["diff_pct_tempo"] = float(row["diff_pct_tempo"])
    key_fn = lambda r: (int(r["n"]), int(r["m"]), r["alfa"])
    groups = defaultdict(list)
    for r in detail_rows:
        groups[key_fn(r)].append(r)

    summary_path = os.path.join(out_dir, "5_comparison_summary.csv")
    with open(summary_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["n", "m", "alfa", "media_diff_pct_cmax", "media_diff_pct_tempo",
                    "wins_MLR_qualidade", "wins_SA_qualidade", "wins_MLR_tempo", "wins_SA_tempo"])
        for (n, m, alfa), replist in sorted(groups.items()):
            diffs_cmax = [x["diff_pct_cmax"] for x in replist]
            diffs_tempo = [x["diff_pct_tempo"] for x in replist]
            wins_q_mlr = sum(1 for x in replist if x["winner_qualidade"] == "MLR")
            wins_q_sa = sum(1 for x in replist if x["winner_qualidade"] == "SA")
            wins_t_mlr = sum(1 for x in replist if x["winner_tempo"] == "MLR")
            wins_t_sa = sum(1 for x in replist if x["winner_tempo"] == "SA")
            w.writerow([n, m, alfa,
                        round(statistics.mean(diffs_cmax), 2),
                        round(statistics.mean(diffs_tempo), 2),
                        wins_q_mlr, wins_q_sa, wins_t_mlr, wins_t_sa])
    print("Escrito:", summary_path)
    return detail_path, summary_path


def main():
    ensure_results_dir()
    raw_path = find_raw_result(sys.argv[1] if len(sys.argv) > 1 else None)
    if not raw_path:
        print("Nenhum arquivo de resultados encontrado. Passe o caminho para result.txt ou coloque result.txt ou results/raw_result.txt.", file=sys.stderr)
        sys.exit(1)
    print("Lendo:", raw_path)
    rows = load_raw_data(raw_path)
    if not rows:
        print("Nenhuma linha de dados.", file=sys.stderr)
        sys.exit(1)

    analysis_1_quality(rows, RESULTS_DIR)
    analysis_4_scalability(rows, RESULTS_DIR)
    analysis_5_comparison(rows, RESULTS_DIR)
    print("Análise concluída. CSVs em", RESULTS_DIR + "/")


if __name__ == "__main__":
    main()
