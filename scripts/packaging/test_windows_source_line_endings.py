"""Packaged notices must retain tagged bytes in a Windows-style checkout."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from check_windows_zip import SOURCE_MATCH

ROOT = Path(__file__).resolve().parents[2]


class WindowsSourceLineEndingsTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('git'), 'Git is required')
    def test_autocrlf_checkout_preserves_packaged_source_bytes(self):
        # Simulate the Windows runner's checkout conversion using Git itself;
        # validators must continue comparing exact bytes, without normalization.
        with tempfile.TemporaryDirectory(prefix='rationalstudio-eol-') as tmp:
            source = Path(tmp) / 'source'
            source.mkdir()
            files = {'.gitattributes', *SOURCE_MATCH.values(),
                     'packaging/licenses/ICU-LICENSE.txt'}
            for name in files:
                target = source / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((ROOT / name).read_bytes())

            def git(*args):
                return subprocess.run(['git', '-C', str(source), *args],
                                      check=True, capture_output=True, text=True)

            git('init', '-q')
            git('config', 'core.autocrlf', 'false')
            git('config', 'user.name', 'Line ending fixture')
            git('config', 'user.email', 'fixture@example.invalid')
            git('add', '.')
            git('-c', 'commit.gpgsign=false', 'commit', '-qm', 'fixture')
            checkout = Path(tmp) / 'windows-checkout'
            git('clone', '-q', '--no-local', '--config', 'core.autocrlf=true',
                str(source), str(checkout))
            for name in files:
                with self.subTest(file=name):
                    self.assertEqual((checkout / name).read_bytes(),
                                     (ROOT / name).read_bytes())
            self.assertFalse(subprocess.check_output(
                ['git', '-C', str(checkout), 'status', '--porcelain'], text=True))


if __name__ == '__main__':
    unittest.main()
