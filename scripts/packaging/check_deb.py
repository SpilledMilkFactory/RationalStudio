#!/usr/bin/env python3
"""Issue #61: inspect a real CPack Debian package, fail closed on incomplete payload."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def run(*argv):
    return subprocess.run(argv, text=True, capture_output=True, check=True).stdout


def check(path: Path, cli: bool) -> str:
    if not path.is_file():
        raise ValueError(f"Missing .deb: {path}")
    if shutil.which('dpkg-deb') is None:
        raise ValueError('dpkg-deb is required')
    metadata = {key: run('dpkg-deb', '-f', str(path), key).strip()
                for key in ('Package', 'Version', 'Architecture', 'Depends')}
    if metadata['Package'] != 'rationalstudio':
        raise ValueError(f"Unexpected package name: {metadata['Package']!r}")
    if not re.fullmatch(r'0\.1\.0-\d+', metadata['Version']):
        raise ValueError(f"Unexpected Issue #61 package version: {metadata['Version']!r}")
    arch = run('dpkg', '--print-architecture').strip()
    if metadata['Architecture'] != arch:
        raise ValueError(f"Architecture mismatch: package={metadata['Architecture']} host={arch}")
    deps = metadata['Depends']
    for required in ('qt6-qpa-plugins', 'libqt6widgets6', 'libc6'):
        # Ubuntu 24.04 Qt6 is libqt6widgets6t64; accept either ABI spelling.
        if required == 'libqt6widgets6':
            if not re.search(r'\blibqt6widgets6(?:t64)?\b', deps):
                raise ValueError(f"Missing Qt Widgets dependency: {deps}")
        elif not re.search(r'\b' + re.escape(required) + r'\b', deps):
            raise ValueError(f"Missing dependency {required}: {deps}")
    with tempfile.TemporaryDirectory(prefix='rationalstudio-deb-') as tmp:
        root = Path(tmp)
        subprocess.run(['dpkg-deb', '-x', str(path), str(root)], check=True,
                       stdout=subprocess.DEVNULL)
        expected = ['usr/bin/RationalStudio', 'usr/share/applications/rationalstudio.desktop',
                    'usr/share/doc/rationalstudio/copyright',
                    'usr/share/doc/rationalstudio/README.md']
        if cli:
            expected.append('usr/bin/RationalCLI')
        for rel in expected:
            item = root / rel
            if not item.is_file() or item.is_symlink():
                raise ValueError(f"Missing/unsafe payload entry: {rel}")
        executable = root / 'usr/bin/RationalStudio'
        if not os.access(executable, os.X_OK):
            raise ValueError('RationalStudio is not executable')
        desktop = (root / 'usr/share/applications/rationalstudio.desktop').read_text()
        for required in ('[Desktop Entry]', 'Type=Application', 'Exec=RationalStudio',
                         'TryExec=RationalStudio', 'Terminal=false', 'Icon=accessories-calculator'):
            if required not in desktop.splitlines():
                raise ValueError(f"Invalid desktop launcher: missing {required}")
        copyright_text = (root / 'usr/share/doc/rationalstudio/copyright').read_text()
        if 'Permission is hereby granted' not in copyright_text:
            raise ValueError('Packaged license does not contain the expected MIT grant')
        # No test binaries or generated build source trees should be installed.
        if (root / 'usr/include').exists() or (root / 'usr/lib/cmake').exists():
            raise ValueError('Package unexpectedly contains development files')
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    return (f"PASS: {path.name} package={metadata['Package']} "
            f"version={metadata['Version']} arch={arch}\n"
            f"Depends: {deps}\nSHA256: {digest}\n")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('deb', type=Path)
    parser.add_argument('--no-cli', action='store_true')
    args = parser.parse_args()
    try:
        print(check(args.deb, not args.no_cli), end='')
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f'ERROR: {exc}\n')
