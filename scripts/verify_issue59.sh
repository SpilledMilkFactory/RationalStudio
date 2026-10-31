#!/usr/bin/env bash
# Optional fresh local Linux qualification mirroring the hosted CI matrix.
set -Eeuo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
for config in 'gui Debug' 'gui Release' 'core Sanitize'; do
    read -r kind flavor <<< "$config"
    CI_JOBS="${RATIONALSTUDIO_JOBS:-2}" bash scripts/ci/qualify.sh "$kind" "$flavor"
done
printf '\nPASS: all three Issue #59 local qualification configurations\n'
