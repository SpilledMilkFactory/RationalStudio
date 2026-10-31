#!/usr/bin/env bash
# Issue #59: fail-closed, reproducible clean-checkout Linux qualification.
set -Eeuo pipefail

if [[ $# -ne 2 || ! "$1" =~ ^(gui|core)$ || ! "$2" =~ ^(Debug|Release|Sanitize)$ ]]; then
    echo 'Usage: bash scripts/ci/qualify.sh {gui|core} {Debug|Release|Sanitize}' >&2
    exit 2
fi
mode="$1"
requested="$2"
if [[ "$mode" == gui && "$requested" == Sanitize ]] || \
   [[ "$mode" == core && "$requested" != Sanitize ]]; then
    echo 'Supported configurations: gui Debug, gui Release, core Sanitize' >&2
    exit 2
fi
cd "$(dirname "${BASH_SOURCE[0]}")/../.."
root="$PWD"
label="${mode}-$(echo "$requested" | tr '[:upper:]' '[:lower:]')"
evidence="${CI_EVIDENCE_DIR:-$root/issue59-evidence/$label}"
mkdir -p "$evidence"
evidence="$(cd "$evidence" && pwd)"
build="$root/build-issue59-$label"
if [[ -e "$build" ]]; then
    echo "Refusing to overwrite existing build: $build" >&2
    exit 2
fi
mkdir -p "$build"
# Keep normal desktop QSettings state and test caches out of CI.
export XDG_CONFIG_HOME="$evidence/xdg-config"
export XDG_CACHE_HOME="$evidence/xdg-cache"
export QT_QPA_PLATFORM=offscreen
mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME"
: "${CI_JOBS:=2}"
if ! [[ "$CI_JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo 'CI_JOBS must be a positive integer' >&2
    exit 2
fi

for item in CMakeLists.txt src/core/Expression.cpp tests/GraphBenchmarkStatisticsTests.cpp benchmarks/BenchmarkMetrics.hpp; do
    if [[ ! -f "$item" ]]; then
        echo "Clean checkout is missing required tracked source: $item" >&2
        exit 2
    fi
done
python3 scripts/ci/source_manifest.py --root "$root" --output "$evidence/source-before.sha256"
{
    printf 'configuration=%s\n' "$label"
    printf 'timestamp_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf 'git_commit=%s\n' "$(git rev-parse HEAD 2>/dev/null || echo non-git-extraction)"
    uname -a
    cmake --version | head -1
    ninja --version
    c++ --version | head -1
    if [[ "$mode" == gui ]]; then pkg-config --modversion Qt6Widgets || true; fi
} > "$evidence/environment.txt"

cmake_args=(-S . -B "$build" -G Ninja -DBUILD_TESTING=ON
    -DRATIONALSTUDIO_BUILD_CLI=ON
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF
    -DRATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS=OFF)
if [[ "$mode" == gui ]]; then
    cmake_args+=(-DRATIONALSTUDIO_BUILD_GUI=ON -DCMAKE_BUILD_TYPE="$requested")
else
    cmake_args+=(-DRATIONALSTUDIO_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
        '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer'
        '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
        '-DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=address,undefined')
    export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:halt_on_error=1}"
    export UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}"
fi
cmake "${cmake_args[@]}" 2>&1 | tee "$evidence/configure.log"
cmake --build "$build" --parallel "$CI_JOBS" 2>&1 | tee "$evidence/build.log"
# Explicitly reject compiler warnings, not generic CMake text containing "warning".
if grep -E -i '(^|[[:space:]])warning:' "$evidence/build.log" > "$evidence/compiler-warnings.txt"; then
    echo 'Compiler warnings are a CI failure. See compiler-warnings.txt.' >&2
    exit 1
fi
: > "$evidence/compiler-warnings.txt"
expected=43
[[ "$mode" == gui ]] && expected=71
actual="$(ctest --test-dir "$build" -N | sed -n 's/^Total Tests: \([0-9][0-9]*\)$/\1/p')"
if [[ "$actual" != "$expected" ]]; then
    echo "Expected $expected CTests; CMake registered ${actual:-none}." >&2
    exit 1
fi
printf 'expected_tests=%s\nregistered_tests=%s\n' "$expected" "$actual" > "$evidence/test-count.txt"
ctest --test-dir "$build" --output-on-failure --parallel "$CI_JOBS" \
    --output-junit "$evidence/ctest-junit.xml" 2>&1 | tee "$evidence/ctest.log"
python3 scripts/ci/source_manifest.py --root "$root" --output "$evidence/source-after.sha256"
if ! cmp -s "$evidence/source-before.sha256" "$evidence/source-after.sha256"; then
    echo 'Source provenance changed during CI qualification.' >&2
    diff -u "$evidence/source-before.sha256" "$evidence/source-after.sha256" || true
    exit 1
fi
printf 'PASS: %s (%s/%s tests), no compiler warnings, unchanged sources\n' \
    "$label" "$actual" "$expected" | tee "$evidence/result.txt"
