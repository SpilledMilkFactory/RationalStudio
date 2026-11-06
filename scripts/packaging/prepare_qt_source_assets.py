#!/usr/bin/env python3
"""Prepare Qt 6.10.3 corresponding-source assets for a public release.

This never writes inside the Git repository unless --output explicitly points
there. Outputs two source .tar.xz files and SHA256SUMS.txt. Does not establish
exact binary/source equivalence or license approval; those need release review.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
from urllib.request import urlopen

BASE = "https://download.qt.io/archive/qt/6.10/6.10.3/submodules/"
ASSETS = {
    "qtbase-everywhere-src-6.10.3.tar.xz": "383dc907816338f0cba72088a524c07458dfc69ce684ca9132fcc4fe91c24b0b",
    "qtsvg-everywhere-src-6.10.3.tar.xz": "b3223fe005f6a4c7f21f34e4ee6ce0094737cff9503ba15bbb171ac18794f76d",
}


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def prepare(output: Path, offline: bool) -> None:
    output.mkdir(parents=True, exist_ok=True)
    lines = []
    for name, expected in ASSETS.items():
        target = output / name
        if not target.exists():
            if offline:
                raise ValueError(f"Required Qt source archive absent: {target}")
            temp = target.with_suffix(target.suffix + ".part")
            try:
                with urlopen(BASE + name, timeout=120) as r, temp.open("wb") as out:
                    for block in iter(lambda: r.read(1024 * 1024), b""):
                        out.write(block)
                temp.replace(target)
            finally:
                if temp.exists():
                    temp.unlink()
        observed = digest(target)
        if observed != expected:
            raise ValueError(f"BAD HASH {name}: {observed}; expected {expected}")
        lines.append(f"{observed}  {name}")
        print(f"PASS {name}: SHA-256 {observed}")
    (output / "SHA256SUMS.txt").write_text("\n".join(lines) + "\n", encoding="ascii")
    print(f"PASS: Qt Base + Qt SVG source assets ready in {output}")
    print("Upload both archives to the SAME GitHub Release as the Windows ZIP. If retaining this Qt-only inventory, name its release copy QT_SOURCE_SHA256SUMS.txt to avoid the top-level SHA256SUMS.txt.")


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--offline", action="store_true", help="Verify existing files only")
    a = p.parse_args()
    try:
        prepare(a.output, a.offline)
    except (OSError, ValueError) as e:
        p.exit(1, f"ERROR: {e}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
