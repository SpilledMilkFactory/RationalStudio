#!/usr/bin/env python3
"""Issue #62 portable Windows ZIP validator (stdlib only, runs on Linux/Windows).

Checks archive layout, safety, required Qt/MSVC runtimes, exact notice text,
manifest integrity, and payload exclusions. Does NOT certify legal compliance.
"""
from __future__ import annotations
import argparse
import hashlib
import re
from pathlib import Path, PurePosixPath
from zipfile import ZipFile, BadZipFile

REQUIRED_DOCS = (
    'share/doc/rationalstudio/LICENSE.MD',
    'share/doc/rationalstudio/README.md',
    'share/doc/rationalstudio/THIRD_PARTY_DEPENDENCIES.md',
    'share/doc/rationalstudio/THIRD_PARTY_NOTICES.txt',
    'share/doc/rationalstudio/licenses/LGPL-3.0.txt',
    'share/doc/rationalstudio/licenses/GPL-3.0.txt',
    'share/doc/rationalstudio/licenses/BSL-1.0.txt',
)
SOURCE_MATCH = {
    'share/doc/rationalstudio/LICENSE.MD': 'LICENSE.MD',
    'share/doc/rationalstudio/README.md': 'README.md',
    'share/doc/rationalstudio/THIRD_PARTY_DEPENDENCIES.md': 'THIRD_PARTY_DEPENDENCIES.md',
    'share/doc/rationalstudio/THIRD_PARTY_NOTICES.txt': 'packaging/windows/THIRD_PARTY_NOTICES.txt',
    'share/doc/rationalstudio/licenses/LGPL-3.0.txt': 'packaging/licenses/LGPL-3.0.txt',
    'share/doc/rationalstudio/licenses/GPL-3.0.txt': 'packaging/licenses/GPL-3.0.txt',
    'share/doc/rationalstudio/licenses/BSL-1.0.txt': 'packaging/licenses/BSL-1.0.txt',
}


def inspect(path: Path, source: Path, version: str) -> dict:
    if not path.is_file():
        raise ValueError(f'ZIP not found: {path}')
    name = f'RationalStudio-{version}-Windows-x64'
    archive_root = f'{name}/'
    with ZipFile(path) as archive:
        files = {}
        seen = set()
        for item in archive.infolist():
            raw = item.filename.replace('\\', '/')
            pp = PurePosixPath(raw)
            if (not raw.startswith(archive_root) or raw.startswith('/') or
                    any(x in ('.', '..') for x in pp.parts) or ':' in raw or '\x00' in raw):
                raise ValueError(f'Unsafe/outside-root ZIP member: {raw}')
            if item.is_dir():
                continue
            key = raw.casefold()
            if key in seen:
                raise ValueError(f'Duplicate case-insensitive ZIP member: {raw}')
            seen.add(key)
            rel = raw[len(archive_root):]
            if not rel:
                raise ValueError('Empty entry at archive root')
            files[rel] = item
        required = set(REQUIRED_DOCS) | {
            'RationalStudio.exe', 'RationalCLI.exe',
            'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll',
            'platforms/qwindows.dll', 'qt.conf',
            'msvcp140.dll', 'vcruntime140.dll', 'SHA256SUMS.txt',
        }
        missing = sorted(required - set(files))
        if missing:
            raise ValueError('Missing required portable files: ' + ', '.join(missing))
        if any(x.lower().endswith(('.pdb', '.lib', '.obj', '.ilk', '.exp')) for x in files):
            raise ValueError('Debug/build artifacts found in portable package')
        if any(re.search(r'(?:^|/)qt6\w+d\.dll$', x, re.I) for x in files):
            raise ValueError('Qt debug library found in Release archive')
        if any(re.search(r'(?:^|/)(?:qwindowsd|qoffscreen(d)?|qminimald|vcruntime\d+d|msvcp\d+d|ucrtbased)\.dll$', x, re.I) for x in files):
            raise ValueError('MSVC debug runtime found in Release archive')
        for name_in_zip, src in SOURCE_MATCH.items():
            upstream = source / src
            if not upstream.is_file():
                raise ValueError(f'Source for packaged notice missing: {upstream}')
            if archive.read(files[name_in_zip]) != upstream.read_bytes():
                raise ValueError(f'Packaged notice differs from source: {name_in_zip}')
        conf = archive.read(files['qt.conf']).decode('utf-8-sig').replace('\r\n', '\n')
        if '[Paths]' not in conf or 'Plugins=.' not in conf:
            raise ValueError('qt.conf does not direct Qt to local plugins')
        manifest = archive.read(files['SHA256SUMS.txt']).decode('utf-8-sig')
        recorded = {}
        for line in manifest.splitlines():
            m = re.fullmatch(r'([a-fA-F0-9]{64})  (.+)', line)
            if not m or m.group(2) in recorded:
                raise ValueError(f'Invalid SHA256SUMS record: {line}')
            recorded[m.group(2)] = m.group(1).lower()
        expected_manifest = set(files) - {'SHA256SUMS.txt'}
        if set(recorded) != expected_manifest:
            raise ValueError(f'Manifest inventory mismatch, missing={sorted(expected_manifest-set(recorded))}, extra={sorted(set(recorded)-expected_manifest)}')
        for rel in expected_manifest:
            value = hashlib.sha256(archive.read(files[rel])).hexdigest()
            if recorded[rel] != value:
                raise ValueError(f'Payload checksum mismatch: {rel}')
        all_dlls = sorted(x for x in files if x.lower().endswith('.dll'))
        non_qt = [x for x in all_dlls if not (Path(x).name.lower().startswith('qt6'))]
        return {'files': len(files), 'dlls': all_dlls, 'non_qt_dlls': non_qt,
                'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--zip', type=Path, required=True)
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--version', required=True)
    a = p.parse_args()
    try:
        v = inspect(a.zip, a.source, a.version)
    except (ValueError, OSError, BadZipFile) as e:
        print(f'FAIL: Issue #62 portable ZIP: {e}')
        return 1
    print(f'PASS: Issue #62 portable ZIP: {a.zip.name}, files={v["files"]}, DLLs={len(v["dlls"])}')
    print(f'SHA256: {v["sha256"]}')
    print('Non-Qt DLL inventory for human release/licensing review:')
    for f in v['non_qt_dlls']:
        print('  ' + f)
    print('NOTE: ZIP structure and manifests passed; independent clean-machine/Qt licensing sign-off remains required.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
