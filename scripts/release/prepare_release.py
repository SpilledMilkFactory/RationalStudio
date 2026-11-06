#!/usr/bin/env python3
"""Issue #66: offline, fail-closed release-candidate assembler.

This tool NEVER creates tags, pushes commits, creates a GitHub Release, or
asserts that a human redistribution review is legally sufficient. Builds and
GitHub Actions runs must already have been performed against the tag commit.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# Run directly from any working directory, without requiring pip installation.
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.packaging.check_deb import check as check_deb  # noqa: E402
from scripts.packaging.check_windows_zip import inspect as inspect_windows  # noqa: E402
from scripts.packaging.prepare_qt_source_assets import ASSETS as QT_ASSETS  # noqa: E402

CHECKS = {
    'linux_debug': ('Linux Qt6 Debug', 73, 'ubuntu-24.04'),
    'linux_release': ('Linux Qt6 Release', 73, 'ubuntu-24.04'),
    'core_sanitize': ('Linux core ASan/UBSan', 43, 'ubuntu-24.04'),
    'linux_deb': ('Linux Debian packaging', 73, 'ubuntu-24.04'),
    'linux_license': ('Linux license/dependency audit', 73, 'ubuntu-24.04'),
    'linux_gui_smoke': ('Linux Xvfb/offscreen smoke', 2, 'ubuntu-24.04'),
    'windows_portable': ('Windows MSVC Qt6 portable ZIP', 73, 'windows-2022'),
}
REVIEW_KEYS = (
    'linux_qt', 'windows_qt', 'windows_icu', 'msvc_redist',
    'qt_source', 'clean_linux', 'clean_windows', 'license_approval',
)
TAG_PATTERN = re.compile(r'v(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)\Z')
SHA_PATTERN = re.compile(r'[0-9a-f]{40}\Z')
SHA256_PATTERN = re.compile(r'[0-9a-f]{64}\Z')
RUN_PATTERN = re.compile(r'https://github\.com/SpilledMilkFactory/RationalStudio/actions/runs/[1-9]\d*\Z')


class ReleaseError(ValueError):
    """A release precondition was not satisfied."""


def git(root: Path, *args: str) -> str:
    result = subprocess.run(['git', '-C', str(root), *args], capture_output=True, text=True)
    if result.returncode:
        raise ReleaseError(f'git {" ".join(args)} failed: {result.stderr.strip()}')
    return result.stdout.strip()


def project_version(root: Path) -> str:
    data = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    m = re.search(r'(?m)^project\(RationalStudio\s+VERSION\s+(\d+\.\d+\.\d+)\s+LANGUAGES\s+CXX\)', data)
    if not m:
        raise ReleaseError('Cannot extract a unique CMake project version')
    return m.group(1)


def verify_tag(root: Path, tag: str) -> tuple[str, str]:
    m = TAG_PATTERN.fullmatch(tag)
    if not m:
        raise ReleaseError('Tag must be vMAJOR.MINOR.PATCH (no prerelease for v0.1.0)')
    root = root.resolve()
    if Path(git(root, 'rev-parse', '--show-toplevel')).resolve() != root:
        raise ReleaseError('Source directory must be the Git repository root')
    version = project_version(root)
    if tag != 'v' + version:
        raise ReleaseError(f'Tag {tag} differs from CMake project version {version}')
    if git(root, 'status', '--porcelain', '--untracked-files=normal'):
        raise ReleaseError('Git tree is not clean; commit changes before tagging and preparing a release')
    # Require an ANNOTATED tag, not merely an arbitrary branch or lightweight tag.
    kind = git(root, 'cat-file', '-t', f'refs/tags/{tag}')
    if kind != 'tag':
        raise ReleaseError(f'{tag} is not an annotated Git tag')
    commit = git(root, 'rev-parse', 'HEAD').lower()
    tagged = git(root, 'rev-parse', f'refs/tags/{tag}^{{commit}}').lower()
    if not SHA_PATTERN.fullmatch(commit) or commit != tagged:
        raise ReleaseError(f'Tag {tag} does not resolve to checked-out HEAD')
    for required in ('LICENSE.MD', 'docs/USER_GUIDE.md', 'docs/ENGINEERING_HISTORY.md',
                     'VERIFICATION.md', 'THIRD_PARTY_DEPENDENCIES.md',
                     'scripts/packaging/test_user_documentation.py',
                     'scripts/release/prepare_release.py', f'docs/release/RELEASE_NOTES_{tag}.md'):
        if required not in git(root, 'ls-files', '--', required).splitlines():
            raise ReleaseError(f'Required file is not tracked in tagged Git source: {required}')
    return version, commit


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def evidence_read(path: Path, commit: str) -> dict:
    data = json.loads(path.read_text(encoding='utf-8'))
    if not isinstance(data, dict) or set(data) != {'source_commit', 'checks', 'review'}:
        raise ReleaseError('Evidence requires exactly source_commit, checks, review')
    if data['source_commit'] != commit:
        raise ReleaseError('Evidence source_commit does not match annotated tag revision')
    checks = data['checks']
    if not isinstance(checks, dict) or set(checks) != set(CHECKS):
        raise ReleaseError(f'Evidence checks must contain exactly: {", ".join(CHECKS)}')
    for name, (label, count, runner) in CHECKS.items():
        item = checks[name]
        expected_fields = {'run_url', 'source_commit', 'ctests_passed', 'runner', 'toolchain'}
        if name in ('linux_deb', 'windows_portable'):
            expected_fields.add('artifact_sha256')
        if not isinstance(item, dict) or set(item) != expected_fields:
            raise ReleaseError(f'Missing or extra {label} evidence fields')
        if 'artifact_sha256' in expected_fields and (not isinstance(item['artifact_sha256'], str) or
                                                      not SHA256_PATTERN.fullmatch(item['artifact_sha256'])):
            raise ReleaseError(f'Missing or invalid tagged-run package digest for {label}')
        if not isinstance(item['run_url'], str) or not RUN_PATTERN.fullmatch(item['run_url']):
            raise ReleaseError(f'Invalid hosted GitHub Actions run URL for {label}')
        if item['source_commit'] != commit or item['runner'] != runner or item['ctests_passed'] != count:
            raise ReleaseError(f'Incorrect revision/runner/CTest count for {label}')
        if (not isinstance(item['toolchain'], str) or not 3 <= len(item['toolchain']) <= 200
                or re.search(r'\b(REPLACE|TODO|TBD|PENDING|NOT REVIEWED)\b', item['toolchain'], flags=re.I)):
            raise ReleaseError(f'Toolchain version evidence missing for {label}')
    review = data['review']
    if not isinstance(review, dict) or set(review) != set(REVIEW_KEYS) | {'reviewer', 'reviewed_utc', 'approval_statement'}:
        raise ReleaseError('Incomplete manual release review fields')
    for key in REVIEW_KEYS + ('reviewer',):
        value = review[key]
        if not isinstance(value, str) or not value.strip() or len(value) > 2000 or re.search(r'\b(REPLACE|TODO|TBD|PENDING|NOT REVIEWED)\b', value, flags=re.I):
            raise ReleaseError(f'Manual review field {key!r} is incomplete')
    if review['approval_statement'] != 'I have reviewed the exact release assets and approve public distribution.':
        raise ReleaseError('Release owner has not entered the exact affirmative approval statement')
    try:
        reviewed = datetime.fromisoformat(review['reviewed_utc'].replace('Z', '+00:00'))
        if reviewed.tzinfo is None:
            raise ValueError('no timezone')
    except (TypeError, ValueError) as exc:
        raise ReleaseError('reviewed_utc must be a timezone-aware ISO 8601 timestamp') from exc
    return data


def validate_packages(root: Path, version: str, deb: Path, windows_zip: Path, qt_sources: Path) -> dict[str, Path]:
    if deb.name != f'rationalstudio_{version}-1_amd64.deb' or not deb.is_file():
        raise ReleaseError('Expected the exact versioned Ubuntu amd64 Debian package')
    if windows_zip.name != f'RationalStudio-{version}-Windows-x64.zip' or not windows_zip.is_file():
        raise ReleaseError('Expected the exact versioned Windows portable package')
    # Reuse the real, existing package validators. This cannot replace legal approval.
    check_deb(deb, True)
    inspect_windows(windows_zip, root, version)
    files = {deb.name: deb, windows_zip.name: windows_zip}
    for filename, expected in QT_ASSETS.items():
        path = qt_sources / filename
        if not path.is_file() or sha256(path) != expected:
            raise ReleaseError(f'Qt source archive missing or SHA-256 mismatch: {filename}')
        files[filename] = path
    return files


def markdown_escape(value: object) -> str:
    return str(value).replace('\\', '\\\\').replace('|', '\\|').replace('\r', ' ').replace('\n', ' ').replace('`', '\\`')


def qualification_report(version: str, tag: str, commit: str, evidence: dict) -> str:
    lines = [f'# Rational Studio {version} — Release qualification', '',
             f'- Source tag: `{tag}` (annotated)', f'- Git commit: `{commit}`',
             '- Evidence: **maintainer-provided GitHub run identifiers**. This file does not independently query GitHub.',
             '- Rebuilding the tagged source is supported; **bit-for-bit identical binary packages are not guaranteed**.', '',
             '## Hosted build/test results', '',
             '| Check | Runner | CTests | Toolchain | GitHub run |', '|---|---|---:|---|---|']
    for key, (name, _, _) in CHECKS.items():
        item = evidence['checks'][key]
        lines.append(f'| {name} | `{item["runner"]}` | {item["ctests_passed"]} PASS (reported) | '
                     f'{markdown_escape(item["toolchain"])} | {item["run_url"]} |')
    lines += ['', '## Manual release gates (release-owner attestation)', '',
              f'- Reviewer: {markdown_escape(evidence["review"]["reviewer"])}',
              f'- Reviewed: {markdown_escape(evidence["review"]["reviewed_utc"])}',
              '- The following are reviewer-supplied references/statements, not automated licensing certification:', '']
    for key in REVIEW_KEYS:
        lines.append(f'- **{key.replace("_", " ").title()}**: {markdown_escape(evidence["review"][key])}')
    lines += ['', '## Unchanged limitations', '',
              '- Known Windows 125% DPI layout issue remains documented.',
              '- Qt and MSVC redistribution must be reviewed for the exact published binaries.',
              '- Users must be able to access corresponding Qt source and relinking instructions.',
              '- Fresh source/archive checksums and release asset inventory accompany this record.', '']
    return '\n'.join(lines)


def assemble(root: Path, tag: str, deb: Path, windows_zip: Path, qt_sources: Path,
             evidence_file: Path, output: Path) -> Path:
    version, commit = verify_tag(root, tag)
    evidence = evidence_read(evidence_file, commit)
    packages = validate_packages(root, version, deb, windows_zip, qt_sources)
    for job, path in (('linux_deb', deb), ('windows_portable', windows_zip)):
        expected = evidence['checks'][job]['artifact_sha256']
        actual = sha256(path)
        if expected != actual:
            raise ReleaseError(f'{job} package differs from the recorded tagged-run artifact SHA-256')
    notes = root / 'docs' / 'release' / f'RELEASE_NOTES_v{version}.md'
    if not notes.is_file() or f'Rational Studio v{version}' not in notes.read_text(encoding='utf-8'):
        raise ReleaseError('Versioned release notes missing or incorrect')
    if output.exists():
        raise ReleaseError(f'Output already exists (refusing overwrite): {output}')
    if output.resolve().is_relative_to(root.resolve()):
        raise ReleaseError('Place release assets outside the Git worktree')
    output.mkdir(parents=True)
    try:
        files = {}
        for name, src in packages.items():
            dst = output / name
            shutil.copyfile(src, dst)
            files[name] = dst
        source_name = f'RationalStudio-{version}-source.tar.gz'
        source_path = output / source_name
        # Git archive operates on the reviewed commit object, never on untracked files.
        with source_path.open('wb') as destination:
            subprocess.run(['git', '-C', str(root), 'archive', '--format=tar.gz',
                            f'--prefix=RationalStudio-{version}/', commit],
                           stdout=destination, check=True)
        files[source_name] = source_path
        release_notes = output / 'RELEASE_NOTES.md'
        shutil.copyfile(notes, release_notes)
        files[release_notes.name] = release_notes
        report_path = output / 'RELEASE_QUALIFICATION.md'
        report_path.write_text(qualification_report(version, tag, commit, evidence), encoding='utf-8')
        files[report_path.name] = report_path
        # Manifest contains only source/environment/evidence metadata and checksums.
        payload = {name: {'sha256': sha256(path), 'size_bytes': path.stat().st_size}
                   for name, path in sorted(files.items())}
        manifest = {'schema': 1, 'project': 'RationalStudio', 'version': version, 'tag': tag,
                    'source_commit': commit, 'assets': payload, 'checks': evidence['checks'],
                    'reviewer': evidence['review']['reviewer'], 'reviewed_utc': evidence['review']['reviewed_utc'],
                    'note': 'Manifest records human-reviewed evidence; no bit-for-bit build reproducibility or legal certification is claimed.'}
        manifest_file = output / 'RELEASE_MANIFEST.json'
        manifest_file.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n', encoding='utf-8')
        files[manifest_file.name] = manifest_file
        (output / 'SHA256SUMS.txt').write_text(''.join(f'{sha256(path)}  {name}\n' for name, path in sorted(files.items())), encoding='ascii')
    except Exception:
        shutil.rmtree(output)
        raise
    return output


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=Path('.'))
    p.add_argument('--tag', required=True, help='Annotated vMAJOR.MINOR.PATCH tag at checked-out HEAD')
    sub = p.add_subparsers(dest='command', required=True)
    sub.add_parser('check-tag', help='Validate source version, Git tag, tracked files and clean tree')
    stage = sub.add_parser('assemble', help='Validate evidence and packages; write offline release assets')
    stage.add_argument('--deb', type=Path, required=True)
    stage.add_argument('--windows-zip', type=Path, required=True)
    stage.add_argument('--qt-source-dir', type=Path, required=True)
    stage.add_argument('--evidence', type=Path, required=True)
    stage.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    try:
        if a.command == 'check-tag':
            version, commit = verify_tag(a.root, a.tag)
            print(f'PASS: RationalStudio {version} tag={a.tag} commit={commit} (clean, tracked, annotated)')
        else:
            output = assemble(a.root.resolve(), a.tag, a.deb.resolve(), a.windows_zip.resolve(),
                              a.qt_source_dir.resolve(), a.evidence.resolve(), a.output.resolve())
            print(f'PASS: candidate bundle ready (not published): {output}')
            print('Release owner must review all assets, checksums and evidence before manually publishing a draft.')
    except (ReleaseError, OSError, subprocess.CalledProcessError, json.JSONDecodeError) as exc:
        p.exit(1, f'ERROR: {exc}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
