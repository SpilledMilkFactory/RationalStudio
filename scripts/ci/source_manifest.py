#!/usr/bin/env python3
"""Stable SHA-256 manifest of first-party source and CI configuration."""
import argparse
import hashlib
from pathlib import Path

SOURCE_DIRS = ('src', 'tests', 'benchmarks', 'scripts', '.github/workflows', 'cmake', 'packaging')
SOURCE_FILES = ('CMakeLists.txt', 'README.md', 'VERIFICATION.md', 'LICENSE.MD', '.gitignore')

def manifest(root: Path) -> str:
    paths = []
    for item in SOURCE_FILES:
        path = root / item
        if path.is_file():
            paths.append(path)
    for item in SOURCE_DIRS:
        folder = root / item
        if folder.is_dir():
            paths.extend(path for path in folder.rglob('*') if path.is_file() and
                         '__pycache__' not in path.parts and path.suffix != '.pyc')
    lines = []
    for path in sorted(paths, key=lambda item: item.relative_to(root).as_posix()):
        name = path.relative_to(root).as_posix()
        lines.append(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {name}')
    return '\n'.join(lines) + '\n'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', default='.')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    root = Path(args.root).resolve()
    Path(args.output).write_text(manifest(root))

if __name__ == '__main__':
    main()
