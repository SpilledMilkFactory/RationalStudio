#!/usr/bin/env python3
"""Cross-platform negative/positive fixture tests for Issue #62 archive auditor."""
import hashlib
import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from check_windows_zip import inspect, SOURCE_MATCH

VERSION = '0.1.0'
ROOT = 'RationalStudio-0.1.0-Windows-x64/'


def fixture(root: Path, extra=None, omitted=None):
    s = root/'source'; s.mkdir(parents=True)
    contents = {}
    for i, (archive_path, src_path) in enumerate(SOURCE_MATCH.items()):
        src = s/src_path; src.parent.mkdir(parents=True, exist_ok=True)
        data = ('license-'+str(i)).encode()
        src.write_bytes(data); contents[archive_path] = data
    for fn in ('RationalStudio.exe','RationalCLI.exe', 'Qt6Core.dll', 'Qt6Gui.dll',
               'Qt6Widgets.dll', 'platforms/qwindows.dll', 'msvcp140.dll',
               'vcruntime140.dll'):
        contents[fn] = ('binary-'+fn).encode()
    contents['qt.conf'] = b'[Paths]\nPrefix=.\nPlugins=.\n'
    if extra:
        contents.update(extra)
    if omitted:
        for k in omitted: contents.pop(k, None)
    manifest = ''.join(hashlib.sha256(data).hexdigest()+'  '+key+'\n'
                       for key, data in sorted(contents.items())).encode()
    contents['SHA256SUMS.txt'] = manifest
    dest = root/'fixture.zip'
    with ZipFile(dest,'w',ZIP_DEFLATED) as z:
        for k, v in contents.items(): z.writestr(ROOT+k, v)
    return dest,s


class Validation(unittest.TestCase):
    def test_positive(self):
        with tempfile.TemporaryDirectory() as d:
            z,s=fixture(Path(d)); result=inspect(z,s,VERSION)
            self.assertEqual(result['files'], len(SOURCE_MATCH)+10)

    def test_missing_plugin(self):
        with tempfile.TemporaryDirectory() as d:
            z,s=fixture(Path(d),omitted=['platforms/qwindows.dll'])
            with self.assertRaisesRegex(ValueError,'Missing required'): inspect(z,s,VERSION)

    def test_modified_license(self):
        with tempfile.TemporaryDirectory() as d:
            z,s=fixture(Path(d)); (s/'LICENSE.MD').write_text('forged')
            with self.assertRaisesRegex(ValueError,'differs from source'): inspect(z,s,VERSION)

    def test_debug_artifacts(self):
        with tempfile.TemporaryDirectory() as d:
            z,s=fixture(Path(d),extra={'Qt6Cored.dll':b'debug'})
            with self.assertRaisesRegex(ValueError,'Qt debug'): inspect(z,s,VERSION)

    def test_path_traversal(self):
        with tempfile.TemporaryDirectory() as d:
            z,s=fixture(Path(d))
            with ZipFile(z,'a') as a: a.writestr(ROOT+'../escape.txt',b'bad')
            with self.assertRaisesRegex(ValueError,'Unsafe'): inspect(z,s,VERSION)

    def test_corrupt_hash(self):
        with tempfile.TemporaryDirectory() as d:
            z,s=fixture(Path(d))
            with ZipFile(z,'a') as a: a.writestr(ROOT+'RationalStudio.exe',b'altered')
            with self.assertRaisesRegex(ValueError,'Duplicate'): inspect(z,s,VERSION)

if __name__=='__main__': unittest.main()
