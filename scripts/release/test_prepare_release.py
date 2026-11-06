"""Issue #66 source-only release protocol regressions; no network or publication."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prepare_release as release


class ReleaseProtocolTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='rationalstudio-release-tests-')
        self.addCleanup(self.tmp.cleanup)
        self.work = Path(self.tmp.name)
        self.root = self.work / 'repo'
        self.root.mkdir()
        contents = {
            'CMakeLists.txt': 'cmake_minimum_required(VERSION 3.16)\nproject(RationalStudio VERSION 0.1.0 LANGUAGES CXX)\n',
            'docs/USER_GUIDE.md': 'user guide\n',
            'docs/ENGINEERING_HISTORY.md': 'engineering history\n',
            'VERIFICATION.md': 'historical evidence\n',
            'THIRD_PARTY_DEPENDENCIES.md': 'third parties\n',
            'LICENSE.MD': 'MIT license test fixture\n',
            'scripts/packaging/test_user_documentation.py': 'test fixture\n',
            'scripts/release/prepare_release.py': 'release tool fixture\n',
            'docs/release/RELEASE_NOTES_v0.1.0.md': '# Rational Studio v0.1.0\n\nInitial release.\n',
        }
        for required in release.REQUIRED_SOURCE:
            contents.setdefault(required, 'tracked fixture source\n')
        for rel, value in contents.items():
            path = self.root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(value, encoding='utf-8')
        self.run_git('init', '-q')
        self.run_git('config', 'user.name', 'Fixture Maintainer')
        self.run_git('config', 'user.email', 'fixture@example.invalid')
        self.run_git('add', '.')
        self.run_git('commit', '-qm', 'test fixture')
        self.run_git('tag', '-a', 'v0.1.0', '-m', 'v0.1.0 fixture')
        self.commit = self.run_git('rev-parse', 'HEAD')

    def run_git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.root), *args], text=True).strip()

    def evidence(self):
        data = {'source_commit': self.commit,
                'checks': {k: {'run_url': f'https://github.com/SpilledMilkFactory/RationalStudio/actions/runs/{1000+i}',
                               'source_commit': self.commit, 'ctests_passed': count, 'runner': runner,
                               'toolchain': 'GCC/Qt6 or MSVC 2022 Qt 6.10.3',
                               'conclusion': 'success', 'workflow': 'Fixture workflow',
                               'evidence_paths': ['public-evidence/ctest.log'], 'results': 'CTest and applicable smoke/package checks PASS'}
                           for i, (k, (_, count, runner)) in enumerate(release.CHECKS.items())},
                'ubuntu_26_04': self.native_evidence(),
                'review': {**{k: 'Evidence: reviewed release artifact inventory and exact dependency versions.'
                              for k in release.REVIEW_KEYS},
                           'reviewer': 'Fixture Maintainer', 'reviewed_utc': '2026-11-02T20:00:00Z',
                           'approval_statement': 'I have reviewed the exact release assets and approve public distribution.'}}
        for key in ('linux_deb', 'windows_portable'):
            data['checks'][key]['artifact_sha256'] = 'a' * 64
            data['checks'][key]['artifact_id'] = 12345
        return data

    def native_evidence(self):
        return {'source_commit': self.commit, 'build_host': 'native fixture host',
                'os_version': 'Ubuntu 26.04.1 LTS', 'cmake_version': '4.2.3',
                'compiler_version': 'GCC 15.2.0', 'qt_version': '6.10.2',
                'ctests_passed': 73, 'artifact_sha256': 'a' * 64,
                'build_configuration': 'Release', 'native_build': True,
                'packaging_validation': 'PASS', 'licensing_audit': 'PASS',
                'depends': 'fixture dependency metadata',
                'runtime': {key: 'PASS' for key in ('extracted_gui', 'cli_arithmetic',
                            'clean_install', 'desktop_graphing', 'package_removal')},
                'evidence_paths': ['public-evidence/native/ctest.xml'], 'limitations': []}

    def write_evidence(self, d=None):
        path = self.work / 'evidence.json'
        path.write_text(json.dumps(d if d is not None else self.evidence()), encoding='utf-8')
        return path

    def test_tag_matches_version_and_head(self):
        self.assertEqual(release.verify_tag(self.root, 'v0.1.0'), ('0.1.0', self.commit))

    def test_tag_not_present(self):
        with self.assertRaises(release.ReleaseError):
            release.verify_tag(self.root, 'v0.1.1')

    def test_wrong_version(self):
        with self.assertRaisesRegex(release.ReleaseError, 'differs'):
            release.verify_tag(self.root, 'v0.1.2')

    def test_untracked_source_rejected(self):
        (self.root / 'extra.cpp').write_text('untracked')
        with self.assertRaisesRegex(release.ReleaseError, 'not clean'):
            release.verify_tag(self.root, 'v0.1.0')

    def test_modified_source_rejected(self):
        (self.root / 'LICENSE.MD').write_text('tampered')
        with self.assertRaisesRegex(release.ReleaseError, 'not clean'):
            release.verify_tag(self.root, 'v0.1.0')

    def test_lightweight_tag_rejected(self):
        self.run_git('tag', '-d', 'v0.1.0')
        self.run_git('tag', 'v0.1.0')
        with self.assertRaisesRegex(release.ReleaseError, 'not an annotated'):
            release.verify_tag(self.root, 'v0.1.0')

    def test_missing_tracked_documentation_rejected(self):
        self.run_git('rm', '-q', 'docs/USER_GUIDE.md')
        self.run_git('commit', '-qm', 'delete required doc')
        self.run_git('tag', '-fa', 'v0.1.0', '-m', 'retag fixture')
        with self.assertRaisesRegex(release.ReleaseError, 'Required file is not tracked'):
            release.verify_tag(self.root, 'v0.1.0')

    def test_evidence_complete_accepts(self):
        self.assertEqual(release.evidence_read(self.write_evidence(), self.commit)['source_commit'], self.commit)

    def test_evidence_rejects_wrong_sha(self):
        d = self.evidence()
        d['checks']['linux_release']['source_commit'] = '1' * 40
        with self.assertRaisesRegex(release.ReleaseError, 'revision'):
            release.evidence_read(self.write_evidence(d), self.commit)

    def test_evidence_rejects_wrong_test_count(self):
        d = self.evidence()
        d['checks']['core_sanitize']['ctests_passed'] = 41
        with self.assertRaisesRegex(release.ReleaseError, 'CTest count'):
            release.evidence_read(self.write_evidence(d), self.commit)

    def test_evidence_rejects_missing_job(self):
        d = self.evidence()
        del d['checks']['linux_gui_smoke']
        with self.assertRaisesRegex(release.ReleaseError, 'exactly'):
            release.evidence_read(self.write_evidence(d), self.commit)

    def test_evidence_rejects_fake_run(self):
        d = self.evidence()
        d['checks']['windows_portable']['run_url'] = 'https://example.net/pretend'
        with self.assertRaisesRegex(release.ReleaseError, 'Invalid hosted'):
            release.evidence_read(self.write_evidence(d), self.commit)

    def test_evidence_rejects_unreviewed_license(self):
        d = self.evidence()
        d['review']['windows_qt'] = 'PENDING'
        with self.assertRaisesRegex(release.ReleaseError, 'incomplete'):
            release.evidence_read(self.write_evidence(d), self.commit)

    def test_evidence_rejects_missing_package_digest(self):
        d = self.evidence()
        del d['checks']['linux_deb']['artifact_sha256']
        with self.assertRaisesRegex(release.ReleaseError, 'fields'):
            release.evidence_read(self.write_evidence(d), self.commit)

    def test_evidence_rejects_no_publication_approval(self):
        d = self.evidence()
        d['review']['approval_statement'] = ''
        with self.assertRaisesRegex(release.ReleaseError, 'approval'):
            release.evidence_read(self.write_evidence(d), self.commit)

    def test_archive_and_checksums_match_tag_source(self):
        # This deliberately isolates the bundle writer; real packaging is
        # validated by the existing Debian/Windows/Qt validators, not mocked in CI.
        fake = {}
        for name in ('rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb', 'rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb', 'RationalStudio-0.1.0-Windows-x64.zip',
                     *release.QT_ASSETS.keys()):
            path = self.work / name
            path.write_bytes(name.encode('ascii'))
            fake[name] = path
        output = self.work / 'release-candidate'
        evidence = self.evidence()
        evidence['checks']['linux_deb']['artifact_sha256'] = hashlib.sha256(fake['rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb'].read_bytes()).hexdigest()
        evidence['checks']['windows_portable']['artifact_sha256'] = hashlib.sha256(fake['RationalStudio-0.1.0-Windows-x64.zip'].read_bytes()).hexdigest()
        evidence['ubuntu_26_04']['artifact_sha256'] = release.sha256(fake['rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb'])
        evidence_file = self.write_evidence(evidence)
        with patch.object(release, 'validate_packages', return_value=fake), patch.object(release, 'gitless_deb_field', return_value=evidence['ubuntu_26_04']['depends']):
            self.assertEqual(release.assemble(self.root, 'v0.1.0', fake['rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb'],
                            fake['RationalStudio-0.1.0-Windows-x64.zip'], self.work,
                            evidence_file, output, fake['rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb']), output)
        manifest = json.loads((output / 'RELEASE_MANIFEST.json').read_text())
        self.assertEqual(manifest['source_commit'], self.commit)
        self.assertEqual(manifest['tag'], 'v0.1.0')
        self.assertEqual(len(manifest['distribution_targets']), 3)
        self.assertEqual(manifest['assets']['rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb']['qualification_reference'], 'ubuntu_26_04')
        self.assertIn('rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb', manifest['assets'])
        self.assertTrue(manifest['public_distribution_approved'])
        self.assertFalse(any('qt-everywhere-full' in n for n in manifest['assets']))
        for line in (output / 'SHA256SUMS.txt').read_text().splitlines():
            digest, filename = line.split('  ', 1)
            self.assertEqual(digest, hashlib.sha256((output / filename).read_bytes()).hexdigest())
        evidence['review'] = {k: 'PENDING: owner review' for k in release.REVIEW_KEYS}
        evidence['review'].update(reviewer='', reviewed_utc='', approval_statement='')
        draft_output = self.work / 'draft-candidate'
        with patch.object(release, 'validate_packages', return_value=fake), patch.object(release, 'gitless_deb_field', return_value=evidence['ubuntu_26_04']['depends']):
            release.assemble(self.root, 'v0.1.0', fake['rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb'],
                             fake['RationalStudio-0.1.0-Windows-x64.zip'], self.work,
                             self.write_evidence(evidence), draft_output,
                             fake['rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb'], True)
        draft_manifest = json.loads((draft_output / 'RELEASE_MANIFEST.json').read_text())
        self.assertFalse(draft_manifest['public_distribution_approved'])
        self.assertEqual(draft_manifest['review']['approval_statement'], '')
        self.assertIn('public distribution NOT approved', (draft_output / 'RELEASE_QUALIFICATION.md').read_text())
        self.assertIn('PENDING', (draft_output / 'RELEASE_QUALIFICATION.md').read_text())
        # Restore the approved record for the refusal-to-overwrite check below.
        evidence_file = self.write_evidence({**evidence, 'review': self.evidence()['review']})
        with tarfile.open(output / 'RationalStudio-0.1.0-source.tar.gz') as archive:
            names = archive.getnames()
            self.assertIn('RationalStudio-0.1.0/docs/USER_GUIDE.md', names)
            self.assertFalse(any(name.startswith('RationalStudio-0.1.0/.git/') for name in names))
        with patch.object(release, 'validate_packages', return_value=fake), patch.object(release, 'gitless_deb_field', return_value=evidence['ubuntu_26_04']['depends']):
            with self.assertRaisesRegex(release.ReleaseError, 'already exists'):
                release.assemble(self.root, 'v0.1.0', fake['rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb'],
                                 fake['RationalStudio-0.1.0-Windows-x64.zip'], self.work,
                                 evidence_file, output, fake['rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb'])

    def test_assembly_rejects_package_hash_mismatch(self):
        deb = self.work / 'rationalstudio_0.1.0-1_amd64.deb'
        deb.write_bytes(b'local package does not match run')
        portable = self.work / 'RationalStudio-0.1.0-Windows-x64.zip'
        portable.write_bytes(b'local ZIP does not match run')
        with patch.object(release, 'validate_packages', return_value={deb.name: deb, portable.name: portable}):
            with self.assertRaisesRegex(release.ReleaseError, 'differs'):
                release.assemble(self.root, 'v0.1.0', deb, portable, self.work,
                                 self.write_evidence(), self.work / 'output')

    def test_assembly_rejects_native_hash_mismatch(self):
        deb = self.work / 'rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb'
        native = self.work / 'rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb'
        portable = self.work / 'RationalStudio-0.1.0-Windows-x64.zip'
        for path in (deb, native, portable):
            path.write_bytes(path.name.encode())
        d = self.evidence()
        d['checks']['linux_deb']['artifact_sha256'] = release.sha256(deb)
        d['checks']['windows_portable']['artifact_sha256'] = release.sha256(portable)
        with patch.object(release, 'validate_packages', return_value={deb.name: deb, native.name: native, portable.name: portable}):
            with self.assertRaisesRegex(release.ReleaseError, 'Ubuntu 26.04 package differs'):
                release.assemble(self.root, 'v0.1.0', deb, portable, self.work,
                                 self.write_evidence(d), self.work / 'output', native)

    def test_tag_wrong_head_rejected(self):
        (self.root / 'extra').write_text('next commit')
        self.run_git('add', '.')
        self.run_git('commit', '-qm', 'later source')
        with self.assertRaisesRegex(release.ReleaseError, 'HEAD'):
            release.verify_tag(self.root, 'v0.1.0')

    def test_tracked_cache_or_full_qt_archive_rejected(self):
        for rel in ('scripts/__pycache__/cached.pyc', 'qt-everywhere-full/old.tar.xz'):
            with self.subTest(rel=rel):
                path = self.root / rel
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'excluded')
                self.run_git('add', rel)
                self.run_git('commit', '-qm', 'bad tracked artifact fixture')
                self.run_git('tag', '-fa', 'v0.1.0', '-m', 'bad fixture')
                with self.assertRaisesRegex(release.ReleaseError, 'Forbidden'):
                    release.verify_tag(self.root, 'v0.1.0')
                self.run_git('rm', rel)
                self.run_git('commit', '-qm', 'remove bad fixture')

    def test_draft_permits_pending_reviews_but_final_fails(self):
        d = self.evidence()
        d['review'] = {k: 'PENDING: owner review' for k in release.REVIEW_KEYS}
        d['review'].update(reviewer='', reviewed_utc='', approval_statement='')
        d['ubuntu_26_04']['runtime']['clean_install'] = 'PENDING'
        d['ubuntu_26_04']['limitations'] = ['Independent clean-machine acceptance is pending.']
        path = self.write_evidence(d)
        self.assertEqual(release.evidence_read(path, self.commit, True), d)
        with self.assertRaises(release.ReleaseError):
            release.evidence_read(path, self.commit)
        d['checks']['linux_release']['ctests_passed'] = 72
        with self.assertRaises(release.ReleaseError):
            release.evidence_read(self.write_evidence(d), self.commit, True)

    def test_draft_rejects_false_public_approval(self):
        with self.assertRaisesRegex(release.ReleaseError, 'leave public approval'):
            release.evidence_read(self.write_evidence(), self.commit, True)

    def test_native_evidence_rejects_wrong_revision_build_or_digest(self):
        for key, value in [('source_commit', 'b' * 40), ('native_build', False),
                           ('os_version', 'Ubuntu 24.04'), ('ctests_passed', 72),
                           ('artifact_sha256', 'invalid'), ('licensing_audit', 'PENDING')]:
            with self.subTest(key=key):
                d = self.evidence()
                d['ubuntu_26_04'][key] = value
                with self.assertRaises(release.ReleaseError):
                    release.evidence_read(self.write_evidence(d), self.commit)

    def test_pending_or_missing_hosted_evidence_rejected(self):
        for key, value in [('conclusion', 'pending'), ('evidence_paths', []), ('artifact_id', 0)]:
            with self.subTest(key=key):
                d = self.evidence()
                d['checks']['linux_deb'][key] = value
                with self.assertRaises(release.ReleaseError):
                    release.evidence_read(self.write_evidence(d), self.commit)

    def test_dual_package_validation_and_qt_whitelist(self):
        debs = []
        for target in ('24', '26'):
            directory = self.work / target
            directory.mkdir()
            deb = directory / 'rationalstudio_0.1.0-1_amd64.deb'
            deb.write_bytes(target.encode())
            debs.append(deb)
        portable = self.work / 'RationalStudio-0.1.0-Windows-x64.zip'
        portable.write_bytes(b'fixture ZIP')
        modules = {}
        for name in release.QT_ASSETS:
            path = self.work / name
            path.write_bytes(name.encode())
            modules[name] = release.sha256(path)
        forbidden = self.work / 'qt-everywhere-full'
        forbidden.mkdir()
        (forbidden / 'never-copy.tar.xz').write_bytes(b'excluded')
        with patch.object(release, 'check_deb') as deb_check, patch.object(release, 'inspect_source', return_value={}), patch.object(release, 'inspect_deb') as audit, patch.object(release, 'inspect_windows') as windows, patch.object(release, 'gitless_deb_field', return_value='amd64'), patch.object(release, 'QT_ASSETS', modules):
            files = release.validate_packages(self.root, '0.1.0', debs[0], portable, self.work, debs[1])
            self.assertEqual(deb_check.call_count, 2)
            self.assertEqual(audit.call_count, 2)
            windows.assert_called_once()
            self.assertIn('rationalstudio_0.1.0-1_ubuntu24.04_amd64.deb', files)
            self.assertIn('rationalstudio_0.1.0-1_ubuntu26.04_amd64.deb', files)
            self.assertEqual(len(files), 5)
            with self.assertRaisesRegex(release.ReleaseError, 'Distinct native'):
                release.validate_packages(self.root, '0.1.0', debs[0], portable, self.work, debs[0])
            for name in modules:
                path = self.work / name
                saved = path.read_bytes()
                path.write_bytes(b'bad checksum')
                with self.assertRaisesRegex(release.ReleaseError, 'Qt source'):
                    release.validate_packages(self.root, '0.1.0', debs[0], portable, self.work, debs[1])
                path.unlink()
                with self.assertRaisesRegex(release.ReleaseError, 'Qt source'):
                    release.validate_packages(self.root, '0.1.0', debs[0], portable, self.work, debs[1])
                path.write_bytes(saved)
            (self.work / 'SHA256SUMS.txt').write_text('incorrect inventory')
            with self.assertRaisesRegex(release.ReleaseError, 'Qt-only'):
                release.validate_packages(self.root, '0.1.0', debs[0], portable, self.work, debs[1])

    def test_missing_ubuntu26_rejected(self):
        with self.assertRaisesRegex(release.ReleaseError, 'Missing Ubuntu 26.04'):
            release.validate_packages(self.root, '0.1.0', self.work/'missing.deb',
                                      self.work/'missing.zip', self.work)

    def test_version_is_single_cmake_source_for_cli_and_gui(self):
        cmake = (release.ROOT / 'CMakeLists.txt').read_text()
        cli = (release.ROOT / 'src/cli/main.cpp').read_text()
        app = (release.ROOT / 'src/main.cpp').read_text()
        self.assertIn('RATIONALSTUDIO_VERSION="${PROJECT_VERSION}"', cmake)
        self.assertIn('RATIONALSTUDIO_VERSION', cli)
        self.assertIn('setApplicationVersion', app)


if __name__ == '__main__':
    unittest.main()
