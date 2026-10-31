#!/usr/bin/env python3
"""Issue #64: fail-closed source-and-DEB third-party license audit.

This checks concrete package evidence. It is NOT legal advice/certification.
No network access, elevated privileges, or local Qt runtime is required.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

LICENSES = ('BSL-1.0.txt', 'LGPL-3.0.txt', 'GPL-3.0.txt')
REQUIRED = ('usr/bin/RationalStudio', 'usr/share/doc/rationalstudio/copyright',
            'usr/share/doc/rationalstudio/THIRD_PARTY_NOTICES.txt')
QT_SONAMES = ('libQt6Core.so.', 'libQt6Gui.so.', 'libQt6Widgets.so.')


class AuditError(Exception):
    pass


def invoke(*args):
    p = subprocess.run(args, text=True, capture_output=True, check=False)
    if p.returncode:
        raise AuditError(f"{args[0]} failed: {p.stderr.strip()[:1200]}")
    return p.stdout


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, message):
    if not ok:
        raise AuditError(message)


def parse_needed(text):
    return re.findall(r'\(NEEDED\)\s+Shared library: \[([^\]]+)\]', text)


def parse_deps(text):
    """Debian Depends dependency alternatives with version constraints."""
    return [re.findall(r'\b([a-z][a-z0-9+.-]+)\b', field.split('(')[0])
            for field in text.split(',')]


def inspect_source(root):
    cfg = root / 'cmake/LinuxPackaging.cmake'
    core = root / 'src/core/Rational.hpp'
    license_file = root / 'LICENSE.MD'
    for path in (cfg, core, license_file, root/'CMakeLists.txt',
                 root/'packaging/linux/THIRD_PARTY_NOTICES.txt'):
        require(path.is_file(), f'missing source file: {path.relative_to(root)}')
    require('MIT License' in license_file.read_text(), 'project MIT license not identified')
    require('boost/multiprecision/cpp_int.hpp' in core.read_text(),
            'expected Boost header dependency changed; review inventory')
    cmake = (root/'CMakeLists.txt').read_text()
    require('Qt${QT_VERSION_MAJOR}::Widgets' in cmake,
            'Qt target/dependency changed; review inventory')
    contents = cfg.read_text()
    for name in ('THIRD_PARTY_NOTICES.txt', *LICENSES, 'qt6-qpa-plugins',
                 'CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON'):
        require(name in contents, f'packaging metadata missing: {name}')
    notice = (root/'packaging/linux/THIRD_PARTY_NOTICES.txt').read_text()
    for key in ('Qt 6', 'LGPL', 'Boost.Multiprecision', 'BSL', 'source', 'modified'):
        require(key in notice, f'notice missing expected subject: {key}')
    for name in LICENSES:
        path = root/'packaging/licenses'/name
        require(path.is_file() and path.stat().st_size > 500,
                f'license missing/truncated: {name}')
    for name, header in [('LGPL-3.0.txt','GNU LESSER GENERAL PUBLIC LICENSE'),
                         ('GPL-3.0.txt','GNU GENERAL PUBLIC LICENSE'),
                         ('BSL-1.0.txt','Boost Software License')]:
        require(header in (root/'packaging/licenses'/name).read_text(),
                f'license text not recognized: {name}')
    # Stop on tracked vendored/bundled artifacts that this audit has not inventoried.
    for folder in ('third_party','third-party','vendor','external','extern'):
        require(not (root/folder).is_dir(), f'unreviewed vendored directory: {folder}')
    return {'project_license': 'MIT', 'qt_modules': ['Core','Gui','Widgets'],
            'boost_header':'boost/multiprecision/cpp_int.hpp',
            'license_sha256': {name:sha(root/'packaging/licenses'/name) for name in LICENSES}}


def inspect_deb(root, deb, source):
    require(deb.is_file(), f'DEB not found: {deb}')
    name=invoke('dpkg-deb','-f',str(deb),'Package').strip()
    require(name == 'rationalstudio', f'unexpected package name: {name}')
    depends=invoke('dpkg-deb','-f',str(deb),'Depends').strip()
    dep_groups=parse_deps(depends)
    flattened={d for alternatives in dep_groups for d in alternatives}
    for dep in ('qt6-qpa-plugins','libc6'):
        require(dep in flattened, f'missing package dependency: {dep}')
    for prefix in ('libqt6core6', 'libqt6gui6', 'libqt6widgets6'):
        require(any(d.startswith(prefix) for d in flattened),
                f'missing Qt package dependency: {prefix}')
    with tempfile.TemporaryDirectory(prefix='rationalstudio-license-audit-') as temp:
        dest=Path(temp)
        invoke('dpkg-deb','-x',str(deb),str(dest))
        for name in REQUIRED:
            path=dest/name
            require(path.is_file() and not path.is_symlink(), f'missing/unsafe payload: {name}')
        require((dest/'usr/share/doc/rationalstudio/copyright').read_bytes() ==
                (root/'LICENSE.MD').read_bytes(), 'packaged MIT license differs from source')
        packaged_notice=dest/'usr/share/doc/rationalstudio/THIRD_PARTY_NOTICES.txt'
        require(packaged_notice.read_bytes() ==
                (root/'packaging/linux/THIRD_PARTY_NOTICES.txt').read_bytes(),
                'notice missing or differs from versioned source')
        for name in LICENSES:
            path=dest/'usr/share/doc/rationalstudio/licenses'/name
            require(path.is_file() and not path.is_symlink(), f'missing packaged license: {name}')
            require(sha(path)==source['license_sha256'][name],
                    f'license text modified or mismatched: {name}')
        files=[p.relative_to(dest).as_posix() for p in dest.rglob('*') if p.is_file()]
        unexpected=[p for p in files if p.endswith(('.so','.a','.la')) or '.so.' in p]
        require(not unexpected, f'package bundles unreviewed shared/static libraries: {unexpected}')
        icons=[p for p in files if p.lower().endswith(('.png','.svg','.ttf','.otf','.woff','.woff2'))]
        require(not icons, f'package bundles unreviewed assets: {icons}')
        needed={}
        for rel in ('usr/bin/RationalStudio','usr/bin/RationalCLI'):
            binary=dest/rel
            if not binary.exists():
                if rel.endswith('RationalCLI'):continue
                raise AuditError('missing RationalStudio executable')
            dynamic=invoke('readelf','-d',str(binary))
            sonames=parse_needed(dynamic)
            require(sonames, f'no dynamic dependencies in {rel}; review static linking')
            needed[rel]=sonames
        gui=needed['usr/bin/RationalStudio']
        for prefix in QT_SONAMES:
            require(any(s.startswith(prefix) for s in gui),
                    f'Qt shared-library dependency missing: {prefix}')
        require(not any('Qt5' in s for s in gui), 'unexpected Qt5 dependency')
        require(not any('Qt6' in s for s in needed.get('usr/bin/RationalCLI', [])),
                'Qt dependencies unexpectedly introduced into RationalCLI')
        payload=sorted(files)
        allowed={*REQUIRED,'usr/bin/RationalCLI',
                 'usr/share/applications/rationalstudio.desktop',
                 'usr/share/doc/rationalstudio/README.md',
                 *(f'usr/share/doc/rationalstudio/licenses/{item}' for item in LICENSES)}
        require(set(payload) <= allowed,
                f'unreviewed file(s) in package payload: {sorted(set(payload)-allowed)}')
    return {'package':'rationalstudio', 'deb_sha256':sha(deb), 'depends':depends,
            'needed':needed, 'payload':payload, 'qt_bundled':False}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=Path('.'))
    p.add_argument('--deb',type=Path,help='qualify a real .deb; omit for source-only validation')
    p.add_argument('--json',type=Path,help='save machine-readable audit evidence')
    args=p.parse_args()
    root=args.root.resolve()
    try:
        source=inspect_source(root)
        report={'audit_schema':1,'result':'PASS','scope':'source-only' if not args.deb else 'source-and-deb',
                'source':source,'legal_review':'REQUIRED: this is a technical audit, not legal certification'}
        if args.deb:
            report['package']=inspect_deb(root,args.deb.resolve(),source)
    except (AuditError,OSError,UnicodeError) as exc:
        print(f'FAIL: Issue #64 license audit: {exc}',file=sys.stderr)
        return 1
    if args.json:
        args.json.parent.mkdir(parents=True,exist_ok=True)
        args.json.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(f"PASS: Issue #64 {report['scope']} technical licensing audit")
    print('NOTICE: legal/source-availability review remains a separate release gate')
    return 0


if __name__=='__main__':
    sys.exit(main())
