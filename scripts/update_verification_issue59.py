#!/usr/bin/env python3
"""Record attested GitHub-hosted Issue #59 results without inventing evidence."""
import argparse
from pathlib import Path
import re

MARKER = '<!-- ISSUE59_VERIFICATION_CURRENT_BEGIN -->'
OLD_MARKER = '<!-- ISSUE58_VERIFICATION_CURRENT_BEGIN -->'

def update(text, run_url):
    if MARKER in text:
        return text, False
    if not text.startswith('# Rational Studio — Verification Report\n') or OLD_MARKER not in text:
        raise ValueError('Unexpected VERIFICATION.md structure; refusing to modify')
    block = f'''{MARKER}

**Current status: PASS — Issue #59 (M5-05), automated CMake/CTest workflows.**
GitHub-hosted Linux CI: Qt6 Debug **71/71 CTests PASS**, Qt6 Release
**71/71 PASS**, and Qt-independent ASan/UBSan **43/43 PASS**, as
**developer-attested from GitHub Actions run**:
{run_url}

GitHub Actions runs on `ubuntu-24.04` for pull requests, main-branch pushes,
and manually requested runs. Each job configures a clean CMake/Ninja build,
compiles full configured targets, runs the entire registered CTest suite,
checks compiler-warning logs and stable source provenance, and uploads
configuration logs, JUnit results and SHA-256 manifests even on failure.
The required benchmark-statistics helper was restored as tracked source to
make clean checkouts buildable. Core algorithms, GUI interaction, graph
persistence and mathematical behavior were not changed by this CI issue.

**Evidence limitation:** This entry records a developer-verified GitHub run
URL, not an independently fetched CI API attestation. Review the linked run
and each of its three jobs before treating this as hosted qualification.
Issue #58 and all earlier qualifications remain historical below.

'''
    replaced = text.replace(OLD_MARKER, '<!-- ISSUE58_VERIFICATION_HISTORICAL_BEGIN -->', 1)
    return replaced.replace('<!-- ISSUE58_VERIFICATION_HISTORICAL_BEGIN -->',
                            block + '<!-- ISSUE58_VERIFICATION_HISTORICAL_BEGIN -->', 1), True

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--file', default='VERIFICATION.md')
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--github-actions-pass', action='store_true')
    ap.add_argument('--run-url', help='Link to a completed green three-job GitHub Actions run')
    args = ap.parse_args()
    path = Path(args.file)
    text = path.read_text()
    if MARKER in text:
        print('ALREADY UPDATED: ' + str(path)); return
    if not args.github_actions_pass:
        raise SystemExit('ERROR: --github-actions-pass required; local tests alone are insufficient')
    pattern = r'https://github\.com/SpilledMilkFactory/RationalStudio/actions/runs/[0-9]+'
    if not args.run_url or not re.fullmatch(pattern, args.run_url):
        raise SystemExit('ERROR: supply exact GitHub Actions run URL via --run-url')
    revised, _ = update(text, args.run_url)
    if not args.dry_run:
        path.write_text(revised)
    print(('WOULD UPDATE' if args.dry_run else 'UPDATED') + ': ' + str(path))

if __name__ == '__main__':
    main()
