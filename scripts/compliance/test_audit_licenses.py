#!/usr/bin/env python3
"""No-Qt regression checks for Issue #64 compliance gate."""
import shutil
import tempfile
import unittest
from pathlib import Path
from audit_licenses import AuditError, inspect_deb, inspect_source, parse_deps, parse_needed

ROOT = Path(__file__).resolve().parents[2]

class LicenseAuditTests(unittest.TestCase):
    def test_current_source_inventory(self):
        result=inspect_source(ROOT)
        self.assertEqual(result['project_license'],'MIT')
        self.assertEqual(set(result['qt_modules']),{'Core','Gui','Widgets'})
        self.assertEqual(len(result['license_sha256']),3)

    def test_soname_extraction(self):
        data='0x0001 (NEEDED) Shared library: [libQt6Widgets.so.6]\n0x0002 (NEEDED) Shared library: [libc.so.6]'
        self.assertEqual(parse_needed(data), ['libQt6Widgets.so.6','libc.so.6'])

    def test_package_dep_alternatives(self):
        d=parse_deps('qt6-qpa-plugins, libqt6widgets6t64 (>= 6.4) | libqt6widgets6')
        self.assertIn('qt6-qpa-plugins', d[0])
        self.assertIn('libqt6widgets6t64', d[1])

    def test_missing_notices_fail(self):
        with tempfile.TemporaryDirectory() as p:
            root=Path(p)
            for rel in ('CMakeLists.txt','LICENSE.MD','src/core/Rational.hpp',
                        'cmake/LinuxPackaging.cmake','packaging/linux/THIRD_PARTY_NOTICES.txt'):
                dest=root/rel
                dest.parent.mkdir(parents=True,exist_ok=True)
                dest.write_bytes((ROOT/rel).read_bytes())
            for f in ('GPL-3.0.txt','LGPL-3.0.txt','BSL-1.0.txt'):
                dest=root/'packaging/licenses'/f
                dest.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(ROOT/'packaging/licenses'/f,dest)
            (root/'packaging/licenses/LGPL-3.0.txt').unlink()
            with self.assertRaisesRegex(AuditError,'missing/truncated'):
                inspect_source(root)

    def test_incomplete_package_rejected(self):
        # A deterministic .deb without third-party notices must be rejected.
        import subprocess
        with tempfile.TemporaryDirectory() as td:
            tmp=Path(td)
            pkg=tmp/'package'
            (pkg/'DEBIAN').mkdir(parents=True)
            (pkg/'DEBIAN/control').write_text(
                'Package: rationalstudio\nVersion: 0.1.0-1\nArchitecture: amd64\n'
                'Maintainer: test <test@example.invalid>\nDescription: negative fixture\n'
                'Depends: qt6-qpa-plugins, libc6, libqt6core6t64, libqt6gui6, libqt6widgets6\n')
            binary=pkg/'usr/bin/RationalStudio'
            binary.parent.mkdir(parents=True)
            binary.write_text('fixture')
            project_license=pkg/'usr/share/doc/rationalstudio/copyright'
            project_license.parent.mkdir(parents=True)
            shutil.copyfile(ROOT/'LICENSE.MD',project_license)
            deb=tmp/'negative.deb'
            subprocess.run(['dpkg-deb','--build',str(pkg),str(deb)],
                           check=True,capture_output=True)
            with self.assertRaisesRegex(AuditError,'THIRD_PARTY_NOTICES'):
                inspect_deb(ROOT,deb,inspect_source(ROOT))

if __name__=='__main__':
    unittest.main()
