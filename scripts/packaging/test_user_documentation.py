"""Issue #65: source-only release docs and runtime Help contract tests."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class UserDocumentationTests(unittest.TestCase):
    def test_workspaces_current(self):
        g = (ROOT / 'docs/USER_GUIDE.md').read_text(encoding='utf-8')
        for word in ('Scientific', 'Graphing', 'Exact Fractions', 'Advanced Math',
                     'Parametric', 'Polar', 'Implicit', 'Inequality', 'Regression',
                     'Windows', 'Linux', '125% scaling', 'Ctrl+F1'):
            with self.subTest(word=word): self.assertIn(word, g)

    def test_main_readme_link(self):
        r = (ROOT / 'README.md').read_text(encoding='utf-8')
        self.assertIn('docs/USER_GUIDE.md', r)
        self.assertIn('docs/ENGINEERING_HISTORY.md', r)
        self.assertNotIn('Implicit graphs; complex arithmetic; matrices', r)

    def test_help_offline_and_tested(self):
        source = (ROOT / 'src/gui/MainWindow.cpp').read_text(encoding='utf-8')
        check = (ROOT / 'tests/KeyboardNavigationGuiTests.cpp').read_text(encoding='utf-8')
        help_text = (ROOT / 'src/gui/UserHelpContent.hpp').read_text(encoding='utf-8')
        self.assertIn('userGuideHelp', source)
        self.assertIn('userGuideDialog', source)
        self.assertIn('userGuideDialog', check)
        self.assertIn('Intersections', help_text)

    def test_both_packages_install_guide(self):
        for path in ('cmake/LinuxPackaging.cmake', 'cmake/WindowsPackaging.cmake'):
            with self.subTest(path=path):
                self.assertIn('docs/USER_GUIDE.md', (ROOT / path).read_text(encoding='utf-8'))
        self.assertIn('USER_GUIDE.md', (ROOT / 'scripts/packaging/check_windows_zip.py').read_text())
        self.assertIn('USER_GUIDE.md', (ROOT / 'scripts/packaging/check_deb.py').read_text())

if __name__ == '__main__':
    unittest.main()
