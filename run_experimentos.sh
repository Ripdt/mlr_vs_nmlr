#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

MACHINES=(10 20 50)
R_VALUES=(1.5 2)
REPLICACOES=${REPLICACOES:-10}
OUTPUT=result.txt

echo "Compilando..."
gcc main.c machine.c stack.c -o mlr_vs_nmlr -lm

echo "heuristica,n,m,replicacao,tempo,iteracoes,valor,parametro" > "$OUTPUT"
echo "Saída: $OUTPUT (replicações=$REPLICACOES)"

for m in "${MACHINES[@]}"; do
  for r in "${R_VALUES[@]}"; do
    n=$(awk "BEGIN { printf \"%.0f\", $m^$r }")
    echo "Rodando m=$m n=$n (r=$r) com $REPLICACOES replicações..."
    ./mlr_vs_nmlr "$m" "$n" "$REPLICACOES"
  done
done

echo "Concluído. Resultados em $OUTPUT"
wc -l "$OUTPUT"

mkdir -p results
cp "$OUTPUT" results/raw_result.txt
if command -v python3 >/dev/null 2>&1; then
  echo "Executando análise (1_quality, 4_scalability, 5_comparison)..."
  python3 analyze_results.py results/raw_result.txt
else
  echo "python3 não encontrado; pule a análise. CSVs de convergência/aceitação estão em results/"
fi
