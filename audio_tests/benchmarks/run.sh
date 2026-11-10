#!/usr/bin/env bash
# Orchestrates the full benchmark (runs INSIDE the neve-audio-bench container).
# NEVE_AUDIO_BENCH_RUNS (default 3): repetitions of every implementation; merge.py reports medians.
# NEVE_AUDIO_BENCH_RESULTS: output directory (default audio_tests/benchmarks/results).
set -euo pipefail

cd /neve
export NEVE_LIBS=/neve/lib
RUNS=${NEVE_AUDIO_BENCH_RUNS:-3}

echo "== [1/6] lib.so (FFI pattern) ==="
make -C /neve/lib/audio 2>&1 | tail -5

echo "== [2/6] test signals =="
python3 audio_tests/benchmarks/gen_data.py

cd /neve/audio_tests/benchmarks
OUT=${NEVE_AUDIO_BENCH_RESULTS:-/neve/audio_tests/benchmarks/results}
export NEVE_AUDIO_BENCH_RESULTS="$OUT"
mkdir -p "$OUT"
rm -rf "$OUT"/* "$OUT"/.[!.]* 2>/dev/null || true
# bench_neve.nv writes to results/ (relative path): point it at the chosen directory.
if [ "$OUT" != "/neve/audio_tests/benchmarks/results" ]; then
  rm -rf /neve/audio_tests/benchmarks/results
  ln -s "$OUT" /neve/audio_tests/benchmarks/results
fi

# 1T columns run pinned to one core (the first one this container may use); defT runs are not pinned.
PIN=""
if command -v taskset >/dev/null 2>&1; then
  CPU=$(taskset -pc $$ | sed 's/.*: //; s/[,-].*//')
  PIN="taskset -c $CPU"
fi
ONE="env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 NUMEXPR_NUM_THREADS=1"
RSS="python3 /neve/audio_tests/benchmarks/run_with_rss.py"

for r in $(seq 1 "$RUNS"); do
  echo "== [3/6] python: librosa / torchaudio (1 thread), run $r/$RUNS =="
  $RSS librosa 1T "$r" -- $PIN $ONE python3 bench_py.py --impl librosa --threads 1 > "$OUT/librosa_1_r$r.csv"
  $RSS torchaudio 1T "$r" -- $PIN $ONE python3 bench_py.py --impl torchaudio --threads 1 > "$OUT/torchaudio_1_r$r.csv"

  echo "== [4/6] python: librosa / torchaudio (default threads), run $r/$RUNS =="
  $RSS librosa defT "$r" -- python3 bench_py.py --impl librosa --threads default > "$OUT/librosa_default_r$r.csv"
  $RSS torchaudio defT "$r" -- python3 bench_py.py --impl torchaudio --threads default > "$OUT/torchaudio_default_r$r.csv"

  echo "== [5/6] neve, run $r/$RUNS =="
  (cd /neve && $RSS neve 1T "$r" -- $PIN neve audio_tests/benchmarks/bench_neve.nv > "$OUT/neve_r$r.csv")
done

echo "== [6/6] merge + correctness cross-check =="
python3 merge.py

echo "== done =="
