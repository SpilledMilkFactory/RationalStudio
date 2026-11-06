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
from scripts.compliance.audit_licenses import inspect_source, inspect_deb, AuditError  # noqa: E402

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
    'qt_source', 'clean_linux', 'clean_ubuntu_26_04', 'clean_windows', 'license_approval',
)
REQUIRED_SOURCE = (
    '.gitattributes', 'CMakeLists.txt', 'LICENSE.MD', 'README.md', 'docs/USER_GUIDE.md',
    'docs/ENGINEERING_HISTORY.md', 'VERIFICATION.md', 'THIRD_PARTY_DEPENDENCIES.md',
    'scripts/packaging/test_user_documentation.py', 'scripts/packaging/check_deb.py',
    'scripts/packaging/check_windows_zip.py', 'scripts/packaging/prepare_qt_source_assets.py',
    'scripts/compliance/audit_licenses.py', 'scripts/release/prepare_release.py',
    'scripts/release/test_prepare_release.py', 'docs/release/RELEASE_PROCEDURE.md',
    'docs/release/qualification-template.json', 'cmake/LinuxPackaging.cmake',
    'cmake/WindowsPackaging.cmake', 'packaging/linux/THIRD_PARTY_NOTICES.txt',
    'packaging/windows/THIRD_PARTY_NOTICES.txt', 'packaging/windows/QT_SOURCE_AND_RELINKING.md',
    'packaging/licenses/LGPL-3.0.txt', 'packaging/licenses/GPL-3.0.txt',
    'packaging/licenses/BSL-1.0.txt',
    *(f'.github/workflows/{name}.yml' for name in (
        'linux-cmake-ctest', 'linux-debian-package', 'linux-gui-smoke',
        'linux-license-compliance', 'windows-portable-package')),
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
    tracked = git(root, 'ls-files').splitlines()
    for name in tracked:
        parts = Path(name).parts
        if (any(part in ('__pycache__', '.git', '.aws', '.venv', 'qt-everywhere-full') for part in parts)
                or name.endswith(('.pyc', '.pyo')) or parts[0] == '.env'
                or parts[0].startswith(('build-', 'build_')) or parts[0] == 'build'
                or re.fullmatch(r'issue[0-9]+-evidence', parts[0])):
            raise ReleaseError(f'Forbidden development/archive payload tracked in source: {name}')
    for required in (*REQUIRED_SOURCE, f'docs/release/RELEASE_NOTES_{tag}.md'):
        if required not in git(root, 'ls-files', '--', required).splitlines():
            raise ReleaseError(f'Required file is not tracked in tagged Git source: {required}')
    return version, commit


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def evidence_read(path: Path, commit: str, draft_candidate: bool = False) -> dict:
    data = json.loads(path.read_text(encoding='utf-8'))
    if not isinstance(data, dict) or set(data) != {'source_commit', 'checks', 'review', 'ubuntu_26_04'}:
        raise ReleaseError('Evidence requires exactly source_commit, checks, review, ubuntu_26_04')
    if data['source_commit'] != commit:
        raise ReleaseError('Evidence source_commit does not match annotated tag revision')
    checks = data['checks']
    if not isinstance(checks, dict) or set(checks) != set(CHECKS):
        raise ReleaseError(f'Evidence checks must contain exactly: {", ".join(CHECKS)}')
    for name, (label, count, runner) in CHECKS.items():
        item = checks[name]
        expected_fields = {'run_url', 'source_commit', 'ctests_passed', 'runner', 'toolchain',
                           'conclusion', 'workflow', 'evidence_paths', 'results'}
        if name in ('linux_deb', 'windows_portable'):
            expected_fields.update(('artifact_sha256', 'artifact_id'))
        if not isinstance(item, dict) or set(item) != expected_fields:
            raise ReleaseError(f'Missing or extra {label} evidence fields')
        if item['conclusion'] != 'success':
            raise ReleaseError(f'Non-success hosted conclusion for {label}')
        for field in ('workflow', 'results'):
            require_evidence_text(item[field], f'{label} {field}')
        require_evidence_paths(item['evidence_paths'], label)
        if 'artifact_id' in expected_fields and (type(item['artifact_id']) is not int or item['artifact_id'] <= 0):
            raise ReleaseError(f'Invalid artifact identifier for {label}')
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
    validate_ubuntu26(data['ubuntu_26_04'], commit)
    review = data['review']
    if not isinstance(review, dict) or set(review) != set(REVIEW_KEYS) | {'reviewer', 'reviewed_utc', 'approval_statement'}:
        raise ReleaseError('Incomplete manual release review fields')
    if draft_candidate:
        # Explicitly pending fields are allowed only here, never technical failures.
        # No reviewer, timestamp or approval is generated by this tool.
        if any(not isinstance(value, str) or len(value) > 2000
               or re.search(r'\b(REPLACE|TODO|TBD)\b', value, re.I) for value in review.values()):
            raise ReleaseError('Draft review fields must be bounded strings without template placeholders')
        if review['approval_statement']:
            raise ReleaseError('Draft candidate must leave public approval_statement empty')
        return data
    if 'PENDING' in data['ubuntu_26_04']['runtime'].values():
        raise ReleaseError('Final approval requires completed Ubuntu 26.04 native acceptance')
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


def require_evidence_text(value: object, label: str) -> None:
    if (not isinstance(value, str) or not value.strip() or len(value) > 4000
            or re.search(r'\b(REPLACE|TODO|TBD|PENDING|NOT REVIEWED)\b', value, re.I)):
        raise ReleaseError(f'Incomplete technical evidence: {label}')


def require_evidence_paths(value: object, label: str) -> None:
    if not isinstance(value, list) or not value:
        raise ReleaseError(f'Missing evidence paths: {label}')
    for entry in value:
        require_evidence_text(entry, label)


def validate_ubuntu26(item: dict, commit: str) -> None:
    fields = {'source_commit', 'build_host', 'os_version', 'cmake_version',
              'compiler_version', 'qt_version', 'ctests_passed', 'artifact_sha256',
              'build_configuration', 'native_build', 'packaging_validation',
              'licensing_audit', 'depends', 'runtime', 'evidence_paths', 'limitations'}
    if not isinstance(item, dict) or set(item) != fields:
        raise ReleaseError('Incomplete ubuntu_26_04 evidence fields')
    if (item['source_commit'] != commit or item['ctests_passed'] != 73
            or item['build_configuration'] != 'Release' or item['native_build'] is not True):
        raise ReleaseError('Incorrect ubuntu_26_04 revision/CTest count/native Release build')
    for key in ('packaging_validation', 'licensing_audit'):
        if item[key] != 'PASS':
            raise ReleaseError(f'ubuntu_26_04 {key} must PASS')
    for key in ('build_host', 'os_version', 'cmake_version', 'compiler_version', 'qt_version', 'depends'):
        require_evidence_text(item[key], f'ubuntu_26_04 {key}')
    if not re.search(r'Ubuntu.*26\.04', item['os_version']) or not re.fullmatch(r'6\.\d+\.\d+', item['qt_version']):
        raise ReleaseError('ubuntu_26_04 requires Ubuntu 26.04 and Qt6 version evidence')
    if not isinstance(item['artifact_sha256'], str) or not SHA256_PATTERN.fullmatch(item['artifact_sha256']):
        raise ReleaseError('Invalid ubuntu_26_04 artifact SHA-256')
    require_evidence_paths(item['evidence_paths'], 'ubuntu_26_04')
    runtime = item['runtime']
    if not isinstance(runtime, dict) or set(runtime) != {'extracted_gui', 'cli_arithmetic', 'clean_install', 'desktop_graphing', 'package_removal'}:
        raise ReleaseError('Incomplete ubuntu_26_04 runtime evidence')
    for key, result in runtime.items():
        if result not in ('PASS', 'PENDING') or (key in ('extracted_gui', 'cli_arithmetic') and result != 'PASS'):
            raise ReleaseError(f'Invalid ubuntu_26_04 runtime result: {key}')
    if not isinstance(item['limitations'], list) or any(not isinstance(x, str) or not x.strip() for x in item['limitations']):
        raise ReleaseError('ubuntu_26_04 limitations must be a list of statements')
    if 'PENDING' in runtime.values() and not item['limitations']:
        raise ReleaseError('Pending native acceptance must record limitations')


def validate_packages(root: Path, version: str, deb: Path, windows_zip: Path,
                      qt_sources: Path, ubuntu26: Path | None = None) -> dict[str, Path]:
    if ubuntu26 is None:
        raise ReleaseError('Missing Ubuntu 26.04 package')
    if windows_zip.name != f'RationalStudio-{version}-Windows-x64.zip' or not windows_zip.is_file():
        raise ReleaseError('Expected the exact versioned Windows portable package')
    files = {windows_zip.name: windows_zip}
    source = inspect_source(root)
    for target, package in (('24.04', deb), ('26.04', ubuntu26)):
        # Accept original CPack names in distinct input directories; rename only
        # the release copy. Internal metadata and original digest are preserved.
        name = f'rationalstudio_{version}-1_ubuntu{target}_amd64.deb'
        if package.name not in (f'rationalstudio_{version}-1_amd64.deb', name) or not package.is_file():
            raise ReleaseError(f'Expected versioned Ubuntu {target} amd64 Debian package')
        check_deb(package, True)
        if gitless_deb_field(package, 'Architecture') != 'amd64':
            raise ReleaseError('Release Debian architecture must be amd64')
        inspect_deb(root, package, source)
        files[name] = package
    if deb.resolve() == ubuntu26.resolve() or sha256(deb) == sha256(ubuntu26):
        raise ReleaseError('Distinct native Ubuntu distributions are required')
    inspect_windows(windows_zip, root, version)
    # Whitelist the two module files; never recursively enumerate/copy Qt input.
    for filename, expected in QT_ASSETS.items():
        path = qt_sources / filename
        if not path.is_file() or path.is_symlink() or sha256(path) != expected:
            raise ReleaseError(f'Qt source archive missing or SHA-256 mismatch: {filename}')
        files[filename] = path
    inventory = qt_sources / 'SHA256SUMS.txt'
    if inventory.exists():
        expected_inventory = ''.join(f'{digest}  {name}\n' for name, digest in QT_ASSETS.items())
        if inventory.read_text(encoding='ascii') != expected_inventory:
            raise ReleaseError('Qt-only SHA256SUMS.txt differs from verified module hashes')
    return files


def gitless_deb_field(path: Path, field: str) -> str:
    return subprocess.check_output(['dpkg-deb', '-f', str(path), field], text=True).strip()


def markdown_escape(value: object) -> str:
    return str(value).replace('\\', '\\\\').replace('|', '\\|').replace('\r', ' ').replace('\n', ' ').replace('`', '\\`')


def qualification_report(version: str, tag: str, commit: str, evidence: dict, draft_candidate: bool = False) -> str:
    lines = [f'# Rational Studio {version} — Release qualification', '',
             f'- Assembly mode: {"DRAFT CANDIDATE — public distribution NOT approved" if draft_candidate else "owner-approved candidate"}',
             f'- Source tag: `{tag}` (annotated)', f'- Git commit: `{commit}`',
             '- Evidence: **maintainer-provided GitHub run identifiers**. This file does not independently query GitHub.',
             '- Rebuilding the tagged source is supported; **bit-for-bit identical binary packages are not guaranteed**.', '',
             '## Hosted build/test results', '',
             '| Check | Runner | CTests | Toolchain | GitHub run |', '|---|---|---:|---|---|']
    for key, (name, _, _) in CHECKS.items():
        item = evidence['checks'][key]
        lines.append(f'| {name} | `{item["runner"]}` | {item["ctests_passed"]} PASS (reported) | '
                     f'{markdown_escape(item["toolchain"])} | {item["run_url"]} |')
    for key, (name, _, _) in CHECKS.items():
        item = evidence['checks'][key]
        lines += ['', f'**{name}** — workflow: {markdown_escape(item["workflow"])}; conclusion: {item["conclusion"]}.',
                  f'Results: {markdown_escape(item["results"])}',
                  f'Evidence references: {markdown_escape(", ".join(item["evidence_paths"]))}']
        if 'artifact_id' in item:
            lines.append(f'Artifact ID: {item["artifact_id"]}; original package SHA-256: `{item["artifact_sha256"]}`.')
    lines += ['', '## Native Ubuntu 26.04 qualification', '',
              'Recorded separately from Ubuntu 24.04 GitHub Actions; same tagged source.', '']
    for key, value in evidence['ubuntu_26_04'].items():
        lines.append(f'- **{key}**: {markdown_escape(json.dumps(value) if isinstance(value, (dict, list)) else value)}')
    lines += ['', '## Manual release gates (supplied review status)', '',
              f'- Reviewer: {markdown_escape(evidence["review"]["reviewer"] or "PENDING — no reviewer recorded")}',
              f'- Reviewed: {markdown_escape(evidence["review"]["reviewed_utc"] or "PENDING — no review timestamp recorded")}',
              '- The following are reviewer-supplied references/statements, not automated licensing certification:', '']
    for key in REVIEW_KEYS:
        lines.append(f'- **{key.replace("_", " ").title()}**: {markdown_escape(evidence["review"][key] or "PENDING")}')
    lines += ['', '## Unchanged limitations', '',
              '- Known Windows 125% DPI layout issue remains documented.',
              '- Qt and MSVC redistribution must be reviewed for the exact published binaries.',
              '- Users must be able to access corresponding Qt source and relinking instructions.',
              '- Fresh source/archive checksums and release asset inventory accompany this record.', '']
    return '\n'.join(lines)


def assemble(root: Path, tag: str, deb: Path, windows_zip: Path, qt_sources: Path,
             evidence_file: Path, output: Path, ubuntu26: Path | None = None,
             draft_candidate: bool = False) -> Path:
    version, commit = verify_tag(root, tag)
    evidence = evidence_read(evidence_file, commit, draft_candidate)
    packages = validate_packages(root, version, deb, windows_zip, qt_sources, ubuntu26)
    for job, path in (('linux_deb', deb), ('windows_portable', windows_zip)):
        expected = evidence['checks'][job]['artifact_sha256']
        actual = sha256(path)
        if expected != actual:
            raise ReleaseError(f'{job} package differs from the recorded tagged-run artifact SHA-256')
    if ubuntu26 is None or sha256(ubuntu26) != evidence['ubuntu_26_04']['artifact_sha256']:
        raise ReleaseError('Ubuntu 26.04 package differs from recorded native artifact SHA-256')
    if gitless_deb_field(ubuntu26, 'Depends') != evidence['ubuntu_26_04']['depends']:
        raise ReleaseError('Ubuntu 26.04 dependency evidence differs from actual package')
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
        report_path.write_text(qualification_report(version, tag, commit, evidence, draft_candidate), encoding='utf-8')
        files[report_path.name] = report_path
        # Manifest contains only source/environment/evidence metadata and checksums.
        payload = {name: {'sha256': sha256(path), 'size_bytes': path.stat().st_size}
                   for name, path in sorted(files.items())}
        for filename, target, reference in (
            (f'rationalstudio_{version}-1_ubuntu24.04_amd64.deb', 'ubuntu-24.04-amd64', 'checks.linux_deb'),
            (f'rationalstudio_{version}-1_ubuntu26.04_amd64.deb', 'ubuntu-26.04-amd64', 'ubuntu_26_04'),
            (f'RationalStudio-{version}-Windows-x64.zip', 'windows-x64', 'checks.windows_portable'),
        ):
            payload[filename].update(distribution_target=target, qualification_reference=reference)
        payload[source_name]['provenance'] = f'git archive of {commit}'
        manifest = {'schema': 2, 'project': 'RationalStudio', 'version': version, 'tag': tag,
                    'source_commit': commit, 'assets': payload, 'checks': evidence['checks'],
                    'assembly_mode': 'draft-candidate' if draft_candidate else 'approved',
                    'public_distribution_approved': not draft_candidate,
                    'distribution_targets': ['ubuntu-24.04-amd64', 'ubuntu-26.04-amd64', 'windows-x64'],
                    'ubuntu_26_04': evidence['ubuntu_26_04'],
                    'review': evidence['review'],
                    'known_review_limitations': {k: v for k, v in evidence['review'].items()
                                               if not v or re.search(r'PENDING|NOT REVIEWED', v, re.I)},
                    'reviewer': evidence['review']['reviewer'], 'reviewed_utc': evidence['review']['reviewed_utc'],
                    'note': 'Manifest records supplied technical evidence and explicit review status; no bit-for-bit build reproducibility or legal certification is claimed.'}
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
    stage.add_argument('--deb', '--deb-ubuntu24', dest='deb', type=Path, required=True)
    stage.add_argument('--deb-ubuntu26', type=Path, required=True)
    stage.add_argument('--draft-candidate', action='store_true',
                       help='Permit pending human review for an unpublished draft; technical checks remain mandatory')
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
                              a.qt_source_dir.resolve(), a.evidence.resolve(), a.output.resolve(),
                              a.deb_ubuntu26.resolve(), a.draft_candidate)
            print(f'PASS: candidate bundle ready (not published): {output}')
            print('Release owner must review all assets, checksums and evidence before any public publication; leave the GitHub Release in Draft status for owner inspection.')
    except (ValueError, AuditError, OSError, subprocess.CalledProcessError, json.JSONDecodeError) as exc:
        p.exit(1, f'ERROR: {exc}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
