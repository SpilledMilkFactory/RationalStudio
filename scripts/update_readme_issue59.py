#!/usr/bin/env python3
"""Add the M5-05 CI documentation once; preserve all existing README text."""
import argparse
from pathlib import Path

MARKER = '<!-- ISSUE59_README_UPDATED -->'
ANCHOR = '<!-- ISSUE58_README_UPDATED -->'
BLOCK = '''<!-- ISSUE59_README_UPDATED -->

**Automated Linux CMake/CTest workflows (Issue #59; Milestone 5).**
GitHub Actions runs on `ubuntu-24.04` for pull requests, pushes to `main`, and
manual `workflow_dispatch` requests. Three independent configurations exercise
Qt 6 GUI Debug (**71/71 expected CTests**), Qt 6 GUI Release (**71/71**), and
Qt-independent ASan/UBSan (**43/43**). The build uses CMake/Ninja with bundled
benchmarks disabled; complete application and regression targets are built.
Offscreen Qt regressions include normal and 2× rendering. The workflow fails
on build/test errors, changed source provenance, missing required benchmark
headers, unexpected test counts, or recorded compiler warnings. Per-configuration
CTest JUnit, build logs, and source checksums are uploaded as short-lived
artifacts (also on failure). Benchmark timings are not hard performance gates.
The `benchmarks/BenchmarkMetrics.hpp` dependency is included in source control
so clean CI checkouts can compile `RationalGraphBenchmarkStatisticsTests`.

Run the equivalent qualification on a Linux host with Qt 6 build packages:

```bash
RATIONALSTUDIO_JOBS=2 bash scripts/verify_issue59.sh
```

**Scope:** GitHub-hosted Linux results must be confirmed in the Actions UI
before Issue #59 is accepted; local runs alone do not establish hosted CI.
Native Windows and Qt 5 builds remain outside this initial CI matrix (Issue
#60). Packaging, clean-machine installation, and release reproducibility are
addressed by later Milestone 5 issues. Hosted runner versions and package
updates can affect results even when the project revision is unchanged.

'''

def update(text):
    if MARKER in text:
        return text, False
    if ANCHOR not in text:
        raise ValueError('Missing Issue #58 README anchor; refusing unsafe edit')
    updated = text.replace(ANCHOR, BLOCK + ANCHOR, 1)
    # Historical narrative still described the older Issue #24 test counts.
    old = ('With GUI and testing enabled, CTest registers **52 tests from 51 executables**: '
           'thirty-one Qt-independent tests and twenty-one offscreen Qt invocations '
           'from twenty widget test executables. The coverage executable also runs '
           'separately at 2× display scale. Core-only builds register thirty-one tests:')
    new = ('With GUI and testing enabled, CTest currently registers **71 tests** '
           '(including 1×/2× offscreen rendering cases). Qt-independent builds '
           'register **43 tests**. These counts are the verified Issue #58 baseline '
           'and the initial Issue #59 CI acceptance targets:')
    if old not in updated:
        raise ValueError('Expected older Testing count paragraph not found')
    return updated.replace(old, new, 1), True

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--file', default='README.md')
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()
    path = Path(args.file)
    before = path.read_text()
    after, changed = update(before)
    if changed and not args.dry_run:
        path.write_text(after)
    print(f'{"WOULD UPDATE" if args.dry_run else "UPDATED" if changed else "ALREADY UPDATED"}: {path}')

if __name__ == '__main__':
    main()
