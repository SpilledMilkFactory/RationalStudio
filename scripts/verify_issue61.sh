#!/usr/bin/env bash
# Issue #61 — fresh Release Qt6 package qualification, no host installation.
set -Eeuo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
root="$PWD"
build="$root/build-issue61-release"
evidence="$root/issue61-evidence"
if [[ -e "$build" || -e "$evidence" ]]; then
    echo "ERROR: build-issue61-release or issue61-evidence already exists; refusing overwrite" >&2
    exit 2
fi
[[ -s LICENSE.MD ]] || { echo 'ERROR: genuine tracked LICENSE.MD required' >&2; exit 2; }
command -v dpkg-deb >/dev/null || { echo 'ERROR: dpkg-deb required'; exit 2; }
command -v dpkg-shlibdeps >/dev/null || { echo 'ERROR: dpkg-shlibdeps required (dpkg-dev)'; exit 2; }
command -v cpack >/dev/null || { echo 'ERROR: cpack required'; exit 2; }
mkdir -p "$evidence"
: "${RATIONALSTUDIO_JOBS:=4}"
[[ "$RATIONALSTUDIO_JOBS" =~ ^[1-9][0-9]*$ ]] || { echo 'ERROR: invalid jobs'; exit 2; }
export QT_QPA_PLATFORM=offscreen
export XDG_CONFIG_HOME="$evidence/xdg-config"
export XDG_CACHE_HOME="$evidence/xdg-cache"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME"
python3 scripts/ci/source_manifest.py --root "$root" --output "$evidence/source-before.sha256"
cmake -S . -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=ON \
    -DRATIONALSTUDIO_BUILD_GUI=ON \
    -DRATIONALSTUDIO_BUILD_CLI=ON \
    -DRATIONALSTUDIO_BUILD_BENCHMARKS=OFF \
    -DRATIONALSTUDIO_BUILD_ADVANCED_GRAPH_BENCHMARKS=OFF \
    -DRATIONALSTUDIO_ENABLE_LINUX_PACKAGING=ON 2>&1 | tee "$evidence/configure.log"
cmake --build "$build" --parallel "$RATIONALSTUDIO_JOBS" 2>&1 | tee "$evidence/build.log"
if grep -E -i '(^|[[:space:]])warning:' "$evidence/build.log" > "$evidence/compiler-warnings.txt"; then
    echo 'ERROR: compiler warnings in Release build' >&2; exit 1
fi
: > "$evidence/compiler-warnings.txt"
actual="$(ctest --test-dir "$build" -N | sed -n 's/^Total Tests: \([0-9][0-9]*\)$/\1/p')"
[[ "$actual" == 73 ]] || { echo "ERROR: expected 73 tests, got ${actual:-none}" >&2; exit 1; }
ctest --test-dir "$build" --output-on-failure --parallel "$RATIONALSTUDIO_JOBS" \
    --output-junit "$evidence/ctest-junit.xml" 2>&1 | tee "$evidence/ctest.log"
mkdir -p "$evidence/packages"
cpack --config "$build/CPackConfig.cmake" -G DEB \
    -B "$evidence/packages" 2>&1 | tee "$evidence/cpack.log"
mapfile -d '' -t packages < <(find "$evidence/packages" -maxdepth 1 -type f -name '*.deb' -print0)
[[ "${#packages[@]}" -eq 1 ]] || { echo "ERROR: expected one .deb package, got ${#packages[@]}" >&2; exit 1; }
python3 scripts/packaging/check_deb.py "${packages[0]}" 2>&1 | tee "$evidence/deb-check.log"
dpkg-deb -I "${packages[0]}" > "$evidence/deb-control.txt"
dpkg-deb -c "${packages[0]}" > "$evidence/deb-contents.txt"
# Offline same-host smoke from extraction: no sudo, no real settings writes.
mkdir -p "$evidence/extracted"
dpkg-deb -x "${packages[0]}" "$evidence/extracted"
python3 scripts/packaging/launch_smoke.py \
    "$evidence/extracted/usr/bin/RationalStudio" 2>&1 | tee "$evidence/launch-smoke.log"
python3 scripts/ci/source_manifest.py --root "$root" --output "$evidence/source-after.sha256"
cmp -s "$evidence/source-before.sha256" "$evidence/source-after.sha256" || {
    echo 'ERROR: source manifest changed during packaging' >&2; exit 1;
}
sha256sum "${packages[0]}" > "$evidence/package.sha256"
{ echo 'PASS: Linux Qt6 Release 73/73 CTests; Debian package metadata/payload PASS; extracted GUI smoke PASS; source unchanged.'
  echo 'NOTE: Real clean-system dependency installation + desktop-launch test still required before sign-off.';
} | tee "$evidence/result.txt"
