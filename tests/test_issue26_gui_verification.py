"""Stdlib tests for the Issue #26 manual evidence recorder (not a CTest)."""
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "issue26_gui_verification.py"
spec = importlib.util.spec_from_file_location("issue26_gui_verification", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class ManualEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.file = self.root / "evidence" / "manual_gui.json"
        self.data = mod.fresh_data(self.root, "test-gui")

    def test_initial_incomplete(self):
        outcome = mod.assessment(self.data)
        self.assertEqual("INCOMPLETE", outcome["verdict"])
        self.assertEqual(len([x for x in mod.CHECKS if x["required"]]), len(outcome["unfinished_required"]))

    def test_all_required_pass_optional_skip(self):
        for case in mod.CHECKS:
            self.data["checks"][case["id"]]["status"] = "pass" if case["required"] else "skip"
        self.assertEqual("PASS", mod.assessment(self.data)["verdict"])
        mod.save(self.data, self.file)
        data2 = json.loads(self.file.read_text())
        self.assertEqual("PASS", mod.assessment(data2)["verdict"])
        self.assertIn("Human-attested assessment:** **PASS", self.file.with_suffix(".md").read_text())

    def test_required_skip_is_incomplete(self):
        for case in mod.CHECKS:
            self.data["checks"][case["id"]]["status"] = "pass"
        self.data["checks"][mod.CHECKS[0]["id"]]["status"] = "skip"
        self.assertEqual("INCOMPLETE", mod.assessment(self.data)["verdict"])

    def test_failure_overrides_missing(self):
        self.data["checks"][mod.CHECKS[0]["id"]]["status"] = "fail"
        self.assertEqual("FAIL", mod.assessment(self.data)["verdict"])

    def test_optional_failure_blocks_pass(self):
        for case in mod.CHECKS:
            self.data["checks"][case["id"]]["status"] = "pass"
        self.data["checks"][mod.CHECKS[-1]["id"]]["status"] = "fail"
        self.assertEqual("FAIL", mod.assessment(self.data)["verdict"])

    def test_reject_bad_schema(self):
        self.data["issue"] = 25
        with self.assertRaises(ValueError):
            mod.validate(self.data)

    def test_reject_missing_case(self):
        del self.data["checks"][mod.CHECKS[0]["id"]]
        with self.assertRaises(ValueError):
            mod.validate(self.data)

    def test_reject_unknown_state(self):
        self.data["checks"][mod.CHECKS[0]["id"]]["status"] = "success"
        with self.assertRaises(ValueError):
            mod.validate(self.data)

    def test_save_and_reject_overwrite(self):
        mod.save(self.data, self.file)
        result = subprocess.run([sys.executable, str(SCRIPT), "--output", str(self.file)], capture_output=True, text=True)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("Refusing to overwrite", result.stderr)

    def test_resume_after_quit(self):
        # Record a deliberate 'quit' without asserting any GUI behavior passed.
        result = subprocess.run([sys.executable, str(SCRIPT), "--output", str(self.file)],
                                input="q\n", capture_output=True, text=True)
        self.assertEqual(0, result.returncode)
        self.assertTrue(self.file.is_file())
        self.assertEqual("INCOMPLETE", mod.assessment(json.loads(self.file.read_text()))["verdict"])

    def test_check_exit_status(self):
        mod.save(self.data, self.file)
        cmd = [sys.executable, str(SCRIPT), "--check", "--output", str(self.file)]
        incomplete = subprocess.run(cmd, capture_output=True)
        self.assertEqual(1, incomplete.returncode)
        for case in mod.CHECKS:
            self.data["checks"][case["id"]]["status"] = "pass" if case["required"] else "skip"
        mod.save(self.data, self.file)
        complete = subprocess.run(cmd, capture_output=True)
        self.assertEqual(0, complete.returncode)

    def test_markdown_escape(self):
        self.assertEqual(r"a\|b<br>c", mod.md_escape("a|b\nc"))


if __name__ == "__main__":
    unittest.main()
