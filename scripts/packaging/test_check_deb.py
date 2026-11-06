"""Real Debian metadata/payload regressions; no installation or Qt required."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from check_deb import check
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from scripts.compliance.audit_licenses import AuditError, inspect_deb, inspect_source

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(shutil.which('dpkg-deb') and shutil.which('dpkg'), 'Debian tools required')
class DebianPackageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.work = Path(self.tmp.name)
        self.pkg = self.work / 'payload'
        (self.pkg / 'DEBIAN').mkdir(parents=True)
        self.version = '0.1.0-1'
        self.arch = 'amd64'
        for name in ('RationalStudio', 'RationalCLI'):
            path = self.pkg / 'usr/bin' / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('fixture only, never executed')
            path.chmod(0o755)
        for src, dest in (
            ('LICENSE.MD', 'usr/share/doc/rationalstudio/copyright'),
            ('README.md', 'usr/share/doc/rationalstudio/README.md'),
            ('docs/USER_GUIDE.md', 'usr/share/doc/rationalstudio/USER_GUIDE.md'),
            ('packaging/linux/rationalstudio.desktop', 'usr/share/applications/rationalstudio.desktop'),
            ('packaging/linux/THIRD_PARTY_NOTICES.txt', 'usr/share/doc/rationalstudio/THIRD_PARTY_NOTICES.txt'),
            *((f'packaging/licenses/{name}', f'usr/share/doc/rationalstudio/licenses/{name}')
              for name in ('BSL-1.0.txt', 'LGPL-3.0.txt', 'GPL-3.0.txt')),
        ):
            path = self.pkg / dest
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / src, path)

    def package(self):
        (self.pkg / 'DEBIAN/control').write_text(
            f'Package: rationalstudio\nVersion: {self.version}\nArchitecture: {self.arch}\n'
            'Maintainer: fixture <fixture@example.invalid>\nDescription: negative fixture\n'
            'Depends: qt6-qpa-plugins, libc6, libqt6core6t64, libqt6gui6, libqt6widgets6\n')
        path = self.work / 'rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb'
        subprocess.run(['dpkg-deb', '--build', str(self.pkg), str(path)], check=True, capture_output=True)
        return path

    def test_metadata_and_required_payload(self):
        self.assertIn('version=0.1.0-1', check(self.package(), True))

    def test_missing_ubuntu26_guide(self):
        (self.pkg / 'usr/share/doc/rationalstudio/USER_GUIDE.md').unlink()
        with self.assertRaisesRegex(ValueError, 'USER_GUIDE'):
            check(self.package(), True)

    def test_wrong_version(self):
        self.version = '1.0.0-1'
        with self.assertRaisesRegex(ValueError, 'version'):
            check(self.package(), True)

    def test_wrong_architecture(self):
        self.arch = 'arm64'
        with self.assertRaisesRegex(ValueError, 'Architecture'):
            check(self.package(), True)

    def test_missing_licenses_rejected(self):
        (self.pkg / 'usr/share/doc/rationalstudio/licenses/LGPL-3.0.txt').unlink()
        with self.assertRaisesRegex(AuditError, 'missing packaged license'):
            inspect_deb(ROOT, self.package(), inspect_source(ROOT))

    def test_stale_documentation_rejected(self):
        (self.pkg / 'usr/share/doc/rationalstudio/USER_GUIDE.md').write_text('old guide')
        with self.assertRaisesRegex(AuditError, 'documentation differs'):
            inspect_deb(ROOT, self.package(), inspect_source(ROOT))


if __name__ == '__main__':
    unittest.main()
