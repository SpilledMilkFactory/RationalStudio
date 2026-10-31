#!/usr/bin/env bash
# Issue #64 — fail-closed technical license/package qualification (not legal certification).
set -Eeuo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
root="$PWD"
build="$root/build-issue64-release"
evidence="$root/issue64-evidence"
if [[ -e "$build" || -e "$evidence" ]]; then
    echo 'ERROR: issue64 build/evidence already exists; refusing overwrite' >&2; exit 2
fi
: "${RATIONALSTUDIO_JOBS:=4}"
[[ "$RATIONALSTUDIO_JOBS" =~ ^[1-9][0-9]*$ ]] || { echo 'ERROR: invalid jobs'; exit 2; }
mkdir -p "$evidence/packages"
export QT_QPA_PLATFORM=offscreen
export XDG_CONFIG_HOME="$evidence/config"
export XDG_CACHE_HOME="$evidence/cache"
python3 scripts/ci/source_manifest.py --root "$root" --output "$evidence/source-before.sha256"
python3 scripts/compliance/audit_licenses.py --root "$root" --json "$evidence/source-audit.json" \
    2>&1 | tee "$evidence/source-audit.log"
python3 scripts/compliance/test_audit_licenses.py -v 2>&1 | tee "$evidence/compliance-regressions.log"
cmake -S . -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=ON -DRATIONALSTUDIO_BUILD_GUI=ON \
    -DRATIONALSTUDIO_BUILD_CLI=ON \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DRATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS=OFF \
    -DRATIONALSTUDIO_ENABLE_LINUX_PACKAGING=ON \
    2>&1 | tee "$evidence/configure.log"
cmake --build "$build" --parallel "$RATIONALSTUDIO_JOBS" 2>&1 | tee "$evidence/build.log"
if grep -Ei '(^|[[:space:]])warning:' "$evidence/build.log" > "$evidence/compiler-warnings.txt"; then
    echo 'ERROR: compiler warnings found' >&2; exit 1
fi
: > "$evidence/compiler-warnings.txt"
actual="$(ctest --test-dir "$build" -N | sed -n 's/^Total Tests: \([0-9][0-9]*\)$/\1/p')"
[[ "$actual" == '73' ]] || { echo "ERROR: expected 73 CTests; got ${actual:-none}" >&2; exit 1; }
ctest --test-dir "$build" --output-on-failure --parallel "$RATIONALSTUDIO_JOBS" \
    --output-junit "$evidence/ctest-junit.xml" 2>&1 | tee "$evidence/ctest.log"
mkdir -p "$evidence/packages"
cpack --config "$build/CPackConfig.cmake" -G DEB -B "$evidence/packages" \
    2>&1 | tee "$evidence/cpack.log"
mapfile -d '' -t packages < <(find "$evidence/packages" -maxdepth 1 -type f -name '*.deb' -print0)
[[ "${#packages[@]}" == 1 ]] || { echo "ERROR: expected one .deb, got ${#packages[@]}" >&2; exit 1; }
python3 scripts/packaging/check_deb.py "${packages[0]}" 2>&1 | tee "$evidence/deb-check.log"
python3 scripts/compliance/audit_licenses.py --root "$root" --deb "${packages[0]}" \
    --json "$evidence/license-audit.json" 2>&1 | tee "$evidence/license-audit.log"
mkdir -p "$evidence/extracted"
dpkg-deb -x "${packages[0]}" "$evidence/extracted"
python3 scripts/packaging/launch_smoke.py "$evidence/extracted/usr/bin/RationalStudio" \
    2>&1 | tee "$evidence/launch-smoke.log"
python3 scripts/ci/source_manifest.py --root "$root" --output "$evidence/source-after.sha256"
cmp -s "$evidence/source-before.sha256" "$evidence/source-after.sha256" || {
    echo 'ERROR: source changed during qualification' >&2; exit 1;
}
sha256sum "${packages[0]}" > "$evidence/package.sha256"
printf 'PASS: Issue #64 source/package notice technical audit; 73/73 CTests; clean provenance; extracted GUI smoke.\nLEGAL REVIEW: still required for Qt source/relinking conditions and release approval.\n' \
    | tee "$evidence/result.txt"
