#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build-issue25-release}"
OUT="${OUTPUT_DIR:-$ROOT/benchmark-results/issue25}"
REPETITIONS="${REPETITIONS:-25}"
WARMUPS="${WARMUPS:-5}"
EVENTS="${NAVIGATION_EVENTS:-12}"
mkdir -p "$OUT"
if [[ ! -x "$BUILD/RationalGraphSamplingBench" || ! -x "$BUILD/RationalGraphCanvasBench" ]]; then
    echo 'Missing benchmark executable(s). Configure with -DRATIONALSTUDIO_BUILD_BENCHMARKS=ON and build.' >&2
    exit 1
fi
printf 'Benchmark output: %s\nBuild: %s\n' "$OUT" "$BUILD"
"$BUILD/RationalGraphSamplingBench" --output "$OUT" \
    --repetitions "$REPETITIONS" --warmups "$WARMUPS" \
    --navigation-events "$EVENTS"
QT_QPA_PLATFORM=offscreen "$BUILD/RationalGraphCanvasBench" --output "$OUT" \
    --repetitions "$REPETITIONS" --warmups "$WARMUPS" \
    --navigation-events "$EVENTS"
python3 "$ROOT/scripts/summarize_issue25.py" --input "$OUT" \
    --output "$OUT/RESULTS.md" --repo-root "$ROOT" \
    --repetitions "$REPETITIONS" --navigation-events "$EVENTS"
cat <<EOF2
Created:
 $OUT/graph_sampling.csv
 $OUT/graph_sampling.json
 $OUT/graph_canvas.csv
 $OUT/graph_canvas.json
 $OUT/RESULTS.md
Review results and machine metadata before committing any benchmark report.
EOF2
